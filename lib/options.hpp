#pragma once

#include <stdbool.h>

typedef struct {
    bool help;
    bool version;
    char* input_file;
    bool compressed;
} CommandLineArgs;

bool parse_command_line(int argc, char* argv[], CommandLineArgs* args);
void print_usage(const char* program_name);
void print_version(void);
void cleanup_args(CommandLineArgs* args);