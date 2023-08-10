#ifndef FREQUENCYSWEEP_H
#define FREQUENCYSWEEP_H

#include "Command.h"

/**
 * @brief This class is used to perform a frequency sweep
*/
class FrequencySweep : public Command {
public:
    void execute(String input_lne) override;
    void printResult() override;
    void printHelp() override;
private:
    uint32_t resonance_frequency;
};

#endif