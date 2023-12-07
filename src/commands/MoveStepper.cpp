#include "Utilities.h"
#include "MoveStepper.h"

void MoveStepper::execute(String input_line)
{
    #define MATCHING_STEPPER 'm'
    #define TUNING_STEPPER 't'

    char stepper = input_line[1];
    int steps = input_line.substring(2).toInt();

    if (stepper == MATCHING_STEPPER)
    {
         matcher.STEPPER.move(steps);
         matcher.STEPPER.runToPosition();
    }
    else if (stepper == TUNING_STEPPER)
    {
        tuner.STEPPER.move(steps);
        tuner.STEPPER.runToPosition();
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
}

void MoveStepper::printHelp()
{
    Serial.println("Move stepper command");
    Serial.println("Syntax: m<stepper motor><steps>");
    Serial.println("Example: mt100");
    Serial.println("This will move the tuning stepper motor 100 steps");
}