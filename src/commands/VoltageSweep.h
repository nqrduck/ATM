#ifndef VOLTAGESWEEP_H
#define VOLTAGESWEEP_H

#include "Command.h"

/**
 * @brief This class is used to perform a voltage sweep. This means the return loss at a given frequency will be measured and the tuning and matching voltage with the lowest return loss will be found.
 */
class VoltageSweep : public Command
{
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;

private:
    void sweepVoltages(float_t voltage_step, float_t tuning_start, float_t tuning_stop, float_t matching_start, float_t matching_stop);
    void automaticSweep(uint32_t frequency);
    void presetVoltages(uint32_t frequency, float_t tuning_voltage, float_t matching_voltage);
    float_t matching_voltage;
    float_t tuning_voltage;
};

#endif