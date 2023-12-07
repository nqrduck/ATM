#include "Utilities.h"
#include "VoltageSweep.h"

void VoltageSweep::execute(String input_line)
{
    float MAX_VOLTAGE = 5.0;
    float MIN_VOLTAGE = 0.0;

    // First we get the frequency where the voltage sweep should be performed
    float frequency_MHz = input_line.substring(1).toFloat();
    uint32_t frequency = validateInput(frequency_MHz);
    if (frequency == 0)
    {
        printInfo("Invalid frequency input for voltage sweep.");
        return;
    }

    // First set the frequency 2 MHz below the target frequency
    uint32_t distance = 2000000;
    setFrequency(frequency - distance);

    // Then we perform a rough voltage sweep with a step size of 1V
    sweepVoltages(1, MIN_VOLTAGE, MAX_VOLTAGE, MIN_VOLTAGE, MAX_VOLTAGE);

    // Now we set the target frequency
    setFrequency(frequency);

    // Then we perform a more precise voltage sweep with a step size of 0.1V but only in the region of the found voltages
    float_t tuning_range = 0.5;
    sweepVoltages(0.05, tuning_voltage - tuning_range, tuning_voltage + tuning_range, MIN_VOLTAGE, MAX_VOLTAGE);

    // Then we perform a last very precise voltage sweep with a step size of 0.01V but only in the region of the found voltages
    tuning_range = 0.1;
    float_t matching_range = 0.2;
    sweepVoltages(0.01, tuning_voltage - tuning_range, tuning_voltage + tuning_range, matching_voltage - matching_range, matching_voltage + matching_range);

    // Finally we set the found voltages
    adac.write_DAC(VM, matching_voltage);
    adac.write_DAC(VT, tuning_voltage);
}

void VoltageSweep::sweepVoltages(float_t voltage_step, float_t tuning_start, float_t tuning_stop, float_t matching_start, float_t matching_stop)
{
    int AVERAGES = 4;
    // We want to maximize the reflection value, so we start with zero
    float_t minimum_reflection = 0.0;

    // This bruteforces the optimum voltage for tuning and matching.
    for (float_t c_tuning_voltage = tuning_start; c_tuning_voltage <= tuning_stop; c_tuning_voltage += voltage_step)
    {
        for (float_t c_matching_voltage = matching_start; c_matching_voltage <= matching_stop; c_matching_voltage += voltage_step)
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
                // float_t reflection_db = (reflection - 900) / 30.0;
                // if (reflection_db > 14)
                //    return;
            }
        }
    }
}

void VoltageSweep::printResult()
{
    // Print the results which are then read by the autotm module
    char identifier = 'v';
    char delimiter = 't';

    if (tuning_voltage < 0.0 || matching_voltage < 0.0)
    {
        tuning_voltage = 0.0;
        matching_voltage = 0.0;
    }
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