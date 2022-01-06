// Stepper Settings
#define MICROSTEPS 16
#define STEPS_PER_ROTATION 200U * MICROSTEPS // 1.8° per step

#define MAXSPEED 12000U
#define ACCELERATION 12000U

// Stall Detection sensitivity
#define STALL_VALUE 16 // [-64..63]

struct Stepper{
   AccelStepper STEPPER;
   TMC2130Stepper DRIVER;
   int STALL_PIN;
   int HOME_POSITION;
};
