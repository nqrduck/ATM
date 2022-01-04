#include <TMC2130Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>  

#include "ADF4351.h"

#define DEBUG 

#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
#endif

//SPI Pins
#define SCLK_PIN 14
#define MOSI_PIN 13
#define MISO_PIN 12

//ADF Pins
#define LE_PIN 27
#define CE_PIN 25

// Pins M1
#define EN_PIN_M1    25  
#define DIR_PIN_M1   33  
#define STEP_PIN_M1  32  
#define CS_PIN_M1    26  

#define DIAG1_PIN_M1 2 // used for homing 

// Pins M2
#define EN_PIN_M2    17  
#define DIR_PIN_M2   19  
#define STEP_PIN_M2  5  
#define CS_PIN_M2    18 

#define DIAG1_PIN_M2 4 // used for homing

// Stall Detection sensitivity
#define STALL_VALUE 16 // [-64..63]

//ADC Pin
#define REFLECTION_PIN 15

// Frequency Settings
#define FREQUENCY_STEP 100000U // 100kHz frequency steps for initial frequency sweep
#define START_FREQUENCY 80000000U // 80MHz 
#define STOP_FREQUENCY 160000000 // 120MHz

// Stepper Settings
#define STEPS_PER_ROTATION 3200U // 200 * 16 -> Microstepping
#define TUNING_STEPPER_HOME 30800U
#define MATCHING_STEPPER_HOME 32000U

ADF4351 adf4351(SCLK_PIN, MOSI_PIN, LE_PIN, CE_PIN); // declares object PLL of type ADF4351

TMC2130Stepper tuning_driver = TMC2130Stepper(EN_PIN_M1, DIR_PIN_M1, STEP_PIN_M1, CS_PIN_M1, MOSI_PIN, MISO_PIN, SCLK_PIN);
TMC2130Stepper matching_driver = TMC2130Stepper(EN_PIN_M2, DIR_PIN_M2, STEP_PIN_M2, CS_PIN_M2, MOSI_PIN, MISO_PIN, SCLK_PIN);

AccelStepper tuning_stepper = AccelStepper(tuning_stepper.DRIVER, STEP_PIN_M1, DIR_PIN_M1);
AccelStepper matching_stepper = AccelStepper(matching_stepper.DRIVER, STEP_PIN_M2, DIR_PIN_M2);

struct Stepper{
   AccelStepper STEPPER;
   TMC2130Stepper DRIVER;
   int STALL_PIN;
   int HOME_POSITION;
};

Stepper tuner = {tuning_stepper, tuning_driver, DIAG1_PIN_M1, TUNING_STEPPER_HOME};

Stepper matcher = {matching_stepper, matching_driver, DIAG1_PIN_M2, MATCHING_STEPPER_HOME};

