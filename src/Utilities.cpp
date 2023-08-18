#include <TMC2130Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>

#include "Utilities.h"

// Frequency Settings
#define FREQUENCY_STEP 100000U    // 100kHz frequency steps for initial frequency sweep
#define START_FREQUENCY 50000000U // 50MHz
#define STOP_FREQUENCY 110000000  // 110MHz

int32_t findCurrentResonanceFrequency(uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step, boolean print_data)
{
  int maximum_reflection = 0;
  int current_reflection = 0;
  int current_phase = 0;
  uint32_t minimum_frequency = 0;
  float reflection = 0;

  adf4351.setf(start_frequency); // A frequency value needs to be set once -> there seems to be a bug with the first SPI call
  delay(50);

  for (uint32_t frequency = start_frequency; frequency <= stop_frequency; frequency += frequency_step)
  {
    // adf4351.setf(frequency);
    setFrequency(frequency);

    // delay(5); // This delay is essential! There is a glitch with ADC2 that leads to wrong readings if GPIO27 is set to high for multiple microseconds.

    current_reflection = readReflection(4);
    current_phase = readPhase(4);

    // Send out the frequency identifier f with the frequency value
    if (print_data)
      Serial.println(String("f") + frequency + "r" + current_reflection + "p" + current_phase);

    if (current_reflection > maximum_reflection)
    {
      minimum_frequency = frequency;
      maximum_reflection = current_reflection;
    }
  }

  adf4351.setf(minimum_frequency);
  delay(50);
  reflection = readReflection(16);
  if (reflection < 130)
  {
    DEBUG_PRINT("Resonance could not be found.");
    DEBUG_PRINT(reflection);
    return 0;
  }

  // Capacitor needs to charge - therefore rerun around area with longer delay. -> REFACTOR THIS!!!!
  maximum_reflection = 0;
  for (uint32_t frequency = minimum_frequency - 300000U; frequency <= minimum_frequency + 300000U; frequency += frequency_step)
  {
    adf4351.setf(frequency);

    current_reflection = readReflection(64);

    if (current_reflection > maximum_reflection)
    {
      minimum_frequency = frequency;
      maximum_reflection = current_reflection;
    }
  }

  return minimum_frequency;
}

void setFrequency(uint32_t frequency)
{
  // First we check what filter has to be used from the FILTERS array
  // Then we set the filterbank accordingly
  int i;
  for (i = 0; i < sizeof(FILTERS) / sizeof(FILTERS[0]); i++)
  {
    // For the first filter we just check if the frequency is below the fg
    if ((i == 0) && (frequency < FILTERS[i].fg))
      break;
    // For the last filter we just check if the frequency is above the fg
    else if ((i == sizeof(FILTERS) / sizeof(FILTERS[0]) - 1) && (frequency > FILTERS[i].fg))
      break;
    // For the filters in between we check if the frequency is between the fg and the fg of the previous filter
    else if ((frequency < FILTERS[i].fg) && (frequency > FILTERS[i - 1].fg))
      break;
  }
  if (active_filter.fg != FILTERS[i].fg)
  {
    printInfo("Switching filter to: " + String(FILTERS[i].fg) + "Hz");
    active_filter = FILTERS[i];
    digitalWrite(FILTER_SWITCH_A, FILTERS[i].control_input_a);
    digitalWrite(FILTER_SWITCH_B, FILTERS[i].control_input_b);
  }

  // Finally we set the frequency
  adf4351.setf(frequency);
}

int readReflection(int averages)
{
  int reflection = 0;
  for (int i = 0; i < averages; i++)
    // We multiply by 1000 to get the result in millivolts
    reflection += (adac.read_ADC(0) * 1000);
  return reflection / averages;
}

int readPhase(int averages)
{
  int phase = 0;
  for (int i = 0; i < averages; i++)
    phase += (adac.read_ADC(1) * 1000);
  return phase / averages;
}

int sumReflectionAroundFrequency(uint32_t center_frequency)
{
  int sum_reflection = 0;

  // sum approach -> cummulates reflection around resonance -> reduce influence of wrong minimum and noise
  for (uint32_t frequency = center_frequency - 500000U; frequency < center_frequency + 500000U; frequency += FREQUENCY_STEP / 10)
  {
    adf4351.setf(frequency);
    delay(10);
    sum_reflection += readReflection(16);
  }

  return sum_reflection;
}

