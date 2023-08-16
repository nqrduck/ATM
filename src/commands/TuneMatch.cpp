#include "Utilities.h"
#include "TuneMatch.h"

void TuneMatch::execute(String input_line)
{
  float target_frequency_MHz = input_line.substring(1).toFloat();
  uint32_t target_frequency = validateInput(target_frequency_MHz);
  if (target_frequency == 0)
    return;

  uint32_t startf = 35000000U;
  uint32_t stopf = 110000000U;
  uint32_t stepf = 100000U;
  printInfo("Tuning and Matching to target frequency in MHz (automatic mode):");
  printInfo(target_frequency_MHz);

  uint32_t resonance_frequency = automaticTM(target_frequency, startf, stopf, stepf);
}

uint32_t TuneMatch::automaticTM(uint32_t target_frequency, uint32_t start_frequency, uint32_t stop_frequency, uint32_t frequency_step)
{
  uint32_t resonance_frequency = findCurrentResonanceFrequency(start_frequency, stop_frequency, frequency_step);

  DEBUG_PRINT("Resonance Frequency before TM");
  DEBUG_PRINT(resonance_frequency);

  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);

  optimizeMatching(resonance_frequency);

  resonance_frequency = findCurrentResonanceFrequency(resonance_frequency - 1000000U, resonance_frequency + 1000000U, frequency_step / 2);

  resonance_frequency = bruteforceResonance(target_frequency, resonance_frequency);

  return resonance_frequency;
}

void TuneMatch::printResult()
{
  Serial.print("r");
  printInfo(resonance_frequency);
}

void TuneMatch::printHelp()
{
  Serial.println("Tune and Match command");
  Serial.println("Syntax: d<target frequency in MHz>");
  Serial.println("Example: d100");
  Serial.println("This will tune and match to 100 MHz");
}