void setup() {
  Serial.begin(115200);

  pinMode(MISO_PIN, INPUT_PULLUP); // Seems to be necessary for SPI to work

  tuner.DRIVER.begin();       // Initiate pins and registeries
  tuner.DRIVER.rms_current(400);   // Set stepper current to 800mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  tuner.DRIVER.microsteps(16);
  tuner.DRIVER.coolstep_min_speed(0xFFFFF); // 20bit max - needs to be set for stallguard 
  tuner.DRIVER.diag1_stall(1);
  tuner.DRIVER.diag1_active_high(1);
  tuner.DRIVER.sg_stall_value(STALL_VALUE);
  tuner.DRIVER.shaft_dir(0);
  tuner.DRIVER.stealthChop(1);   // Enable extremely quiet stepping

  pinMode(DIAG1_PIN_M1, INPUT);

  Serial.print("DRV_STATUS=0b");
  Serial.println(tuning_driver.DRV_STATUS(), BIN);

  matcher.DRIVER.begin();       // Initiate pins and registeries
  matcher.DRIVER.rms_current(200);   // Set stepper current to 800mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  matcher.DRIVER.microsteps(16);
  matcher.DRIVER.coolstep_min_speed(0xFFFFF); // 20bit max - needs to be set for stallguard 
  matcher.DRIVER.diag1_stall(1);
  matcher.DRIVER.diag1_active_high(1);
  matcher.DRIVER.sg_stall_value(STALL_VALUE);
  matcher.DRIVER.shaft_dir(0);
  matcher.DRIVER.stealthChop(1);   // Enable extremely quiet stepping
  
  digitalWrite(EN_PIN_M1, LOW);

  digitalWrite(EN_PIN_M2, LOW);

  tuner.STEPPER.setMaxSpeed(12000); 
  tuner.STEPPER.setAcceleration(12000); 
  tuner.STEPPER.setEnablePin(EN_PIN_M1);
  tuner.STEPPER.setPinsInverted(false, false, true);
  tuner.STEPPER.enableOutputs();

  matcher.STEPPER.setMaxSpeed(12000); 
  matcher.STEPPER.setAcceleration(12000); 
  matcher.STEPPER.setEnablePin(EN_PIN_M2);
  matcher.STEPPER.setPinsInverted(false, false, true);
  matcher.STEPPER.enableOutputs();
  
  tuner.STEPPER.setCurrentPosition(0);

  matcher.STEPPER.setCurrentPosition(0);

  adf4351.begin();

  adf4351.setrf(25000000U);
  adf4351.pwrlevel = 0; // This equals -4dBm*/
  adf4351.setf(START_FREQUENCY);

  ///////////////////// TESTING //////////////////////////// -> This works reliably
  /*unsigned long stime = millis();

  uint32_t target_frequency = 105000000U;
  Serial.println("_______________________________________________");
  Serial.println("Start - Target frequency is:");
  Serial.println(target_frequency);
  uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);
  Serial.println("Resonance is at:");
  Serial.println(resonance_frequency);

  resonance_frequency = approximateResonance(target_frequency, resonance_frequency);
  Serial.println("Resonance after approximation is at:");
  Serial.println(resonance_frequency);

  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
  Serial.println("Resonance after bruteforce is at:");
  Serial.println(resonance_frequency);

  /*int reflection = optimizeMatching(resonance_frequency);
  Serial.println("Minimum Reflection is:");
  Serial.println(reflection); 

  resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);
  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
  Serial.println("Resonance after bruteforce is at:");
  Serial.println(resonance_frequency);*/

  //Serial.println("Matched in s");
  //Serial.println((millis()-stime)/ 1000);

  
  
}

// Implement Serial communication ...
// This could probably cleaned up by using structs for the inputs, pointing to the different functions- > would reduce copy-paste code and make adding functions more intuitive
void loop() {
  if (Serial.available()) { 
    
    String input_line = Serial.readStringUntil('\n'); // read string until newline character

    char command = input_line.charAt(0); // gets first character of input

    // approximate call
    // CAREFULL -> if the coil has no proper matching in the frequency range this will not work! Only use this for testing -> otherwise use the automated 'decide' call. 
    if (command == 'a') {
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0) return;
      
      Serial.println("Approximating matching to target frequency in MHz:");
      Serial.println(target_frequency_MHz);

      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);
      resonance_frequency = approximateResonance(target_frequency, resonance_frequency);
      Serial.println("Resonance after approximation is at:");
      Serial.println(resonance_frequency);
      

    // bruteforce call 
    // CAREFULL -> if the current resonance frequency is not within +-5MHz of the target frequency this will not work. Only use this for testing -> otherwise use the automated 'decide' call. 
    } else if (command == 'b') {
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0) return;
      
      Serial.println("Bruteforce matching to target frequency in MHz:");
      Serial.println(target_frequency_MHz);

      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
      Serial.println("Resonance after bruteforce is at:");
      Serial.println(resonance_frequency);
      
      

    // decide call
    // this function decides what kind of t&m mode should be used based on the relationship between target frequency and current resonance
    // it also makes sure that there a homing routine performed in case there is currently no proper resonance in the frequency range
    } else if (command == 'd'){
      float target_frequency_MHz = input_line.substring(1).toFloat();
      uint32_t target_frequency = validateInput(target_frequency_MHz);
      if (target_frequency == 0) return;

      Serial.println("Tuning and Matching to target frequency in MHz (automatic mode):");
      Serial.println(target_frequency_MHz);

      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      int32_t delta_frequency = target_frequency - resonance_frequency; // needs to be int -> negative frequencies possible
      if (abs(delta_frequency) > 5000000U) resonance_frequency = approximateResonance(target_frequency, resonance_frequency);

      resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);
      
      Serial.println("Resonance after tuning and matching is at:");
      Serial.println(resonance_frequency);

    // calibration call
    // Perform the homing routine by looking for the limit of the capacitors
    // it also places the steppers in a way so there is a resonance dip inside the frequency range
    // CAREFULL -> The values are hardcoded, these need to be changed if there is a different coil in use
    } else if (command == 'c'){
      Serial.println("Homing...");
      homeStepper(tuner);
      homeStepper(matcher);
      Serial.println("Resonance frequency after homing:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);
      Serial.println(resonance_frequency);
      
    // frequency sweep call
    // scans the frequency range for the current resonance frequency
    } else if (command == 'f'){
      Serial.println("Frequency sweep...");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);
      Serial.println("Resonance is at:");
      Serial.println(resonance_frequency);

    // calculates Reflection loss for a given frequency
    } else if (command == 'r'){
      float frequency_MHz = input_line.substring(1).toFloat();
      uint32_t frequency = validateInput(frequency_MHz);
      if (frequency == 0) return;

      float reflection_loss = getReflectionRMS(frequency);
      Serial.println("For frequency:");
      Serial.println(frequency);
      Serial.println("RMS of the reflection is:");
      Serial.println(reflection_loss);

    //optimize Matching 
    } else if (command == 'm'){
      Serial.println("Optimize Matching around frequency:");
      uint32_t resonance_frequency = findCurrentResonanceFrequency(START_FREQUENCY, STOP_FREQUENCY, FREQUENCY_STEP);

      Serial.println(resonance_frequency);

      optimizeMatching(resonance_frequency);
   
    } else {     
      Serial.println("Invalid Input");
    }
    
  }

}

