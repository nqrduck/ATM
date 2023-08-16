#include "Utilities.h"
#include "VoltageSweep.h"

void VoltageSweep::execute(String input_line)
{
    int AVERAGES = 4;
    float VOLTAGE_STEP = 0.1;
    float MAX_VOLTAGE = 5.0;

    // First we get the frequency where the voltage sweep should be performed
    float frequency_MHz = input_line.substring(1).toFloat();
    uint32_t frequency = validateInput(frequency_MHz);
    if (frequency == 0)
    {
        printInfo("Invalid frequency input for voltage sweep.");
        return;
    }

    // First set the frequency
    setFrequency(frequency);

    // Now we can perform the voltage sweep
    printInfo("Started voltage sweep");

    // We want to maximize the reflection value, so we start with zero
    float_t minimum_reflection = 0.0;

    // This bruteforces the optimum voltage for tuning and matching.
    for (float_t c_tuning_voltage = 0.0; c_tuning_voltage <= MAX_VOLTAGE; c_tuning_voltage += VOLTAGE_STEP)
    {
        for (float_t c_matching_voltage = 0.0; c_matching_voltage <= MAX_VOLTAGE; c_matching_voltage += VOLTAGE_STEP)
        {
            // Set the tuning and matching voltage
            adac.write_DAC(VM, c_matching_voltage);
            adac.write_DAC(VT, c_tuning_voltage);

            // Measure the reflection at the given frequency
            int reflection = readReflection(AVERAGES);

            // If the reflection is lower than the current minimum, we have found a new minimum
            if (reflection > minimum_reflection)
            {
                minimum_reflection = reflection;
                tuning_voltage = c_tuning_voltage;
                matching_voltage = c_matching_voltage;
                // If the returnloss is better than 14dB, we can stop the voltage sweep
                float_t reflection_db = (reflection - 900) / 30.0;
                if (reflection_db > 14)
                    return;
            }
        }
    }
}

void VoltageSweep::printResult()
{
    // Print the results which are then read by the autotm module
    char identifier = 'v';
    char delimiter = 't';
    String text = String(identifier) + String(matching_voltage) + String(delimiter) + String(tuning_voltage);

    Serial.println(text);
}

void VoltageSweep::printHelp()
{
    Serial.println("Voltage sweep command");
    Serial.println("Syntax: v<frequency in MHz>");
    Serial.println("Example: v100");
    Serial.println("This will perform a voltage sweep at 100 MHz and will return the optimum tuning and matching voltages for this frequency.");
}