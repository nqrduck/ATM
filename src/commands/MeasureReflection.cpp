#include "Utilities.h"
#include "MeasureReflection.h"

void MeasureReflection::execute(String input_line)
{
    int AVERAGES = 16;
    return_loss = 0;
    phase = 0;

    printInfo("Measure Reflection");
    // Get the float after the r character which is the frequency value where the reflection measurment should be performed
    float frequency_MHz = input_line.substring(1).toFloat();
    uint32_t frequency = validateInput(frequency_MHz);
    if (frequency == 0)
    {
        printInfo("Invalid frequency input.");
        return;
    }

    // First set the frequency
    setFrequency(frequency);

    // Measure the reflection at the given frequency
    return_loss = readReflection(AVERAGES);
    phase = readPhase(AVERAGES);
}

void MeasureReflection::printResult()
{
    // Print the results which are then read by the autotm module
    char identifier = 'r';
    char delimiter = 'p';
    String text = String(identifier) + String(return_loss) + String(delimiter) + String(phase);

    Serial.println(text);
}

void MeasureReflection::printHelp()
{
    Serial.println("Measure reflection command");
    Serial.println("Syntax: r<frequency in MHz>");
    Serial.println("Example: r100");
    Serial.println("This will measure the reflection at 100 MHz");
}