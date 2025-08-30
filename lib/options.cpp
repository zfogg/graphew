#include "options.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

bool parse_command_line(int argc, char* argv[], CommandLineArgs* args) {
    if (!args) return false;
    
    memset(args, 0, sizeof(CommandLineArgs));
    args->min_timestep = -1;
    args->max_timestep = -1;
    
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"inventory", no_argument, 0, 'i'},
        {"items", required_argument, 0, 'I'},
        {"color-total", no_argument, 0, 'c'},
        {"size-freq", no_argument, 0, 's'},
        {"timestep", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "f:hviI:cst:", long_options, &option_index)) != -1) {
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
                
            case 'i':
                args->inventory_mode = true;
                break;
                
            case 'I':
                if (args->filter_items) free(args->filter_items);
                args->filter_items = strdup(optarg);
                break;
                
            case 'c':
                args->color_by_total = true;
                break;
                
            case 's':
                args->size_by_freq = true;
                break;
                
            case 't': {
                // Parse timestep range (format: "start:end" or just "end")
                char* colon = strchr(optarg, ':');
                if (colon) {
                    *colon = '\0';
                    args->min_timestep = atoi(optarg);
                    args->max_timestep = atoi(colon + 1);
                } else {
                    args->max_timestep = atoi(optarg);
                }
                break;
            }
                
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
    printf("Inventory Filtering:\n");
    printf("  -i, --inventory     Create transition graph from inventory states\n");
    printf("  -I, --items LIST    Track specific items (comma-separated, e.g. 'heart,red_ore')\n");
    printf("  -c, --color-total   Color nodes by total inventory value\n");
    printf("  -s, --size-freq     Size nodes by state frequency\n");
    printf("  -t, --timestep N    Limit to timestep range (e.g. '100:500' or just '500')\n\n");
    printf("Examples:\n");
    printf("  # Show all inventory state transitions:\n");
    printf("  %s -f replay.json.z -i\n\n", program_name);
    printf("  # Track only hearts, sized by frequency:\n");
    printf("  %s -f replay.json.z -i -I heart -s\n\n", program_name);
    printf("  # Track all ore types, colored by total:\n");
    printf("  %s -f replay.json.z -i -I red_ore,blue_ore,green_ore -c\n\n", program_name);
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