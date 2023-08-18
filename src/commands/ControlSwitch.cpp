#include "Utilities.h"
#include "ControlSwitch.h"

// Constructor
ControlSwitch::ControlSwitch()
{
    pinMode(RF_SWITCH_PIN, OUTPUT);
    switch_state = ATM_SYSTEM;
    digitalWrite(RF_SWITCH_PIN, LOW);
}

void ControlSwitch::execute(String input_line)
{
    char switch_to = input_line[1];

    if ((switch_to == PRE_AMP) && (switch_state != PRE_AMP))
    {
        switch_state = PRE_AMP;
        digitalWrite(RF_SWITCH_PIN, HIGH);
        result = "Switched to preamp";
    }
    else if ((switch_to == ATM_SYSTEM) && (switch_state != ATM_SYSTEM))
    {
        switch_state = ATM_SYSTEM;
        digitalWrite(RF_SWITCH_PIN, LOW);
        result = "Switched to atm system";
    }
    else if (switch_to == switch_state)
    {
        String active_pathway = (switch_state == PRE_AMP) ? "preamp" : "atm system";
        result = "Already switched to " + String(active_pathway);
    }
    else
    {
        result = "Invalid switch state";
    }
}

void ControlSwitch::printResult()
{
    printInfo(result);
}

void ControlSwitch::printHelp()
{
    Serial.println("Control switch command");
    Serial.println("Syntax: s<switch state>");
    Serial.println("Example: sa");
    Serial.println("This will switch to the atm system");
    Serial.println("Possible switch states:");
    Serial.println("p: preamp");
    Serial.println("a: atm system");
}