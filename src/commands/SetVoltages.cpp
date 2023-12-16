#include "Utilities.h"
#include "SetVoltages.h"

void SetVoltages::execute(String input_line){
    // Format is v<VT voltage>v<VM voltage>
    // Example: v0.5v0.5
    // This will set the VM and VT voltages to 0.5 V
    char delimiter = 'v';
    // remove first character
    input_line = input_line.substring(1);
    int delimiter_index = input_line.indexOf(delimiter);
    if (delimiter_index == -1){
        printInfo("Invalid input for set voltages command.");
        return;
    }

    String tuning_voltage_str = input_line.substring(0, delimiter_index);
    String matching_voltage_str = input_line.substring(delimiter_index + 1);

    tuning_voltage = tuning_voltage_str.toFloat();
    matching_voltage = matching_voltage_str.toFloat();

    adac.write_DAC(VT, tuning_voltage);
    adac.write_DAC(VM, matching_voltage);
}

void SetVoltages::printResult(){
    // Print the results which are then read by the autotm module
    char identifier = 'v';
    char delimiter = 't';

    String text = String(identifier) + String(tuning_voltage) + String(delimiter) + String(matching_voltage);

    Serial.println(text);
}

void SetVoltages::printHelp(){
    Serial.println("Set voltages command");
    Serial.println("Syntax: v<VT voltage>v<VM voltage>");
    Serial.println("Example: v0.5v0.5");
    Serial.println("This will set the VM and VT voltages to 0.5 V");
}
