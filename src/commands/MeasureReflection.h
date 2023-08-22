#ifndef MEASUREREFLECTION_H
#define MEASUREREFLECTION_H

#include "Command.h"

/**
 * @brief This class is used to measure the  reflection at  a given frequency
 */
class MeasureReflection : public Command
{
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;

private:
    int return_loss;
    int phase;
};

#endif