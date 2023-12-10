#include "global.h"

#include "CommandManager.h"
#include "commands/FrequencySweep.h"
#include "commands/TuneMatch.h"
#include "commands/Homing.h"
#include "commands/SetVoltages.h"
#include "commands/MeasureReflection.h"
#include "commands/VoltageSweep.h"
#include "commands/ControlSwitch.h"
#include "commands/MoveStepper.h"

#define DEBUG

#include "Debug.h"

CommandManager commandManager;

// Commands
FrequencySweep frequencySweep;
TuneMatch tuneMatch;
Homing homing;
SetVoltages setVoltages;
MeasureReflection measureReflection;
VoltageSweep voltageSweep;
ControlSwitch controlSwitch;
MoveStepper moveStepper;

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

// This sets the active filter to the first one in the list
Filter active_filter = FILTERS[0];

boolean homed = false;

void setup()
{
  Serial.begin(115200);

  // Here the commands are registered
  commandManager.registerCommand('f', &frequencySweep);
  commandManager.registerCommand('d', &tuneMatch);
  commandManager.registerCommand('h', &homing);
  commandManager.registerCommand('v', &setVoltages);
  commandManager.registerCommand('r', &measureReflection);
  commandManager.registerCommand('s', &voltageSweep);
  commandManager.registerCommand('c', &controlSwitch);
  commandManager.registerCommand('m', &moveStepper);

  pinMode(MISO_PIN, INPUT_PULLUP); // Seems to be necessary for SPI to work

  // Setup fo the tuning stepper
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

  // Setup fo the matching stepper
  matcher.DRIVER.begin();          // Initiate pins and registeries
  matcher.DRIVER.rms_current(200); // Set stepper current to 200mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  matcher.DRIVER.microsteps(16);
  matcher.DRIVER.coolstep_min_speed(0xFFFFF); // 20bit max - needs to be set for stallguard
  matcher.DRIVER.diag1_stall(1);
  matcher.DRIVER.diag1_active_high(1);
  matcher.DRIVER.sg_stall_value(STALL_VALUE-2);
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

  // Setup for the ADF4351 frequency synthesizer
  adf4351.begin();

  adf4351.setrf(25000000U);
  adf4351.pwrlevel = 2; // This equals 2dBm*/ For the electrical probe coils one should use at least -20dbm so an attenuator is necessary
  adf4351.setf(START_FREQUENCY);

  // Setup for the RF Switch for the filterbank
  pinMode(FILTER_SWITCH_A, OUTPUT);
  pinMode(FILTER_SWITCH_B, OUTPUT);

  digitalWrite(FILTER_SWITCH_A, LOW);
  digitalWrite(FILTER_SWITCH_B, HIGH);

  // ADAC module
  adac.enable_internal_Vref();
  adac.set_DAC_max_2x_Vref();
  adac.set_ADC_max_2x_Vref();
  adac.configure_DACs(DACs);
  adac.write_DAC(VM, 0.0);
  adac.write_DAC(VT, 0.0);

  adac.configure_ADCs(ADCs);
}


void loop()
{
  // Serial communication via USB.
  // Commands:
  // f<start frequency>f<stop frequency>f<frequency step> - Frequency Sweep
  // d<target frequency in MHz>f<start frequency>f<stop frequency>f<frequency step> - Tune and Match
  // h - Homing
  // v<VM voltage in V>v<VT voltage in V> - Set Voltages
  // r<frequency in MHz> - Measure Reflection
  // s<start voltage in V>s<stop voltage in V>s<voltage step in V> - Voltage Sweep
  // c<filter identifier> - Control Switch for the filterbank 'p' stands for preamplifier and 'a' for automatic tuning and matching. 
  // m<stepper identifier><steps> - Move stepper motor. 't' for tuner and 'm' for matcher. Positive steps move the stepper away from the motor and negative steps move the stepper towards the motor.
  if (Serial.available())
  {
    String input_line = Serial.readStringUntil('\n'); // read string until newline character

    char command = input_line.charAt(0); // gets first character of input

    commandManager.executeCommand(command, input_line);
    commandManager.printCommandResult(command);
    /*
    Optimize matching call
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
    } */
  }
}

