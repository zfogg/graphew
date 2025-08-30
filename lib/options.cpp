#include "options.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

bool parse_command_line(int argc, char* argv[], CommandLineArgs* args) {
    if (!args) return false;
    
    memset(args, 0, sizeof(CommandLineArgs));
    
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "f:hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'f': {
                if (args->input_file) {
                    free(args->input_file);
                }
                args->input_file = strdup(optarg);
                
                size_t len = strlen(optarg);
                if (len > 2 && strcmp(optarg + len - 2, ".z") == 0) {
                    args->compressed = true;
                } else if (len > 6 && strcmp(optarg + len - 6, ".json.z") == 0) {
                    args->compressed = true;
                }
                break;
            }
                
            case 'h':
                args->help = true;
                break;
                
            case 'v':
                args->version = true;
                break;
                
            case '?':
                return false;
                
            default:
                return false;
        }
    }
    
    if (optind < argc && !args->input_file) {
        args->input_file = strdup(argv[optind]);
        
        size_t len = strlen(argv[optind]);
        if (len > 2 && strcmp(argv[optind] + len - 2, ".z") == 0) {
            args->compressed = true;
        } else if (len > 6 && strcmp(argv[optind] + len - 6, ".json.z") == 0) {
            args->compressed = true;
        }
    }
    
    return true;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [FILE]\n\n", program_name);
    printf("Graphew - 3D Graph Renderer\n");
    printf("Visualize graphs from JSON files with interactive 3D rendering\n\n");
    printf("Options:\n");
    printf("  -f, --file FILE     Load graph from JSON file (supports .json.z compression)\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show version information\n\n");
    printf("Controls:\n");
    printf("  Mouse               Rotate camera around graph\n");
    printf("  Mouse Wheel         Zoom in/out\n");
    printf("  SPACE               Toggle camera mode\n");
    printf("  R                   Reset camera position\n");
    printf("  P                   Toggle physics simulation\n");
    printf("  ESC                 Exit application\n\n");
    printf("Examples:\n");
    printf("  %s                           # Run with sample graph\n", program_name);
    printf("  %s graph.json                # Load uncompressed JSON\n", program_name);
    printf("  %s -f replay.json.z          # Load zlib compressed JSON\n", program_name);
    printf("  %s --file data.json.z        # Load zlib compressed JSON (long form)\n", program_name);
}

void print_version(void) {
    printf("Graphew 1.0.0\n");
    printf("3D Graph Renderer with JSON and zlib support\n");
    printf("Built with raylib, cJSON, and zlib\n");
}

void cleanup_args(CommandLineArgs* args) {
    if (args && args->input_file) {
        free(args->input_file);
        args->input_file = NULL;
    }
}