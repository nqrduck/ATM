#ifndef FREQUENCYSWEEP_H
#define FREQUENCYSWEEP_H

#include "Command.h"

/**
 * @brief This class is used to perform a frequency sweep
*/
class FrequencySweep : public Command {
public:
    /**
     * @brief This function performs a frequency sweep
     * @param input_line The input line from the serial monitor. The syntax is f<start frequency>f<stop frequency>f<frequency step>.
    */
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
private:
    uint32_t resonance_frequency;
};

#endif