#include "Utilities.h"
#include "MoveStepper.h"

void MoveStepper::execute(String input_line)
{
    #define MATCHING_STEPPER 'm'
    #define TUNING_STEPPER 't'

    // Input format is m<stepper motor><steps>,<backlash>

    char stepper = input_line[1];
    input_line = input_line.substring(2);
    uint32_t steps = input_line.substring(0, input_line.indexOf(',')).toInt();
    input_line = input_line.substring(input_line.indexOf(',') + 1);
    uint32_t backlash = input_line.toInt();

    if (stepper == MATCHING_STEPPER)
    {
        uint32_t matching_position = matcher.STEPPER.currentPosition();
        matcher.STEPPER.move(steps + backlash);
        matcher.STEPPER.runToPosition();
        matcher.STEPPER.setCurrentPosition(matching_position + steps);
    }
    else if (stepper == TUNING_STEPPER)
    {
        uint32_t tuning_position = tuner.STEPPER.currentPosition();
        tuner.STEPPER.move(steps + backlash);
        tuner.STEPPER.runToPosition();
        tuner.STEPPER.setCurrentPosition(tuning_position + steps);
    }
    else
    {
        printInfo("Invalid stepper motor");
    }
}

void MoveStepper::printResult()
{
    // Print Info  confirmation
    printInfo("Fnished moving stepper");
    uint32_t tuning_position = tuner.STEPPER.currentPosition();
    uint32_t matching_position = matcher.STEPPER.currentPosition();
    String position = "p" + String(tuning_position) + "m" + String(matching_position);
    Serial.println(position);
}

void MoveStepper::printHelp()
{
    Serial.println("Move stepper command");
    Serial.println("Syntax: m<stepper motor><steps>");
    Serial.println("Example: mt100");
    Serial.println("This will move the tuning stepper motor 100 steps");
}