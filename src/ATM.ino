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
#define START_FREQUENCY 50000000U // 50MHz
#define STOP_FREQUENCY 110000000  // 110MHz

ADF4351 adf4351(SCLK_PIN, MOSI_PIN, LE_PIN, CE_PIN); // declares object PLL of type ADF4351

TMC2130Stepper tuning_driver = TMC2130Stepper(EN_PIN_M1, DIR_PIN_M1, STEP_PIN_M1, CS_PIN_M1, MOSI_PIN, MISO_PIN, SCLK_PIN);
TMC2130Stepper matching_driver = TMC2130Stepper(EN_PIN_M2, DIR_PIN_M2, STEP_PIN_M2, CS_PIN_M2, MOSI_PIN, MISO_PIN, SCLK_PIN);

AccelStepper tuning_stepper = AccelStepper(tuning_stepper.DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper matching_stepper = AccelStepper(matching_stepper.DRIVER, STEP_PIN_M2, DIR_PIN_M2);

Stepper tuner = {tuning_stepper, tuning_driver, DIAG1_PIN_M1, "Tuner"};

Stepper matcher = {matching_stepper, matching_driver, DIAG1_PIN_M2, "Matcher"};

// ADC DAC Module
AD5593R adac = AD5593R(23, I2C_SDA, I2C_SCL);
bool DACs[8] = {0, 0, 1, 1, 0, 0, 0, 0};
bool ADCs[8] = {1, 1, 0, 0, 0, 0, 0, 0};

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
  adf4351.pwrlevel = 2; // This equals -4dBm*/ For the electrical probe coils one should use at least -20dbm so an attenuator is necessary
  adf4351.setf(START_FREQUENCY);

  pinMode(FILTER_SWITCH_A, OUTPUT);
  pinMode(FILTER_SWITCH_B, OUTPUT);

  digitalWrite(FILTER_SWITCH_A, LOW);
  digitalWrite(FILTER_SWITCH_B, HIGH);

  // ADAC module
  adac.enable_internal_Vref();
  adac.set_DAC_max_2x_Vref();
  adac.set_ADC_max_2x_Vref();
  adac.configure_DACs(DACs);

  adac.configure_ADCs(ADCs);

  // RF Switch
  pinMode(RF_SWITCH_PIN, OUTPUT);
  digitalWrite(RF_SWITCH_PIN, HIGH);
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
      printInfo("Not implemented");

      // bruteforce call
      // CAREFULL -> if the current resonance frequency is not within +-5MHz of the target frequency this will not work. Only use this for testing -> otherwise use the automated 'decide' call.
    }
    else if (command == 'b')
    {
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0)
        return;

      printInfo("Bruteforce matching to target frequency in MHz:");
      printInfo(target_frequency_MHz);

      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
      printInfo("Resonance after bruteforce is at:");
      printInfo(String(resonance_frequency));

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

      printInfo("Tuning and Matching to target frequency in MHz (automatic mode):");
      printInfo(target_frequency_MHz);

      uint32_t resonance_frequency = automaticTM(target_frequency);

      printInfo("Resonance after tuning and matching is at:");
      printInfo(resonance_frequency);

      printInfo("Matched to RL in dB:");
      printInfo(calculateRL(resonance_frequency));

      // home call
      // Perform the homing routine by looking for the limit of the capacitors
      // it also places the steppers in a way so there is a resonance dip inside the frequency range
      // CAREFULL -> The values are hardcoded, these need to be changed if there is a different coil in use
    }
    else if (command == 'h')
    {
      printInfo("Homing...");
      tuner.STEPPER.setCurrentPosition(homeStepper(tuner));
      matcher.STEPPER.setCurrentPosition(homeStepper(matcher));
      homed = true;

      printInfo("Resonance frequency after homing:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP / 2);
      printInfo(resonance_frequency);

      // frequency sweep call
      // scans the frequency range for the current resonance frequency
    }
    else if (command == 'f')
    {
      printInfo("Started frequency sweep");
      // Get the start frequency which is the value until the next f character
      char delimiter = 'f';
        // Indices for each variable
      int startFreqIndex = input_line.indexOf(delimiter) + 1;
      int stopFreqIndex = input_line.indexOf(delimiter, startFreqIndex) + 1;
      int freqStepIndex = input_line.indexOf(delimiter, stopFreqIndex) + 1;

      // Extract each variable from the string
      uint32_t startFreq = input_line.substring(startFreqIndex, stopFreqIndex - 1).toInt();  // Subtract 1 to not include the delimiter
      uint32_t stopFreq = input_line.substring(stopFreqIndex, freqStepIndex - 1).toInt();
      uint32_t freqStep = input_line.substring(freqStepIndex).toInt();  // If no second parameter is provided, substring() goes to the end of the string

      uint32_t resonance_frequency = findCurrentResonanceFrequency(startFreq, stopFreq, freqStep);
      
      // This tells the PC that the frequency sweep is finished
      Serial.print("r");
      printInfo(resonance_frequency);

      // calculates Reflection loss for a given frequency
    }
    else if (command == 'r')
    {
      float frequency_MHz = input_line.substring(1).toFloat();
      uint32_t frequency = validateInput(frequency_MHz);
      if (frequency == 0)
        return;

      float reflection_loss = calculateRL(frequency);

      printInfo("For frequency:");
      printInfo(frequency);
      printInfo("RL is:");
      printInfo(reflection_loss);

      // optimize Matching
    }
    else if (command == 'm')
    {
      printInfo("Optimize Matching around frequency:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      printInfo(resonance_frequency);

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
      printInfo("For frequency:");
      printInfo(frequency);
      printInfo("Sum of the reflection is:");
      printInfo(sum);

      // Calibration Call
    }
    else if (command == 'c')
    {
      printInfo("Calibrating ...");
      getCalibrationValues();
    }
    else
    {
      printInfo("Invalid Input");
    }
  }
}

