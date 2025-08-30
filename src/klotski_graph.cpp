#include <iostream>
#include <memory>
#include "graph.hpp"
#include "renderer.hpp"
#include "options.hpp"
#include "swaptube_pixels.hpp"
#include "klotski_bridge.hpp"

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
    
    auto graph3d = std::make_unique<Graph3D>();
    auto renderer = std::make_unique<GraphRenderer>();
    
    renderer->init_window("Graphew - Klotski State Space Graph");
    
    if (args.input_file) {
        std::cout << "Loading Klotski graph from " << (args.compressed ? "compressed " : "") 
                  << "JSON file: " << args.input_file << std::endl;
        
        bool loaded = false;
        if (args.compressed) {
            loaded = graph3d->load_from_compressed_json(args.input_file);
        } else {
            loaded = KlotskiBridge::load_klotski_json(*graph3d, args.input_file);
        }
        
        if (!loaded) {
            std::cout << "Failed to load JSON, generating Klotski graph instead\n";
            goto generate_klotski;
        }
    } else {
        generate_klotski:
        std::cout << "Generating Klotski state space graph...\n";
        
        // Generate a sample Klotski state space graph
        KlotskiGraph klotski_graph = KlotskiBridge::generate_sample_klotski_graph();
        klotski_graph.convert_to_graph3d(*graph3d);
        
        // Export for future use
        klotski_graph.export_to_json("generated_klotski_sample.json");
    }
    
    std::cout << "Graph loaded: " << graph3d->node_count << " nodes, " << graph3d->edge_count << " edges\n";
    
    // Create informational overlay using swaptube Pixels
    Pixels info_overlay(300, 200);
    info_overlay.fill(TRANSPARENT_BLACK);
    info_overlay.fill_rect(5, 5, 290, 25, argb(150, 50, 50, 150));
    info_overlay.bresenham_line(5, 30, 295, 30, argb(255, 255, 255, 255), 1.0f, 2);
    info_overlay.fill_circle(150, 100, 40, argb(100, 100, 255, 200));
    
    sf::Clock clock;
    bool physics_enabled = true;
    bool show_overlay = true;
    
    while (!renderer->should_close()) {
        float delta_time = clock.restart().asSeconds();
        
        renderer->update_camera();
        
        // Toggle physics with P key
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P)) {
            static sf::Clock key_timer;
            if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                physics_enabled = !physics_enabled;
                std::cout << "Physics " << (physics_enabled ? "enabled" : "disabled") << std::endl;
                key_timer.restart();
            }
        }
        
        // Toggle overlay with O key
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O)) {
            static sf::Clock key_timer;
            if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                show_overlay = !show_overlay;
                std::cout << "Overlay " << (show_overlay ? "shown" : "hidden") << std::endl;
                key_timer.restart();
            }
        }
        
        if (physics_enabled) {
            graph3d->update_physics(delta_time * 0.3f); // Slower physics for state graphs
        }
        
        if (show_overlay) {
            renderer->render_frame(*graph3d, info_overlay);
        } else {
            Pixels empty;
            renderer->render_frame(*graph3d, empty);
        }
    }
    
    cleanup_args(&args);
    return EXIT_SUCCESS;
}