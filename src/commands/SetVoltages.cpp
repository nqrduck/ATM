#include "Utilities.h"
#include "SetVoltages.h"

void SetVoltages::execute(String input_line){
    char delimiter = 'v';
    int VMIndex = input_line.indexOf(delimiter) + 1;
    int VTIndex = input_line.indexOf(delimiter, VMIndex) + 1;

    tuning_voltage = input_line.substring(VMIndex, VTIndex - 1).toFloat();
    matching_voltage = input_line.substring(VTIndex).toFloat();

    adac.write_DAC(VM, tuning_voltage);
    adac.write_DAC(VT, matching_voltage);
}

void SetVoltages::printResult(){
    // Print the results which are then read by the autotm module
    char identifier = 'v';
    char delimiter = 't';

    String text = String(identifier) + String(matching_voltage) + String(delimiter) + String(tuning_voltage);

    Serial.println(text);

    printInfo("Voltages set to VM: " + String(matching_voltage) + " V and VT: " + String(tuning_voltage) + " V");
}

void SetVoltages::printHelp(){
    Serial.println("Set voltages command");
    Serial.println("Syntax: v<VM voltage>v<VT voltage>");
    Serial.println("Example: v0.5v0.5");
    Serial.println("This will set the VM and VT voltages to 0.5 V");
}
