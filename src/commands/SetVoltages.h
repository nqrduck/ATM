#ifndef SETVOLTAGES_H
#define SETVOLTAGES_H

#include "Command.h"

/**
 * @brief This class is used to set the voltages of the ADAC module
*/
class SetVoltages : public Command {
public:
    /**
     * @brief This function sets the voltages of the ADAC module
     * @param input_line The input line from the serial monitor. The syntax is v<VM voltage>v<VT voltage>.
    */
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
};

#endif