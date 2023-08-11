#include "Utilities.h"
#include "SetVoltages.h"

void SetVoltages::execute(String input_line){
    char delimiter = 'v';
    int VMIndex = input_line.indexOf(delimiter) + 1;
    int VTIndex = input_line.indexOf(delimiter, VMIndex) + 1;

    float VMVoltage = input_line.substring(VMIndex, VTIndex - 1).toFloat();
    float VTVoltage = input_line.substring(VTIndex).toFloat();

    adac.write_DAC(VM, VMVoltage);
    adac.write_DAC(VT, VTVoltage);
}

void SetVoltages::printResult(){
    Serial.println("Voltages set");
}

void SetVoltages::printHelp(){
    Serial.println("Set voltages command");
    Serial.println("Syntax: v<VM voltage>v<VT voltage>");
    Serial.println("Example: v0.5v0.5");
    Serial.println("This will set the VM and VT voltages to 0.5 V");
}