/**
 * @brief This function checks if the input is valid. It checks if the frequency is within the allowed range.
 * 
 * @param frequency_MHz The frequency that should be checked in MHz.
 * @return uint32_t The frequency in Hz if the input is valid, 0 otherwise.
 * 
 * @example validateInput(100); // returns 100000000U
*/
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

/**
 * @brief This function reads the reflection at the current frequency. It does not set the frequency.
 * 
 * @param averages The number of readings that should be averaged
 * @return int The average reflection in millivolts
 * 
 * @example readReflection(64); // reads the reflection at the current frequency and averages over 64 readings
*/
int readReflection(int averages)
{
  int reflection = 0;
  for (int i = 0; i < averages; i++)
    // We multiply by 1000 to get the result in millivolts
    reflection += (adac.read_ADC(0) * 1000);
  return reflection / averages;
}

/**
 * @brief This function reads the phase at the current frequency. It does not set the frequency.
 * 
 * @param averages The number of readings that should be averaged
 * @return int The average phase in millivolts
 * 
 * @example readPhase(64); // reads the phase at the current frequency and averages over 64 readings
*/
int readPhase(int averages)
{
  int phase = 0;
  for (int i = 0; i < averages; i++)
    phase += (adac.read_ADC(1) * 1000);
  return phase / averages;
}
/**
 * @brief This function should be called after manually tuning and matching the probe coil. It will then print the stepper positions for the current resonance frequency.
 * This can be used to calibrate the stepper positions for the different frequency ranges.
 * 
 * @return void
 * 
 * @example getCalibrationValues(); // prints the stepper positions for the current resonance frequency
*/
void getCalibrationValues()
{
  uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

  tuner.STEPPER.setCurrentPosition(0);
  matcher.STEPPER.setCurrentPosition(0);

  int tuner_position = stallStepper(tuner);

  int matcher_position = stallStepper(matcher);

  printInfo("For Resonance frequency:");
  printInfo(resonance_frequency);
  printInfo("Tuner Calibration is:");
  printInfo(tuner_position);
  printInfo("Matcher Position is:");
  printInfo(matcher_position);
}

/**
 * @brief This function performs a homing of the steppers. It returns the current position of the stepper.
 * 
 * @param stepper The stepper that should be homed
 * @return long The current position of the stepper
 * 
 * @example homeStepper(tuner); // homes the tuner stepper and returns the current position
*/
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

/**
 * @brief This function controls the stepper so that they hit the limit and stall. It then returns the current position of the stepper.
 *
 * @param stepper The stepper that should be stalled
 * @return int The current position of the stepper
 * 
 * @example stallStepper(tuner); // stalls the tuner stepper and returns the current position
*/
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

