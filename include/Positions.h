// Position @ 40, 60, 80, 100, 120, 140, 160, 180, 200, 220 MHzstruct Filter
struct Filter
{
  uint32_t fg;
  int control_input_a;
  int control_input_b;
};

struct FrequencyRange
{
  uint32_t START_FREQUENCY;
  uint32_t STOP_FREQUENCY;
  uint32_t CENTER_FREQUENCY;
  Filter FILTER;
  uint32_t TUNING_CENTER_POSITION;
  uint32_t MATCHING_CENTER_POSITION;
};

const Filter FG_71MHZ = {71000000U, HIGH, HIGH};
const Filter FG_120MHZ = {120000000U, LOW, HIGH};
const Filter FG_180MHZ = {180000000U, LOW, LOW};
const Filter FG_260MHZ = {260000000U, HIGH, LOW};

// Settings for 100MHz -18dB
//#define TUNING_STEPPER_HOME 34250U
//#define MATCHING_STEPPER_HOME 45000U
const FrequencyRange RANGE_35_70MHZ =
    {
        35000000U,
        75000000U,
        55000000U,
        FG_71MHZ,
        34250U, // FIND VALUES
        45000U, // FIND VALUES
};

const FrequencyRange RANGE_70_125MHZ =
    {
        70000000U,
        125000000U,
        100000000U,
        FG_120MHZ,
        34250U, // FIND VALUES
        45000U, // FIND VALUES
};

const FrequencyRange RANGE_125_180MHZ =
    {
        125000000U,
        180000000U,
        150000000U,
        FG_180MHZ,
        34250U, // FIND VALUES
        45000U, // FIND VALUES
};

const FrequencyRange HOME_RANGE = RANGE_70_125MHZ;

// Settings for 125MHz -30dB
//#define TUNING_STEPPER_HOME 37550U
//#define MATCHING_STEPPER_HOME 29500U