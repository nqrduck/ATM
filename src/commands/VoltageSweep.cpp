#include "Utilities.h"
#include "VoltageSweep.h"

void VoltageSweep::execute(String input_line)
{
    // Command format is s<frequency in MHz>o<optional tuning voltage>o<optional matching voltage>


    // First we get the frequency where the voltage sweep should be performed
    char identifier = 's';
    char delimiter = 'o';
    int frequencyIndex = input_line.indexOf(identifier) + 1;

    // set frequency once to 100 MHz
    setFrequency(100000000);

    // Check if optional voltages are given
    if (input_line.indexOf(delimiter, frequencyIndex) == -1)
    {
        // If no optional voltages are given, we perform an automatic voltage sweep
        String frequency_MHz = input_line.substring(frequencyIndex);
        uint32_t frequency = validateInput(frequency_MHz.toFloat());
        if (frequency == 0)
        {
            printInfo("Invalid frequency input for voltage sweep.");
            return;
        }
        printInfo("Starting automatic voltage sweep at " + String(frequency) + " MHz");

        automaticSweep(frequency);
        return;
    }
    else
    {
        // If optional voltages are given, we perform a voltage sweep with the given voltages
        int tuningIndex = input_line.indexOf(delimiter, frequencyIndex) + 1;
        int matchingIndex = input_line.indexOf(delimiter, tuningIndex) + 1;

        float frequency_MHz = input_line.substring(frequencyIndex, tuningIndex - 1).toFloat();
        float tuning_voltage_predefined = input_line.substring(tuningIndex, matchingIndex - 1).toFloat();
        float matching_voltage_predefined = input_line.substring(matchingIndex).toFloat();

        uint32_t frequency = validateInput(frequency_MHz);
        if (frequency == 0)
        {
            printInfo("Invalid frequency input for voltage sweep.");
            return;
        }
        printInfo("Starting preset voltage sweep at " + String(frequency) + " MHz" + " with tuning voltage " + String(tuning_voltage_predefined) + " V and matching voltage " + String(matching_voltage_predefined) + " V");
        presetVoltages(frequency, tuning_voltage_predefined, matching_voltage_predefined);
    }
}

void VoltageSweep::presetVoltages(uint32_t frequency, float_t tuning_voltage_predefined, float_t matching_voltage_predefined)
{
    // For preset voltages we only scan the tuning and matching voltages in a reduced range. This is because the voltages are already close to the optimum values.
    float_t TUNING_RANGE = 0.2;
    float_t MATCHING_RANGE = 0.2;
    float_t voltage_step = 0.01;

    setFrequency(frequency);

    float_t tuning_voltage_start = tuning_voltage_predefined - TUNING_RANGE;
    if (tuning_voltage_start < 0.0)
        tuning_voltage_start = 0.0;

    float_t tuning_voltage_stop = tuning_voltage_predefined + TUNING_RANGE;
    if (tuning_voltage_stop > 5.0)
        tuning_voltage_stop = 5.0;

    printInfo("Tuning voltage start: " + String(tuning_voltage_start));

    float_t matching_voltage_start = matching_voltage_predefined - MATCHING_RANGE;
    if (matching_voltage_start < 0.0)
        matching_voltage_start = 0.0;

    float_t matching_voltage_stop = matching_voltage_predefined + MATCHING_RANGE;
    if (matching_voltage_stop > 5.0)
        matching_voltage_stop = 5.0;

    //  We use the sweepVoltages function to find the optimum tuning and matching voltages
    sweepVoltages(voltage_step, tuning_voltage_start, tuning_voltage_stop, matching_voltage_start, matching_voltage_stop);

    adac.write_DAC(VM, matching_voltage);
    adac.write_DAC(VT, tuning_voltage);
}

void VoltageSweep::automaticSweep(uint32_t frequency)
{
    float MAX_VOLTAGE = 5.0;
    float MIN_VOLTAGE = 0.0;

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

    if (tuning_voltage < 0.0 || matching_voltage < 0.0)
    {
        tuning_voltage = 0.0;
        matching_voltage = 0.0;
        printError("No valid voltages found for automatic voltage sweep.");
    }

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
    for (float c_tuning_voltage = tuning_start; c_tuning_voltage <= tuning_stop; c_tuning_voltage += voltage_step)
    {
        adac.write_DAC(VT, c_tuning_voltage);
        for (float c_matching_voltage = matching_start; c_matching_voltage <= matching_stop; c_matching_voltage += voltage_step)
        {
            // Set the tuning and matching voltage
            adac.write_DAC(VM, c_matching_voltage);
            
            // Measure the reflection at the given frequency
            int reflection = readReflection(AVERAGES);

            // If the reflection is lower than the current minimum, we have found a new minimum
            if (reflection > minimum_reflection)
            {
                minimum_reflection = reflection;
                tuning_voltage = c_tuning_voltage;
                matching_voltage = c_matching_voltage;
            }
        }
    }
}

void VoltageSweep::printResult()
{
    // Print the results which are then read by the autotm module
    char identifier = 'v';
    char delimiter = 't';

    String text = String(identifier) + String(tuning_voltage) + String(delimiter) + String(matching_voltage);

    Serial.println(text);
}

void VoltageSweep::printHelp()
{
    Serial.println("Voltage sweep command");
    Serial.println("Syntax: v<frequency in MHz>");
    Serial.println("Example: v100");
    Serial.println("This will perform a voltage sweep at 100 MHz and will return the optimum tuning and matching voltages for this frequency.");
}