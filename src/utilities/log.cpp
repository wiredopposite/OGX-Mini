#include <stdio.h>
#include <string.h>
#include <stdarg.h>

char stored_string[100];

void clear_log() 
{
    memset(stored_string, 0, sizeof(stored_string));
}

void log(const char* format, ...) 
{
    va_list args;
    va_start(args, format);

    clear_log();

    vsnprintf(stored_string, sizeof(stored_string), format, args);
    strcat(stored_string, "\n");

    va_end(args);
}

const char* get_log() 
{
    return stored_string;
}