#ifndef __SHELL_H__
#  define __SHELL_H__

#include <avr/pgmspace.h>
#include <stdbool.h>

typedef struct {
  const char *text;
  const char *description;
  void (*function)(const char *);
  bool debug;
} shell_command_t;

#define SHELL_COMMAND_DECL(ID, TEXT, DESCRIPTION, DEBUG, FUNCTION) \
  static const char menu##ID##_text[] PROGMEM = TEXT; \
  static const char menu##ID##_description[] PROGMEM = DESCRIPTION; \
  shell_commands[ID].text = menu##ID##_text; \
  shell_commands[ID].description = menu##ID##_description; \
  shell_commands[ID].debug = DEBUG; \
  shell_commands[ID].function = FUNCTION;


void    shell_loop(void);

#endif // __SHELL_H__
