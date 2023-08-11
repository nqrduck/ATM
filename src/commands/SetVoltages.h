#ifndef SETVOLTAGES_H
#define SETVOLTAGES_H

#include "Command.h"

/**
 * @brief This class is used to set the voltages of the ADAC module
*/
class SetVoltages : public Command {
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
};

#endif