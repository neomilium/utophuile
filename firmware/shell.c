#include "shell.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "config.h"

#define SHELL_MAX_COMMAND_LINE_LENGTH   160
#ifndef SHELL_COMMAND_COUNT
#error Please set SHELL_COMMAND_COUNT and check your shell_commands[] array
#endif

extern shell_command_t shell_commands[];

void
shell_loop(void)
{
  char buffer[SHELL_MAX_COMMAND_LINE_LENGTH + 1];
  char command[SHELL_MAX_COMMAND_LINE_LENGTH + 1];

  printf_P(PSTR("$ "));
  if (fgets(buffer, sizeof(buffer) - 1, stdin) == NULL) {
    if (ferror(stdin) != 0) {
      printf_P(PSTR("\n!!! UART data overrun !!!\nresetting shell...\n"));
      clearerr(stdin);
    }
    if (feof(stdin) != 0) {
      clearerr(stdin);
    }
    return;
  }
  buffer[SHELL_MAX_COMMAND_LINE_LENGTH] = '\0';

  if (sscanf_P(buffer, PSTR("%s "), command) > 0) {
    // Look for an exact match
    for (size_t n = 0; n < SHELL_COMMAND_COUNT; n++) {
      if (0 == strcmp_P(command, shell_commands[n].text)) {
        (shell_commands[n].function)(buffer);
        return;
      }
    }
    printf_P(PSTR("%s: unknown command\n"), command);
//     // Look for a single match on left characters of input command (support partial command)
//     void (*function)(const char*);
//     function = NULL;
//     for(size_t n = 0; n < SHELL_COMMAND_COUNT; n++) {
//       if(0 == strncmp(shell_commands[n].text, command, strlen(command))) {
//         if(function != NULL) {
//           printf("%s: multiple command match\n", command);
//           return;
//         } else {
//           function = shell_commands[n].function;
//         }
//       }
//     }
//     if(function != NULL) {
//       (function)(buffer);
//     } else {
//       printf("%s: unknown command\n", command);
//     }
  }
}
