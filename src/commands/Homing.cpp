#include "Utilities.h"
#include "Homing.h"

void Homing::execute(String input_line)
{
    printInfo("Homing...");
    // Move the steppers to their home position
    tuner.STEPPER.setCurrentPosition(homeStepper(tuner));
    matcher.STEPPER.setCurrentPosition(homeStepper(matcher));

    // Now move the stepper a little bit away from the home position
    uint32_t REST_POSITION = 10000;

    tuner.STEPPER.moveTo(REST_POSITION);
    tuner.STEPPER.runToPosition();

    matcher.STEPPER.moveTo(REST_POSITION);
    matcher.STEPPER.runToPosition();
}

void Homing::printResult()
{
    printInfo("Homing finished");
    // Format is p<tuning_position>m<matching_position>
    uint32_t tuning_position = tuner.STEPPER.currentPosition();
    uint32_t matching_position = matcher.STEPPER.currentPosition();
    String position = "p" + String(tuning_position) + "m" + String(matching_position);
    Serial.println(position);
}

void Homing::printHelp()
{
    Serial.println("Homing command");
    Serial.println("Syntax: h");
    Serial.println("Example: h");
    Serial.println("This will home the tuner and matcher");
}
