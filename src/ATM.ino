#include <TMC2130Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>

#include "ADF4351.h"
#include "AD5593R.h"
#include "Pins.h"      // Pins are defined here
#include "Positions.h" // Calibrated frequency positions are defined her
#include "Stepper.h"   // Stepper specific values are defined here

#define DEBUG

#include "Debug.h"

// Frequency Settings
#define FREQUENCY_STEP 100000U    // 100kHz frequency steps for initial frequency sweep
#define START_FREQUENCY 50000000U // 80MHz
#define STOP_FREQUENCY 110000000  // 120MHz

ADF4351 adf4351(SCLK_PIN, MOSI_PIN, LE_PIN, CE_PIN); // declares object PLL of type ADF4351

TMC2130Stepper tuning_driver = TMC2130Stepper(EN_PIN_M1, DIR_PIN_M1, STEP_PIN_M1, CS_PIN_M1, MOSI_PIN, MISO_PIN, SCLK_PIN);
TMC2130Stepper matching_driver = TMC2130Stepper(EN_PIN_M2, DIR_PIN_M2, STEP_PIN_M2, CS_PIN_M2, MOSI_PIN, MISO_PIN, SCLK_PIN);

AccelStepper tuning_stepper = AccelStepper(tuning_stepper.DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper matching_stepper = AccelStepper(matching_stepper.DRIVER, STEP_PIN_M2, DIR_PIN_M2);

Stepper tuner = {tuning_stepper, tuning_driver, DIAG1_PIN_M1, "Tuner"};

Stepper matcher = {matching_stepper, matching_driver, DIAG1_PIN_M2, "Matcher"};

// ADC DAC Module
AD5593R adac =  AD5593R(23, I2C_SDA, I2C_SCL);
bool DACs[8] = {0,1,1,0,0,0,0,0};
bool ADCs[8] = {1,0,0,0,0,0,0,0};


boolean homed = false;

void setup()
{
  Serial.begin(115200);

  pinMode(MISO_PIN, INPUT_PULLUP); // Seems to be necessary for SPI to work

  tuner.DRIVER.begin();          // Initiate pins
  tuner.DRIVER.rms_current(400); // Set stepper current to 400mA.
  tuner.DRIVER.microsteps(16);
  tuner.DRIVER.coolstep_min_speed(0xFFFFF); // 20bit max - needs to be set for stallguard
  tuner.DRIVER.diag1_stall(1);
  tuner.DRIVER.diag1_active_high(1);
  tuner.DRIVER.sg_stall_value(STALL_VALUE);
  tuner.DRIVER.shaft_dir(0);
  tuner.DRIVER.stealthChop(1); // Enable extremely quiet stepping

  pinMode(DIAG1_PIN_M1, INPUT);

  DEBUG_PRINT("DRV_STATUS=0b");
  Serial.println(tuning_driver.DRV_STATUS(), BIN);

  matcher.DRIVER.begin();          // Initiate pins and registeries
  matcher.DRIVER.rms_current(200); // Set stepper current to 200mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  matcher.DRIVER.microsteps(16);
  matcher.DRIVER.coolstep_min_speed(0xFFFFF); // 20bit max - needs to be set for stallguard
  matcher.DRIVER.diag1_stall(1);
  matcher.DRIVER.diag1_active_high(1);
  matcher.DRIVER.sg_stall_value(STALL_VALUE - 2);
  matcher.DRIVER.shaft_dir(0);
  matcher.DRIVER.stealthChop(1); // Enable extremely quiet stepping

  digitalWrite(EN_PIN_M1, LOW);

  digitalWrite(EN_PIN_M2, LOW);

  tuner.STEPPER.setMaxSpeed(12000);
  tuner.STEPPER.setAcceleration(12000);
  tuner.STEPPER.setEnablePin(EN_PIN_M1);
  tuner.STEPPER.setPinsInverted(true, false, true);
  tuner.STEPPER.enableOutputs();

  matcher.STEPPER.setMaxSpeed(12000);
  matcher.STEPPER.setAcceleration(12000);
  matcher.STEPPER.setEnablePin(EN_PIN_M2);
  matcher.STEPPER.setPinsInverted(true, false, true);
  matcher.STEPPER.enableOutputs();

  tuner.STEPPER.setCurrentPosition(0);

  matcher.STEPPER.setCurrentPosition(0);

  adf4351.begin();

  adf4351.setrf(25000000U);
  adf4351.pwrlevel = 2; // This equals -4dBm*/
  adf4351.setf(START_FREQUENCY);

  pinMode(FILTER_SWITCH_A, OUTPUT);
  pinMode(FILTER_SWITCH_B, OUTPUT);

  digitalWrite(FILTER_SWITCH_A, LOW);
  digitalWrite(FILTER_SWITCH_B, HIGH);

  // changeFrequencyRange(HOME_RANGE);

  // ADAC module
  adac.enable_internal_Vref();
  adac.set_DAC_max_2x_Vref();
  adac.set_ADC_max_2x_Vref();
  adac.configure_DACs(DACs);

  adac.configure_ADCs(ADCs);
}

// Implement Serial communication ...
// This could probably cleaned up by using structs for the inputs, pointing to the different functions- > would reduce copy-paste code and make adding functions more intuitive
void loop()
{
  if (Serial.available())
  {

    String input_line = Serial.readStringUntil('\n'); // read string until newline character

    char command = input_line.charAt(0); // gets first character of input

    // approximate call
    // CAREFULL -> if the coil has no proper matching in the frequency range this will not work! Only use this for testing -> otherwise use the automated 'decide' call.
    if (command == 'a')
    {
      Serial.println("Not implemented");

      // bruteforce call
      // CAREFULL -> if the current resonance frequency is not within +-5MHz of the target frequency this will not work. Only use this for testing -> otherwise use the automated 'decide' call.
    }
    else if (command == 'b')
    {
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0)
        return;

      Serial.println("Bruteforce matching to target frequency in MHz:");
      Serial.println(target_frequency_MHz);

      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
      Serial.println("Resonance after bruteforce is at:");
      Serial.println(resonance_frequency);

      // decide call
      // this function decides what kind of t&m mode should be used based on the relationship between target frequency and current resonance
      // it also makes sure that there a homing routine performed in case there is currently no proper resonance in the frequency range
    }
    else if (command == 'd')
    {
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0)
        return;

      Serial.println("Tuning and Matching to target frequency in MHz (automatic mode):");
      Serial.println(target_frequency_MHz);

      uint32_t resonance_frequency = automaticTM(target_frequency);

      Serial.println("Resonance after tuning and matching is at:");
      Serial.println(resonance_frequency);

      Serial.println("Matched to RL in dB:");
      Serial.println(calculateRL(resonance_frequency));

      // home call
      // Perform the homing routine by looking for the limit of the capacitors
      // it also places the steppers in a way so there is a resonance dip inside the frequency range
      // CAREFULL -> The values are hardcoded, these need to be changed if there is a different coil in use
    }
    else if (command == 'h')
    {
      Serial.println("Homing...");
      tuner.STEPPER.setCurrentPosition(homeStepper(tuner));
      matcher.STEPPER.setCurrentPosition(homeStepper(matcher));
      homed = true;
      changeFrequencyRange(HOME_RANGE);

      Serial.println("Resonance frequency after homing:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP / 2);
      Serial.println(resonance_frequency);

      // frequency sweep call
      // scans the frequency range for the current resonance frequency
    }
    else if (command == 'f')
    {
      Serial.println("Frequency sweep...");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP / 2);
      Serial.println("Resonance is at:");
      Serial.println(resonance_frequency);

      // calculates Reflection loss for a given frequency
    }
    else if (command == 'r')
    {
      float frequency_MHz = input_line.substring(1).toFloat();
      uint32_t frequency = validateInput(frequency_MHz);
      if (frequency == 0)
        return;

      float reflection_loss = calculateRL(frequency);

      Serial.println("For frequency:");
      Serial.println(frequency);
      Serial.println("RL is:");
      Serial.println(reflection_loss);

      // optimize Matching
    }
    else if (command == 'm')
    {
      Serial.println("Optimize Matching around frequency:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      Serial.println(resonance_frequency);

      optimizeMatching(resonance_frequency);

      // calculate sum
    }
    else if (command == 's')
    {

      float frequency_MHz = input_line.substring(1).toFloat();
      uint32_t frequency = validateInput(frequency_MHz);
      if (frequency == 0)
        return;

      int sum = sumReflectionAroundFrequency(frequency);
      Serial.println("For frequency:");
      Serial.println(frequency);
      Serial.println("Sum of the reflection is:");
      Serial.println(sum);

      // Calibration Call
    }
    else if (command == 'c')
    {
      Serial.println("Calibrating ...");
      getCalibrationValues();
    }
    else
    {
      Serial.println("Invalid Input");
    }
  }
}