// This helper function checks if the input frequency is plausible, if so it returns the value in Hz
// otherwise it returns 0
uint32_t validateInput(float frequency_MHz){
  uint32_t frequency_Hz = frequency_MHz * 1000000U;

  if (frequency_Hz < START_FREQUENCY){
    Serial.println("Invalid input: frequency too low");
    return 0;
  } else if (frequency_Hz > STOP_FREQUENCY) {
    Serial.println("Invalid input: frequency too high");
    return 0;    
  }else{
    return frequency_Hz;
  }
  
}

int readReflection(int averages){
  int reflection = 0;
  for (int i = 0; i < averages; i ++) reflection += analogReadMilliVolts(REFLECTION_PIN);
  return reflection/averages;
  
}

//should put the stepper stuff into a struct
void homeStepper(Stepper stepper){ 
  stallStepper(stepper);

  /*tuning_stepper.setMaxSpeed(3000); 
  tuning_stepper.setAcceleration(3000); 

  delay(500);
  
  stallStepper(tuning_stepper);

  tuning_stepper.setMaxSpeed(12000); 
  tuning_stepper.setAcceleration(12000); */

  stepper.STEPPER.setCurrentPosition(0);
  stepper.STEPPER.moveTo(stepper.HOME_POSITION);
  stepper.STEPPER.runToPosition();
  
}

void stallStepper(Stepper stepper){
  stepper.STEPPER.moveTo(-9999999);
  
  while (!digitalRead(stepper.STALL_PIN)){
    stepper.STEPPER.run();
  }

  stepper.STEPPER.stop();
}

// calculates the Reflection Loss at a specified frequency
//24mV/dB slope
//0dBV defined as 1V Sin RMS
// Would expect 1.74V as output for unmatched coil -> but it's 1.65V => ~10mV at Logamp

// Measurments: with 40dB LNA @85MHz
// Open: 1.6V RMS output 
// Coil matched to -30dB: 1.0V RmS Output
float calculateRL(uint32_t frequency){
  float RMS_ADF = 13; // at -4dBm the ADF4351 generates sin with an RMS value of 131.5mV but due to to -10dB attenuation of the transcoupler and some additional reflections about 13mV are effectivly at the Logamp
  float reflection_rms = getReflectionRMS(frequency);

  float reflection_loss = 20 * log10((reflection_rms)/ RMS_ADF);

  return reflection_loss;
  
}

