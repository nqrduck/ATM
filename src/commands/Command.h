// Command.h
#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>

class Command {
public:
    /**
   * @brief Executes the command.
   * Pure virtual function that must be implemented by derived classes.
   */
    virtual void execute(String input_line) = 0;

    /**
     * @brief Prints the result of the command.
     * Pure virtual function that must be implemented by derived classes.
    */
    virtual void printResult() = 0;

    /**
     * @brief Prints the help message of the command.
     * Pure virtual function that must be implemented by derived classes.
    */
    virtual void printHelp() = 0;

   
};

#endif