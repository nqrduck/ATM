#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <Arduino.h>
#include <map>
#include "commands/Command.h"

class CommandManager {
public:
  void registerCommand(char identifier, Command* command);
  void executeCommand(char identifier, String input_line);
  void printCommandResult(char identifier);

private:
  std::map<char, Command*> commandMap;
};

#endif