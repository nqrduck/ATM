#include "Utilities.h"
#include "Command.h"

void Command::confirmAndExecute(String input_line)
{
    confirmCommand();
    execute(input_line);
}

void Command::confirmCommand()
{
    Serial.println("c");
}
