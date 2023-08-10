#ifndef TUNEMATCH_H
#define TUNEMATCH_H

#include "Command.h"

class TuneMatch : public Command {
public:
    void execute(String input_line) override;
    void printResult() override;
    void printHelp() override;
private:
    uint32_t automaticTM(uint32_t target_frequency, uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step);
    uint32_t resonance_frequency;

};

#endif