/**
 * 
*/
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
  setFrequency(frequency);

  float reflection = readReflection(64);

  float reflection_loss = reflection / 6; // Divide by the amplifier gain
  reflection_loss = reflection_loss / 24; // Divide by the logamp slope

  return reflection_loss;
}

/**
 * @brief This function finds the current resonance frequency of the coil. There should be a substential dip already present atm.
 * It also returns the data of the frequency scan which can then be sent to the PC for plotting.
 * 
 * @param start_frequency The frequency at which the search should start
 * @param stop_frequency The frequency at which the search should stop
 * @param frequency_step The frequency step size
 * @return int32_t The current resonance frequency
 * 
 * @example findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP); // finds the current resonance frequency
*/
int32_t findCurrentResonanceFrequency(uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step)
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
    //adf4351.setf(frequency);
    setFrequency(frequency);

    // delay(5); // This delay is essential! There is a glitch with ADC2 that leads to wrong readings if GPIO27 is set to high for multiple microseconds.

    current_reflection = readReflection(4);
    current_phase = readPhase(4);

    // Send out the frequency identifier f with the frequency value
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
/**
 * @brief This function sets the frequency of the frequency synthesizer and switches the filterbank accordingly.
 * 
 * @param frequency The frequency that should be set
 * @return void
 * 
 * @example setFrequency(100000000U); // sets the frequency to 100MHz
*/
void setFrequency(uint32_t frequency)
{
  // First we check what filter has to be used from the FILTERS array
  // Then we set the filterbank accordingly
  for (int i = 0; i < sizeof(FILTERS) / sizeof(FILTERS[0]); i++)
  {
    // For the first filter we just check if the frequency is below the fg
    if ((i == 0) && (frequency < FILTERS[i].fg))
    {
      digitalWrite(FILTER_SWITCH_A, FILTERS[i].control_input_a);
      digitalWrite(FILTER_SWITCH_B, FILTERS[i].control_input_b);
      break;
    }
    // For the last filter we just check if the frequency is above the fg
    else if ((i == sizeof(FILTERS) / sizeof(FILTERS[0]) - 1) && (frequency > FILTERS[i].fg))
    {
      digitalWrite(FILTER_SWITCH_A, FILTERS[i].control_input_a);
      digitalWrite(FILTER_SWITCH_B, FILTERS[i].control_input_b);
      break;
    }
    // For the filters in between we check if the frequency is between the fg and the fg of the previous filter
    else if ((frequency < FILTERS[i].fg) &&  (frequency > FILTERS[i - 1].fg))
    {
      digitalWrite(FILTER_SWITCH_A, FILTERS[i].control_input_a);
      digitalWrite(FILTER_SWITCH_B, FILTERS[i].control_input_b);
      break;
    }
  }
  // Finally we set the frequency
  adf4351.setf(frequency);
  
}

/**
 * @brief This function tries out different capacitor positions until iteration depth is reached OR current_resonancy frequency matches the target_frequency.
 * 
 * @param target_frequency The frequency that should be matched
 * @param current_resonance_frequency The current resonance frequency
 * @return int32_t The current resonance frequency
 * 
 * @example bruteforceResonance(100000000U, 90000000U); // tries to match 100MHz with a current resonance frequency of 90MHz
*/
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


/**
 * @brief This function sums up the reflection around a given frequency.
 * 
 * @param center_frequency The frequency around which the reflection should be summed up
 * @return int The sum of the reflection around the given frequency
 * 
 * @example sumReflectionAroundFrequency(100000000U);
*/
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

/**
 * @brief This method should be called when one wants to print information to the serial port.
 * 
 * @param text The text that should be printed to the serial monitor. It should be a string
 * @return void
 * 
 * @example printInfo("This is a test"); // prints "iThis is a test"
 */
void printInfo(String text) {
  Serial.println("i" + text);
}

/**
 * @brief This method should be called when one wants to print information to the serial port.
 * 
 * @param number The number that should be printed to the serial monitor. It should be a number
 * @return void
 * 
 * @example printInfo(123U); // prints "i123"
 */
void printInfo(uint32_t number) {
  Serial.println("i" + String(number)); // convert the number to a string before concatenating
}

/**
 * @brief This method should be called when one wants to an error to the serial port.
 * 
 * @param text The text that should be printed to the serial monitor. It should be a string
 * @return void
 * 
 * @example printError("Duck not found"); // prints "eDuck not found"
 */
void printError(String text) {
  Serial.println("e" + text);
}
