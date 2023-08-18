#include <math.h>
#include <Arduino.h>
#include "Debug.h"
#include "global.h"

/**
 * @brief This function finds the current resonance frequency of the coil. There should be a resonance already present or the algorithm might return nonsense.
 * It also returns the data of the frequency scan which can then be sent to the PC for plotting.
 *
 * @param start_frequency The frequency at which the search should start
 * @param stop_frequency The frequency at which the search should stop
 * @param frequency_step The frequency step size
 * @param print_data If true, the data will be printed to the serial monitor -> defaults to false
 * @return int32_t The current resonance frequency
 *
 * @example findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP); // finds the current resonance frequency
 */
int32_t findCurrentResonanceFrequency(uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step, boolean print_data = false);

/**
 * @brief This function sets the frequency of the frequency synthesizer and switches the filterbank accordingly.
 *
 * @param frequency The frequency that should be set
 * @return void
 *
 * @example setFrequency(100000000U); // sets the frequency to 100MHz
 */
void setFrequency(uint32_t frequency);

/**
 * @brief This function reads the reflection at the current frequency. It does not set the frequency.
 *
 * @param averages The number of readings that should be averaged
 * @return int The average reflection in millivolts
 *
 * @example readReflection(64); // reads the reflection at the current frequency and averages over 64 readings
 */
int readReflection(int averages);

/**
 * @brief This function reads the phase at the current frequency. It does not set the frequency.
 *
 * @param averages The number of readings that should be averaged
 * @return int The average phase in millivolts
 *
 * @example readPhase(64); // reads the phase at the current frequency and averages over 64 readings
 */
int readPhase(int averages);

/**
 * @brief This function sums up the reflection around a given frequency.
 *
 * @param center_frequency The frequency around which the reflection should be summed up
 * @return int The sum of the reflection around the given frequency
 *
 * @example sumReflectionAroundFrequency(100000000U);
 */
int sumReflectionAroundFrequency(uint32_t center_frequency);

/**
 * @brief This function tries out different capacitor positions until iteration depth is reached OR current_resonancy frequency matches the target_frequency.
 *
 * @param target_frequency The frequency that should be matched
 * @param current_resonance_frequency The current resonance frequency
 * @return int32_t The current resonance frequency
 *
 * @example bruteforceResonance(100000000U, 90000000U); // tries to match 100MHz with a current resonance frequency of 90MHz
 */
int32_t bruteforceResonance(uint32_t target_frequency, uint32_t current_resonance_frequency);

/**
 * @brief This function tries to find a matching capacitor position that will decrease the reflection at the current resonance frequency to a minimum.
 * It will then move the stepper to this position.
 *
 * @param current_resonance_frequency The current resonance frequency
 * @return int The reflection at the minimum matching position
 *
 * @example optimizeMatching(100000000); // tries to find the minimum reflection at 100 MHz and moves the stepper to this position
 */
int optimizeMatching(uint32_t current_resonance_frequency);

/**
 * @brief This function finds the direction which the matching capacitor should be turned to decrease the reflection.
 *
 * @param current_resonance_frequency The current resonance frequency
 * @return int The direction in which the matching capacitor should be turned. 1 for clockwise, -1 for anticlockwise
 *
 * @example getMatchRotation(100000000); // returns the direction in which the matching capacitor should be turned at 100MHz to decrease the reflection
 */
int getMatchRotation(uint32_t current_resonance_frequency);

/**
 * @brief This function controls the stepper so that they hit the limit and stall. It then returns the current position of the stepper.
 *
 * @param stepper The stepper that should be stalled
 * @return int The current position of the stepper
 *
 * @example stallStepper(tuner); // stalls the tuner stepper and returns the current position
 */
int stallStepper(Stepper stepper);

/**
 * @brief This function performs a homing of the steppers. It returns the current position of the stepper.
 *
 * @param stepper The stepper that should be homed
 * @return long The current position of the stepper
 *
 * @example homeStepper(tuner); // homes the tuner stepper and returns the current position
 */
long homeStepper(Stepper stepper);

/**
 * @brief This function checks if the input is valid. It checks if the frequency is within the allowed range.
 *
 * @param frequency_MHz The frequency that should be checked in MHz.
 * @return uint32_t The frequency in Hz if the input is valid, 0 otherwise.
 *
 * @example validateInput(100); // returns 100000000U
 */
uint32_t validateInput(float frequency_MHz);
/**
 * * @brief This method should be called when one wants to print information to the serial port.
 *
 * @param text The text that should be printed to the serial monitor. It should be a string
 * @return void
 *
 * @example printInfo("This is a test"); // prints "iThis is a test"
 */
void printInfo(String text);

/**
 * @brief This method should be called when one wants to print information to the serial port.
 *
 * @param number The number that should be printed to the serial monitor. It should be a number
 * @return void
 *
 * @example printInfo(123U); // prints "i123"
 */
void printInfo(uint32_t number);

/**
 * @brief This method should be called when one wants to an error to the serial port.
 *
 * @param text The text that should be printed to the serial monitor. It should be a string
 * @return void
 *
 * @example printError("Duck not found"); // prints "eDuck not found"
 */
void printError(String text);