float getReflectionRMS(uint32_t frequency){
  float LOGAMP_SLOPE = 24; // Slope in mV/dB
  
  adf4351.setf(frequency);
  delay(10);

  int reflection_mv = readReflection(64); // Output of the logamp

  int intercept_positioning = -108; // in dB  

  float reflection_dBV = (reflection_mv/LOGAMP_SLOPE) + intercept_positioning;

  float reflection_rms = pow(10, reflection_dBV / 20) * 1000; // this step could be shortened but I still like to calculate it explicitly since there are multiple logarithmic operations going on here - > this value is in mV
  return reflection_rms;
}

// Finds current Resonance Frequency of the coil. There should be a substential dip already present atm. 
// Add plausibility check to make sure there is one peak at at least -12dB
// Following is for setup WITHOUT 20dB LNA:
// -30dB aprox. 1.15V Oscilloscope -> normally 1.6V -> 1300 Points
// -16dB aprox. 1.27V Oscilloscope - normally 1.6V
// -18dB aprox 1.295V Oscilloscope -> use 1489 Points as decision line for sufficient Matching

// Values for setup WITH 20dB LNA: -> i don't know what happened here.
// open 1.2V
//-16dB  827mV Oscilloscope
//-30dB 770mV Oscilloscope 


int32_t findCurrentResonanceFrequency(uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step){
    int minimum_reflection = 4096;
    int current_reflection = 0;
    uint32_t minimum_frequency = 0;
    float reflection_rms = 0;

    adf4351.setf(start_frequency); // A frequency value needs to be set once -> there seems to be a bug with the first SPI call
  
    for(uint32_t frequency = start_frequency; frequency <= stop_frequency; frequency += frequency_step){
      adf4351.setf(frequency);
      delay(5); // This delay is essential! There is a glitch with ADC2 that leads to wrong readings if GPIO27 is set to high for multiple microseconds.
      
      current_reflection = readReflection(4);

      if (current_reflection < minimum_reflection){
        minimum_frequency = frequency;
        minimum_reflection = current_reflection;
      }
       
    }

    reflection_rms = getReflectionRMS(minimum_frequency); 
    if (reflection_rms > 10){
      Serial.println("Resonance could not be found.");
      Serial.println(reflection_rms);
      return -1;
    }
    
    return minimum_frequency;
}


// Approximates the target frequency to about 3 MHZ with the tuning capacitor .... works so far
int32_t approximateResonance(uint32_t target_frequency, uint32_t current_resonance_frequency){

  int32_t delta_frequency = target_frequency - current_resonance_frequency; 
  int rotation = 0; // rotation == 1 -> clockwise, rotation == -1 -> counterclockwise

  if (delta_frequency < 0) rotation = -1; // negative delta means currentresonance is to high, hence anticlockwise movement is necessary
  else rotation = 1;

  int start_position = tuner.STEPPER.currentPosition();

  tuner.STEPPER.move(STEPS_PER_ROTATION * rotation); // This needs to be changed
  tuner.STEPPER.runToPosition();

  // @ Optimization possibility: -> just scan plausible area, would reduce half the scan time
  int32_t one_revolution_resonance = findCurrentResonanceFrequency(current_resonance_frequency - 30000000U, current_resonance_frequency + 30000000U, FREQUENCY_STEP); 
  
  DEBUG_PRINT(one_revolution_resonance);

  int32_t delta_one_revolution_frequency = one_revolution_resonance - current_resonance_frequency;

  //Plausibility Check - prevents the stepper from turning forever. 
  if ((one_revolution_resonance == -1) || (abs(delta_one_revolution_frequency) > 30000000U)){
    Serial.println("Tuning and matching not possible - homing needed.");
    return -1;
  }
  

  int32_t steps_to_delta_frequency = ((float) delta_frequency / (float) delta_one_revolution_frequency) * STEPS_PER_ROTATION * rotation;

  DEBUG_PRINT(delta_one_revolution_frequency);
  DEBUG_PRINT(delta_frequency);
  
  DEBUG_PRINT(tuner.STEPPER.currentPosition());
  DEBUG_PRINT(steps_to_delta_frequency);

  tuner.STEPPER.moveTo(start_position + steps_to_delta_frequency);
  tuner.STEPPER.runToPosition();

  DEBUG_PRINT(tuner.STEPPER.currentPosition());

  current_resonance_frequency = findCurrentResonanceFrequency(target_frequency - 5000000U, target_frequency + 5000000U, FREQUENCY_STEP);
  return(current_resonance_frequency);
    
}


