#pragma once

#include <stdbool.h>

typedef struct {
    bool help;
    bool version;
    char* input_file;
    bool compressed;
    
    // Inventory filtering options
    char* filter_items;     // Comma-separated list of items to track
    bool inventory_mode;    // Enable inventory transition graph mode
    bool color_by_total;    // Color nodes by total inventory
    bool size_by_freq;      // Size nodes by frequency
    int min_timestep;
    int max_timestep;
} CommandLineArgs;

bool parse_command_line(int argc, char* argv[], CommandLineArgs* args);
void print_usage(const char* program_name);
void print_version(void);
void cleanup_args(CommandLineArgs* args);