int32_t bruteforceResonance(uint32_t target_frequency, uint32_t current_resonance_frequency)
{
  // Change Tuning Stepper -> Clockwise => Freq goes up
  // Dir = 0 => Anticlockwise movement
  int rotation = 0; // rotation == 1 -> clockwise, rotation == -1 -> counterclockwise

  int ITERATIONS = 25; // Iteration depth
  int iteration_steps = 0;

  float MATCHING_THRESHOLD = 140; // if the reflection at the current resonance frequency is lower than this threshold re-matching is necessary -> calibrate to ~RL-8dB
  float resonance_reflection = 0;

  int32_t delta_frequency = target_frequency - current_resonance_frequency;

  if (delta_frequency < 0)
    rotation = -1; // negative delta means currentresonance is too high, hence anticlockwise movement is necessary
  else
    rotation = 1;

  int iteration_start = rotation * (STEPS_PER_ROTATION / 20);

  iteration_steps = iteration_start;
  DEBUG_PRINT(iteration_start);

  //'bruteforce' the stepper position to match the target frequency

  for (int i = 0; i < ITERATIONS; i++)
  {
    tuner.STEPPER.move(iteration_steps);
    tuner.STEPPER.runToPosition();

    // Only rematch matcher for large step width
    if (iteration_steps == iteration_start)
    {
      matcher.STEPPER.move(-iteration_steps * 3);
      matcher.STEPPER.runToPosition();
    }

    // @ Optimization possibility: Reduce frequency range when close to target_frequency
    current_resonance_frequency = findCurrentResonanceFrequency(current_resonance_frequency - 5000000U, current_resonance_frequency + 5000000U, FREQUENCY_STEP / 2);

    DEBUG_PRINT(current_resonance_frequency);

    // Stops the iteration if the minima matches the target frequency
    if (current_resonance_frequency == target_frequency)
      break;

    adf4351.setf(current_resonance_frequency);
    delay(100);
    resonance_reflection = readReflection(16);
    DEBUG_PRINT(resonance_reflection);

    if (resonance_reflection < MATCHING_THRESHOLD)
    {
      optimizeMatching(current_resonance_frequency);
    }

    // This means the bruteforce resolution was too high and the resonance frequency overshoot
    // therfore the turn direction gets inverted and the increment halfed
    if ((current_resonance_frequency > target_frequency) && (rotation == 1))
    {
      rotation = -1;
      iteration_steps /= 2;
      iteration_steps *= rotation;
    }
    else if ((current_resonance_frequency < target_frequency) && (rotation == -1))
    {
      rotation = 1;
      iteration_steps /= 2;
      iteration_steps *= -rotation;
    }
  }

  return current_resonance_frequency;
}

int optimizeMatching(uint32_t current_resonance_frequency)
{
  int ITERATIONS = 50;
  int iteration_steps = 0;

  int maximum_reflection = 0;
  int current_reflection = 0;
  int minimum_matching_position = 0;
  int last_reflection = 10e5;
  int rotation = 1;

  // Look which rotation direction improves matching.
  rotation = getMatchRotation(current_resonance_frequency);

  DEBUG_PRINT(rotation);

  // This tries to find the minimum reflection while ignoring the change in resonance -> it always looks for minima within
  iteration_steps = rotation * (STEPS_PER_ROTATION / 20);

  DEBUG_PRINT(iteration_steps);

  adf4351.setf(current_resonance_frequency);
  for (int i = 0; i < ITERATIONS; i++)
  {
    DEBUG_PRINT(i);
    current_reflection = 0;

    matcher.STEPPER.move(iteration_steps);
    matcher.STEPPER.runToPosition();

    delay(50);

    current_resonance_frequency = findCurrentResonanceFrequency(current_resonance_frequency - 1000000U, current_resonance_frequency + 1000000U, FREQUENCY_STEP / 2);

    // Skip this iteration if the resonance has been lost
    if (current_resonance_frequency == 0)
    {
      delay(1000); // Wait for one second since something has gone wrong
      continue;
    }

    adf4351.setf(current_resonance_frequency);
    delay(100);

    current_reflection = readReflection(16);
    // current_reflection = sumReflectionAroundFrequency(current_resonance_frequency);

    if (current_reflection > maximum_reflection)
    {
      minimum_matching_position = matcher.STEPPER.currentPosition();
      maximum_reflection = current_reflection;
      DEBUG_PRINT("Maximum");
      DEBUG_PRINT(minimum_matching_position);
    }

    DEBUG_PRINT(matcher.STEPPER.currentPosition());
    DEBUG_PRINT(current_resonance_frequency);
    DEBUG_PRINT(last_reflection);

    last_reflection = current_reflection;

    if (iteration_steps == 0)
      break;

    DEBUG_PRINT(current_reflection);
  }

  matcher.STEPPER.moveTo(minimum_matching_position);
  matcher.STEPPER.runToPosition();

  DEBUG_PRINT(matcher.STEPPER.currentPosition());

  return (maximum_reflection);
}

