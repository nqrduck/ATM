#ifndef MOVESTEPPER_H
#define MOVESTEPPER_H

#include "Command.h"

/**
 * @brief This class is used to move either the tuning  or matching stepper motor
*/
class MoveStepper : public Command {
public:
    /**
     * @brief This function moves the stepper motor
     * @param input_line The input line from the serial monitor. 
    */
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
private:
    uint32_t resonance_frequency;
};

#endif