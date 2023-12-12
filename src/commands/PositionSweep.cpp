#include "Utilities.h"
#include "PositionSweep.h"

void PositionSweep::execute(String input_line)
{

    // Command structure: p<frequency in MHz>t<range>,<step size>,<backlash>,<last_direction>m<range>,<step size>,<backlash>,<last_direction>"

    // First we get the frequency where the voltage sweep should be performed
    float frequency_MHz = input_line.substring(1, input_line.indexOf('t')).toFloat();


    uint32_t frequency = validateInput(frequency_MHz);
    if (frequency == 0)
    {
        printInfo("Invalid frequency input for voltage sweep.");
        return;
    }

    // Set the frequency
    setFrequency(frequency);

    // Then we get the tuning parameters
    String tuning_parameters = input_line.substring(input_line.indexOf('t') + 1, input_line.indexOf('m'));
    uint32_t tuning_range = tuning_parameters.substring(0, tuning_parameters.indexOf(',')).toInt();
    tuning_parameters = tuning_parameters.substring(tuning_parameters.indexOf(',') + 1);
    uint32_t tuning_step = tuning_parameters.substring(0, tuning_parameters.indexOf(',')).toInt();
    tuning_parameters = tuning_parameters.substring(tuning_parameters.indexOf(',') + 1);
    uint32_t tuning_backlash = tuning_parameters.substring(0, tuning_parameters.indexOf(',')).toInt();
    tuning_parameters = tuning_parameters.substring(tuning_parameters.indexOf(',') + 1);
    tuning_last_direction = tuning_parameters.toInt();

    // Then we get the matching parameters
    String matching_parameters = input_line.substring(input_line.indexOf('m') + 1);
    uint32_t matching_range = matching_parameters.substring(0, matching_parameters.indexOf(',')).toInt();
    matching_parameters = matching_parameters.substring(matching_parameters.indexOf(',') + 1);
    uint32_t matching_step = matching_parameters.substring(0, matching_parameters.indexOf(',')).toInt();
    matching_parameters = matching_parameters.substring(matching_parameters.indexOf(',') + 1);
    uint32_t matching_backlash = matching_parameters.substring(0, matching_parameters.indexOf(',')).toInt();
    matching_parameters = matching_parameters.substring(matching_parameters.indexOf(',') + 1);
    matching_last_direction = matching_parameters.toInt();


    printInfo("Tuning and Matching to target frequency in MHz (automatic mode):");

    // Perform the  position sweep
    sweepPositions(tuning_range, tuning_step, tuning_backlash, matching_range, matching_step, matching_backlash);

    // Finally we set the found positions
    absolute_move_backlashcorrected(tuner, tuning_position, tuning_backlash);
    absolute_move_backlashcorrected(matcher, matching_position, matching_backlash);

}

void PositionSweep::sweepPositions(uint32_t tuning_range, uint32_t tuning_step, uint32_t tuning_backlash, uint32_t matching_range, uint32_t matching_step, uint32_t matching_backlash)
{
    int AVERAGES = 4;
    // We want to maximize the reflection value, so we start with zero
    float_t minimum_reflection = 0.0;

    // First we get the current tuning position
    uint32_t start_tuning_position = tuner.STEPPER.currentPosition();
    // Then we get the current matching position
    uint32_t start_matching_position = matcher.STEPPER.currentPosition();

    uint32_t minimum_tuning_position = start_tuning_position - tuning_range;
    // Maximum tuning position
    uint32_t maximum_tuning_position = start_tuning_position + tuning_range;

    // Minimum matching position
    uint32_t minimum_matching_position = start_matching_position - matching_range;
    // Maximum matching position
    uint32_t maximum_matching_position = start_matching_position + matching_range;

    // The variables here are absolute positions
    for (uint32_t c_tuning_position = minimum_tuning_position; c_tuning_position <= maximum_tuning_position; c_tuning_position += tuning_step)
    {
        int backlash_compensation = absolute_move_backlashcorrected(tuner, c_tuning_position, tuning_backlash);
        for (uint32_t c_matching_position = minimum_matching_position; c_matching_position <= maximum_matching_position; c_matching_position += matching_step)
        {
            // Set the tuning and matching voltage
            int backlash_compensation = absolute_move_backlashcorrected(matcher, c_matching_position, matching_backlash);

            // Measure the reflection at the given frequency
            int reflection = readReflection(AVERAGES);

            // If the reflection is lower than the current minimum, we have found a new minimum
            if (reflection > minimum_reflection)
            {
                minimum_reflection = reflection;
                tuning_position = c_tuning_position;
                matching_position = c_matching_position;
            }
        }
    }
}

int PositionSweep::absolute_move_backlashcorrected(Stepper stepper, uint32_t position, int32_t backlash)
{
    uint32_t current_position;
    // First we calculate the direction of the movement
    int direction_to_move = 0;
    int32_t backlash_compensation = 0;

    // Get the stepper type
    String stepper_type = stepper.TYPE;

    int stepper_last_direction;

    if (stepper_type == "Tuner")
    {
        current_position = tuner.STEPPER.currentPosition();
        stepper_last_direction = tuning_last_direction;
    }
    else if (stepper_type == "Matcher")
    {
        current_position = matcher.STEPPER.currentPosition();
        stepper_last_direction = matching_last_direction;
    }

    int difference = position - (int32_t)current_position;

    // If the difference is negative the direction is negative
    if (difference < 0)
        direction_to_move = -1;
    // If the difference is positive the direction is positive
    else if (difference > 0)
        direction_to_move = 1;

    // Now we check if the backlash compensation is necessary
    // If the direction of the movement is the same as the last direction, we do not need to compensate for backlash
    if (direction_to_move == stepper_last_direction)
    {
        backlash_compensation = 0;
    }
    // If the direction of the movement is different from the last direction, we need to compensate for backlash
    else
    {
        backlash_compensation = backlash * direction_to_move;
    }

    // Now we move the stepper motor
    // And we set the last direction
    // This done here like that because I don't want to use pointers
    if (stepper_type == "Tuner")
    {
        tuning_last_direction = direction_to_move;
        tuner.STEPPER.moveTo(position + backlash_compensation);
        tuner.STEPPER.runToPosition();
        tuner.STEPPER.setCurrentPosition(position);
    }
    else if (stepper_type == "Matcher")
    {
        matching_last_direction = direction_to_move;
        matcher.STEPPER.moveTo(position + backlash_compensation);
        matcher.STEPPER.runToPosition();
    }

    return backlash_compensation;

}

void PositionSweep::printResult()
{
    // Print the results which are then read by the autotm module
    String identifier = "z";
    char delimiter = 'm';


    String text = String(identifier) + String(tuning_position) + "," + String(tuning_last_direction) + String(delimiter) + String(matching_position)  + "," + String(matching_last_direction);

    Serial.println(text);
}

void PositionSweep::printHelp()
{
    Serial.println("Position sweep command");
    Serial.println("Syntax: p<frequency in MHz>t<range>,<step size>,<backlash>m<range>,<step size>,<backlash>");
    Serial.println("Example: p100t100,20,1m100,20,1");
    Serial.println("This will perform a position sweep around 100MHz with a tuning range of 100 steps, a tuning step size of 20 step and a tuning backlash of 1 step. The same parameters are used for the matching stepper motor.");
}