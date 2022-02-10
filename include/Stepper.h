// Stepper Settings
#define STEPS_PER_ROTATION 3200U // 200 * 16 -> Microstepping

// Stall Detection sensitivity
#define STALL_VALUE 16 // [-64..63]

struct Stepper{
   AccelStepper STEPPER;
   TMC2130Stepper DRIVER;
   int STALL_PIN;
   String TYPE;
};
