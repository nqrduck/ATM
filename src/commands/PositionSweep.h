#ifndef POSITIONSWEEP_H
#define POSITIONSWEEP_H

#include "Command.h"

/**
 * @brief This class is used to perform a position sweep. This means the return loss at a given frequency will be measured and the tuning and matching positions with the lowest return loss will be found.
 */
class PositionSweep : public Command
{
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;

private:
    void sweepPositions(uint32_t tuning_range, uint32_t tuning_step, uint32_t tuning_backlash, uint32_t matching_range, uint32_t matching_step, uint32_t matching_backlash);
    int absolute_move_backlashcorrected(Stepper stepper, uint32_t position, int32_t backlash);
    uint32_t tuning_position;
    uint32_t matching_position;
    int matching_last_direction = 0;
    int tuning_last_direction = 0;
};

#endif