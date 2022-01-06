// Position @ 50, 75, 100, 125, 150, 175, 200 MHz
struct FREQUENCY_POSITION {
  uint32_t FREQUENCY;
  uint32_t TUNING_POSITION;
  uint32_t MATCHING_POSITION;
};

// Settings for 100MHz -18dB
#define TUNING_STEPPER_HOME 34100U
#define MATCHING_STEPPER_HOME 32000U

// Settings for 125MHz -30dB
//#define TUNING_STEPPER_HOME 37550U
//#define MATCHING_STEPPER_HOME 29500U