// This helper function checks if the input frequency is plausible, if so it returns the value in Hz
// otherwise it returns 0
uint32_t validateInput(float frequency_MHz)
{
  uint32_t frequency_Hz = frequency_MHz * 1000000U;

  if (frequency_Hz < START_FREQUENCY)
  {
    Serial.println("Invalid input: frequency too low");
    return 0;
  }
  else if (frequency_Hz > STOP_FREQUENCY)
  {
    Serial.println("Invalid input: frequency too high");
    return 0;
  }
  else
  {
    return frequency_Hz;
  }
}

int readReflection(int averages)
{
  int reflection = 0;
  for (int i = 0; i < averages; i++)
    // We multiply by 1000 to get the result in millivolts
    reflection += (adac.read_ADC(0) * 1000);
  return reflection / averages;
}

void getCalibrationValues()
{
  uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

  tuner.STEPPER.setCurrentPosition(0);
  matcher.STEPPER.setCurrentPosition(0);

  int tuner_position = stallStepper(tuner);

  int matcher_position = stallStepper(matcher);

  Serial.println("For Resonance frequency:");
  Serial.println(resonance_frequency);
  Serial.println("Tuner Calibration is:");
  Serial.println(tuner_position);
  Serial.println("Matcher Position is:");
  Serial.println(matcher_position);
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

// This function changes the filterbank settings for the selected target range
// and drives the tuner and matcher stepper to the center of the selected frequency range
void changeFrequencyRange(FrequencyRange target_range)
{
  digitalWrite(FILTER_SWITCH_A, target_range.FILTER.control_input_a);
  digitalWrite(FILTER_SWITCH_B, target_range.FILTER.control_input_b);

  tuner.STEPPER.moveTo(target_range.TUNING_CENTER_POSITION);
  tuner.STEPPER.runToPosition();

  matcher.STEPPER.moveTo(target_range.MATCHING_CENTER_POSITION);
  matcher.STEPPER.runToPosition();
}

uint32_t automaticTM(uint32_t target_frequency)
{
  uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

  DEBUG_PRINT("Resonance Frequency before TM");
  DEBUG_PRINT(resonance_frequency);

  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);

  optimizeMatching(resonance_frequency);

  resonance_frequency = findCurrentResonanceFrequency(resonance_frequency - 1000000U, resonance_frequency + 1000000U, FREQUENCY_STEP / 2);

  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);

  return resonance_frequency;
}

