#ifndef CONTROLSWITCH_H
#define CONTROLSWITCH_H

#define PRE_AMP 'p'
#define ATM_SYSTEM 'a'

#include "Command.h"

/**
 * @brief This class is used to control the switch which is used to switch between the preamplifier and the atm system.
 * It can take the command <switch_state> where switch_state is either p or a for preamp or atm system respectively.
 */
class ControlSwitch : public Command
{
public:
    ControlSwitch();
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;

private:
    char switch_state;
    String result;
};

#endif