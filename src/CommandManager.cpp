#include "CommandManager.h"

void CommandManager::registerCommand(char identifier, Command *command)
{
  commandMap[identifier] = command;
}

void CommandManager::executeCommand(char identifier, String input_line)
{
  auto it = commandMap.find(identifier);
  if (it != commandMap.end())
  {
    Command *command = it->second;
    command->confirmAndExecute(input_line);
  }
  else
  {
    Serial.println("Unknown command.");
  }
}

void CommandManager::printCommandResult(char identifier)
{
  auto it = commandMap.find(identifier);
  if (it != commandMap.end())
  {
    Command *command = it->second;
    command->printResult();
  }
  else
  {
    Serial.println("Unknown command.");
  }
}