// calculates the Reflection Loss at a specified frequency
// 24mV/dB slope
float calculateRL(uint32_t frequency)
{
  adf4351.setf(frequency);
  delay(100);

  float reflection = readReflection(64);

  float reflection_loss = reflection / 6; // Divide by the amplifier gain
  reflection_loss = reflection_loss / 24;    // Divide by the logamp slope

  return reflection_loss;
}

// Finds current Resonance Frequency of the coil. There should be a substential dip already present atm.
// It also returns the data of the frequency scan which can then be sent to the PC for plotting.

int32_t findCurrentResonanceFrequency(uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step)
{
  int maximum_reflection = 0;
  int current_reflection = 0;
  uint32_t minimum_frequency = 0;
  float reflection = 0;

  adf4351.setf(start_frequency); // A frequency value needs to be set once -> there seems to be a bug with the first SPI call
  delay(50);

  for (uint32_t frequency = start_frequency; frequency <= stop_frequency; frequency += frequency_step)
  {
    adf4351.setf(frequency);

    delay(5); // This delay is essential! There is a glitch with ADC2 that leads to wrong readings if GPIO27 is set to high for multiple microseconds.

    current_reflection = readReflection(4);

    // Send out the frequency identifier f with the frequency value
    Serial.print("f");
    Serial.print(frequency);
    Serial.print("r");
    Serial.println(current_reflection);
    
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
    delay(100); // Higher delay so the capacitor has time to charge

    current_reflection = readReflection(64);

    if (current_reflection > maximum_reflection)
    {
      minimum_frequency = frequency;
      maximum_reflection = current_reflection;
    }
  }

  return minimum_frequency;
}

// Tries out different capacitor position until iteration depth is reached OR current_resonancy frequency matches the target_frequency
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

//
// Matcher clockwise lowers resonance frequency

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

// probably do this for multiple positions in each direction
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
