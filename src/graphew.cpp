#include <iostream>
#include <memory>
#include "graph.hpp"
#include "renderer.hpp"
#include "options.hpp"
#include "swaptube_pixels.hpp"

int main(int argc, char* argv[]) {
    CommandLineArgs args;
    if (!parse_command_line(argc, argv, &args)) {
        std::cerr << "Error parsing command line arguments\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (args.help) {
        print_usage(argv[0]);
        cleanup_args(&args);
        return EXIT_SUCCESS;
    }
    
    if (args.version) {
        print_version();
        cleanup_args(&args);
        return EXIT_SUCCESS;
    }
    
    auto graph = std::make_unique<Graph3D>();
    if (!graph) {
        std::cerr << "Failed to create graph\n";
        cleanup_args(&args);
        return EXIT_FAILURE;
    }
    
    auto renderer = std::make_unique<GraphRenderer>();
    if (!renderer) {
        std::cerr << "Failed to create renderer\n";
        cleanup_args(&args);
        return EXIT_FAILURE;
    }
    
    renderer->init_window("Graphew - 3D Graph Renderer");
    
    if (args.input_file) {
        std::cout << "Loading graph from " << (args.compressed ? "compressed " : "") 
                  << "JSON file: " << args.input_file << std::endl;
        
        bool loaded = false;
        if (args.compressed) {
            loaded = graph->load_from_compressed_json(args.input_file);
        } else {
            loaded = graph->load_from_json(args.input_file);
        }
        
        if (!loaded) {
            std::cout << "Failed to load JSON file, generating sample graph instead\n";
            graph->generate_sample();
        }
    } else {
        std::cout << "No input file provided, generating sample graph\n";
        graph->generate_sample();
    }
    
    std::cout << "Graph loaded: " << graph->node_count << " nodes, " << graph->edge_count << " edges\n";
    
    sf::Clock clock;
    static bool physics_enabled = true;
    
    while (!renderer->should_close()) {
        float delta_time = clock.restart().asSeconds();
        
        renderer->update_camera();
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P)) {
            physics_enabled = !physics_enabled;
            std::cout << "Physics " << (physics_enabled ? "enabled" : "disabled") << std::endl;
        }
        
        if (physics_enabled) {
            graph->update_physics(delta_time * 0.5f);
        }
        
        Pixels empty_overlay;
        renderer->render_frame(*graph, empty_overlay);
    }
    
    cleanup_args(&args);
    return EXIT_SUCCESS;
}