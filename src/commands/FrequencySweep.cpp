#include "Utilities.h"
#include "FrequencySweep.h"

void FrequencySweep::execute(String input_line)
{
    printInfo("Started frequency sweep");
    // Get the start frequency which is the value until the next f character
    char delimiter = 'f';
    // Indices for each variable
    int startFreqIndex = input_line.indexOf(delimiter) + 1;
    int stopFreqIndex = input_line.indexOf(delimiter, startFreqIndex) + 1;
    int freqStepIndex = input_line.indexOf(delimiter, stopFreqIndex) + 1;

    // Extract each variable from the string
    uint32_t startFreq = input_line.substring(startFreqIndex, stopFreqIndex - 1).toInt(); // Subtract 1 to not include the delimiter
    uint32_t stopFreq = input_line.substring(stopFreqIndex, freqStepIndex - 1).toInt();
    uint32_t freqStep = input_line.substring(freqStepIndex).toInt(); // If no second parameter is provided, substring() goes to the end of the string

    // The find current resonance frequency also prints prints the S11 data to the serial monitor
    resonance_frequency = findCurrentResonanceFrequency(startFreq, stopFreq, freqStep, true);
}

void FrequencySweep::printResult()
{
    // This tells the PC that the frequency sweep is finished
    Serial.print("r");
    printInfo(resonance_frequency);
}

void FrequencySweep::printHelp()
{
    Serial.println("Frequency sweep command");
    Serial.println("Syntax: f<start frequency>f<stop frequency>f<frequency step>");
    Serial.println("Example: f100000000f200000000f50000");
    Serial.println("This will sweep the frequency from 100 MHz to 200 MHz with a step of 50 kHz");
}