// Tries out different capacitor position until iteration depth is reached OR current_resonancy frequency matches the target_frequency
int32_t bruteforceResonance(uint32_t target_frequency, uint32_t current_resonance_frequency){
  // Change Tuning Stepper -> Clockwise => Freq goes up
  // Dir = 0 => Anticlockwise movement
  int rotation = 0; // rotation == 1 -> clockwise, rotation == -1 -> counterclockwise
  
  int ITERATIONS = 25; // Iteration depth
  int iteration_steps = 0;
  
  int32_t delta_frequency = target_frequency - current_resonance_frequency; 

  if (delta_frequency < 0) rotation = -1; // negative delta means currentresonance is too high, hence anticlockwise movement is necessary
  else rotation = 1;

  iteration_steps = rotation * (STEPS_PER_ROTATION / 50);

  //'bruteforce' the stepper position to match the target frequency
  
  for (int i = 0; i < ITERATIONS; i ++){  
    tuner.STEPPER.move(iteration_steps);
    tuner.STEPPER.runToPosition();

     // @ Optimization possibility: Reduce frequency range when close to target_frequency
     current_resonance_frequency = findCurrentResonanceFrequency(target_frequency - 5000000U, target_frequency + 5000000U, FREQUENCY_STEP / 2);
    
     //Serial.println(current_resonance_frequency);
     //Serial.println(rotation);
     //Serial.println(iteration_steps);

     // Stops the iteration if the minima matches the target frequency
     if (current_resonance_frequency == target_frequency) break;

     // This means the bruteforce resolution was too high and the resonance frequency overshoot
     // therfore the turn direction gets inverted and the increment halfed
     if ((current_resonance_frequency > target_frequency) && (rotation == 1)) { 
      rotation = -1;
      iteration_steps /= 2;
      iteration_steps *= rotation;
     }
     else if ((current_resonance_frequency < target_frequency) && (rotation == -1)){
      rotation = 1;
      iteration_steps /= 2;
      iteration_steps *= -rotation;
     }
     
  }
  
  return current_resonance_frequency; 
  
}

// 
// Matcher clockwise lowers resonance frequency

int optimizeMatching(uint32_t current_resonance_frequency){
  float minimum_reflection = 4096;
  float current_reflection = 0;
  int minimum_matching_position = 0; 
  float last_reflection = 4096;
  int rotation = 1;

  int ITERATIONS = 25; // //100 equals one full rotation
  int iteration_steps = 0;

  // Look which rotation direction improves matching. 
  

  // This tries to find the minimum reflection while ignoring the change in resonance -> it always looks for minima within 
  iteration_steps = rotation * STEPS_PER_ROTATION / 2;

  adf4351.setf(current_resonance_frequency);
  for (int i = 0; i < ITERATIONS; i ++){
    DEBUG_PRINT("Iteration");
    
    matcher.STEPPER.move(iteration_steps);
    matcher.STEPPER.runToPosition();

    delay(250);

    current_resonance_frequency = findCurrentResonanceFrequency(current_resonance_frequency - 1000000U, current_resonance_frequency + 1000000U, FREQUENCY_STEP / 10);
    
    adf4351.setf(current_resonance_frequency);
    delay(100);
    
    current_reflection = readReflection(64);;     

    if (current_reflection < minimum_reflection){
        minimum_matching_position = matcher.STEPPER.currentPosition();
        minimum_reflection = current_reflection;
        DEBUG_PRINT("Minimum");
        DEBUG_PRINT(minimum_matching_position);
     }
    
    
    if (current_reflection > last_reflection) {
      rotation *= -1;
      iteration_steps /= 2;
      iteration_steps *= rotation;
    }

    DEBUG_PRINT(matcher.STEPPER.currentPosition());
    DEBUG_PRINT(current_resonance_frequency);
    DEBUG_PRINT(last_reflection);
    
    last_reflection = current_reflection;
    
    if(iteration_steps == 0) break;   

    
    DEBUG_PRINT(current_reflection);
    
  }
  
  matcher.STEPPER.moveTo(minimum_matching_position);
  matcher.STEPPER.runToPosition();

  DEBUG_PRINT(matcher.STEPPER.currentPosition());
  
  return (minimum_reflection);
}
