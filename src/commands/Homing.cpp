#include "Utilities.h"
#include "Homing.h"

void Homing::execute(String input_line) {
    printInfo("Homing...");
    tuner.STEPPER.setCurrentPosition(homeStepper(tuner));
    matcher.STEPPER.setCurrentPosition(homeStepper(matcher));
}

void Homing::printResult() {
    printInfo("Resonance frequency after homing:");
    uint32_t startf = 35000000U;
    uint32_t stopf = 110000000U;
    uint32_t stepf = 100000U;
    uint32_t resonance_frequency = findCurrentResonanceFrequency(startf, stopf, stepf / 2);
    printInfo(resonance_frequency);
    Serial.print("r");
    printInfo("Homing finished");
}

void Homing::printHelp() {
    Serial.println("Homing command");
    Serial.println("Syntax: h");
    Serial.println("Example: h");
    Serial.println("This will home the tuner and matcher");
}