int getMatchRotation(uint32_t current_resonance_frequency)
{

  matcher.STEPPER.move(STEPS_PER_ROTATION / 2);
  matcher.STEPPER.runToPosition();

  current_resonance_frequency = findCurrentResonanceFrequency(current_resonance_frequency - 1000000U, current_resonance_frequency + 1000000U, FREQUENCY_STEP / 10);
  // int clockwise_match = sumReflectionAroundFrequency(current_resonance_frequency);
  if (current_resonance_frequency != 0)
    adf4351.setf(current_resonance_frequency);
  delay(100);
  int clockwise_match = readReflection(64);

  matcher.STEPPER.move(-2 * (STEPS_PER_ROTATION / 2));
  matcher.STEPPER.runToPosition();

  current_resonance_frequency = findCurrentResonanceFrequency(current_resonance_frequency - 1000000U, current_resonance_frequency + 1000000U, FREQUENCY_STEP / 10);
  // int anticlockwise_match = sumReflectionAroundFrequency(current_resonance_frequency);
  adf4351.setf(current_resonance_frequency);
  delay(100);
  int anticlockwise_match = readReflection(64);

  matcher.STEPPER.move(STEPS_PER_ROTATION / 2);
  matcher.STEPPER.runToPosition();

  DEBUG_PRINT(clockwise_match);
  DEBUG_PRINT(anticlockwise_match);

  if (clockwise_match > anticlockwise_match)
    return 1;
  else
    return -1;
}

int stallStepper(Stepper stepper)
{
  stepper.STEPPER.moveTo(-9999999);

  while (!digitalRead(stepper.STALL_PIN))
  {
    stepper.STEPPER.run();
  }

  DEBUG_PRINT(stepper.STEPPER.currentPosition());

  stepper.STEPPER.stop();

  return stepper.STEPPER.currentPosition(); // returns value until limit is reached
}

long homeStepper(Stepper stepper)
{
  stallStepper(stepper);
  stepper.STEPPER.setCurrentPosition(0);
  stepper.STEPPER.moveTo(1000);
  stepper.STEPPER.runToPosition();

  stepper.STEPPER.setMaxSpeed(3000);
  stepper.STEPPER.setAcceleration(3000);

  stepper.DRIVER.sg_stall_value(-64); // Stall value needs to be lowered because of slower stepper
  stallStepper(stepper);
  stepper.DRIVER.sg_stall_value(STALL_VALUE);

  stepper.STEPPER.setMaxSpeed(12000);
  stepper.STEPPER.setAcceleration(12000);

  stepper.STEPPER.setCurrentPosition(0);

  stepper.STEPPER.moveTo(1000);

  stepper.STEPPER.runToPosition();

  DEBUG_PRINT(stepper.STEPPER.currentPosition());

  return stepper.STEPPER.currentPosition();
}

uint32_t validateInput(float frequency_MHz)
{
  uint32_t frequency_Hz = frequency_MHz * 1000000U;

  if (frequency_Hz < START_FREQUENCY)
  {
    printError("Invalid input: frequency too low");
    return 0;
  }
  else if (frequency_Hz > 300000000U)
  {
    printError("Invalid input: frequency too high");
    return 0;
  }
  else
  {
    return frequency_Hz;
  }
}

void printInfo(String text)
{
  Serial.println("i" + text);
}

void printInfo(uint32_t number)
{
  Serial.println("i" + String(number)); // convert the number to a string before concatenating
}

void printError(String text)
{
  Serial.println("e" + text);
}