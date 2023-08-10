#ifndef HOMING_H
#define HOMING_H

#include "Command.h"

class Homing : public Command {
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
private:
    uint32_t resonance_frequency;
};

#endif