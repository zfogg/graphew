#include <iostream>
#include <memory>
#include <vector>
#include "graph.hpp"
#include "renderer.hpp"
#include "options.hpp"
#include "swaptube_pixels.hpp"
#include "replay_parser.hpp"

void print_replay_info(const ReplayData& replay) {
    std::cout << "\n=== Replay Analysis ===" << std::endl;
    std::cout << "Agents: " << replay.agents.size() << std::endl;
    std::cout << "Max timestep: " << replay.max_timestep << std::endl;
    std::cout << "Inventory items: ";
    for (const auto& item : replay.inventory_items) {
        std::cout << item << " ";
    }
    std::cout << "\n";
    
    // Show sample agent data
    if (!replay.agents.empty()) {
        const auto& sample_agent = replay.agents[0];
        std::cout << "Sample agent " << sample_agent.agent_id << " inventory:" << std::endl;
        for (const auto& [item, values] : sample_agent.inventory_over_time) {
            if (!values.empty()) {
                std::cout << "  " << item << ": " << values.front().value 
                         << " -> " << values.back().value << std::endl;
            }
        }
        
        if (!sample_agent.total_reward_over_time.empty()) {
            std::cout << "  Total reward: " << sample_agent.total_reward_over_time.front().value
                     << " -> " << sample_agent.total_reward_over_time.back().value << std::endl;
        }
    }
    std::cout << "========================\n" << std::endl;
}

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
    
    renderer->init_window("Graphew - AI Agent Inventory & Reward Visualization");
    
    ReplayData replay;
    
    if (args.input_file) {
        std::cout << "Loading replay from " << (args.compressed ? "compressed " : "") 
                  << "file: " << args.input_file << std::endl;
        
        bool loaded = false;
        if (args.compressed) {
            loaded = ReplayParser::parse_compressed_replay_file(args.input_file, replay);
        } else {
            loaded = ReplayParser::parse_replay_file(args.input_file, replay);
        }
        
        if (!loaded) {
            std::cerr << "Failed to load replay file\n";
            cleanup_args(&args);
            return EXIT_FAILURE;
        }
        
        print_replay_info(replay);
        
        // Find inventory items that agents actually collected
        std::vector<std::string> inventory_dims;
        std::vector<std::string> active_items;
        
        // Check which items have non-zero values across agents
        for (const std::string& item : replay.inventory_items) {
            bool has_values = false;
            for (const auto& agent : replay.agents) {
                auto it = agent.inventory_over_time.find(item);
                if (it != agent.inventory_over_time.end() && !it->second.empty() && it->second.back().value > 0) {
                    has_values = true;
                    break;
                }
            }
            if (has_values) active_items.push_back(item);
        }
        
        // Use active items or fallback to first available items
        if (active_items.size() >= 3) {
            inventory_dims = {active_items[0], active_items[1], active_items[2]};
        } else if (active_items.size() >= 2) {
            inventory_dims = {active_items[0], active_items[1], "time"}; // Use time as Z dimension
        } else {
            inventory_dims = {"ore_red", "battery_red", "time"}; // fallback with time
        }
        
        std::cout << "Mapping inventory dimensions to 3D space:" << std::endl;
        std::cout << "X-axis: " << inventory_dims[0] << std::endl;
        std::cout << "Y-axis: " << inventory_dims[1] << std::endl;
        std::cout << "Z-axis: " << inventory_dims[2] << std::endl;
        
        AgentGraphBuilder::build_inventory_dimensional_graph(replay, *graph3d, inventory_dims);
        
    } else {
        std::cerr << "No replay file provided. Use -f to specify a replay file.\n";
        std::cerr << "Example: ./bin/graphew -f sample.json\n";
        cleanup_args(&args);
        return EXIT_FAILURE;
    }
    
    std::cout << "Graph built: " << graph3d->node_count << " nodes, " << graph3d->edge_count << " edges\n";
    
    // Validate that we have data to render
    if (graph3d->node_count == 0) {
        std::cerr << "Error: No graph nodes created from replay data.\n";
        std::cerr << "This may indicate:\n";
        std::cerr << "  - No agents found in the replay file\n";
        std::cerr << "  - Unsupported file format\n";
        std::cerr << "  - No inventory changes detected\n";
        cleanup_args(&args);
        return EXIT_FAILURE;
    }
    
    if (graph3d->edge_count == 0) {
        std::cerr << "Warning: No edges created - agents may not have changed inventory states\n";
    }
    
    // Create info overlay showing current visualization mode
    Pixels info_overlay(400, 150);
    info_overlay.fill(TRANSPARENT_BLACK);
    info_overlay.fill_rect(5, 5, 390, 25, argb(150, 20, 20, 100));
    info_overlay.bresenham_line(5, 35, 395, 35, argb(255, 255, 255, 200), 1.0f, 1);
    
    // Add some visual indicators
    info_overlay.fill_circle(50, 70, 15, argb(200, 255, 100, 100)); // Green for high reward
    info_overlay.fill_circle(150, 70, 12, argb(200, 255, 255, 100)); // Yellow for medium
    info_overlay.fill_circle(250, 70, 8, argb(200, 255, 100, 100));  // Red for low
    
    sf::Clock clock;
    bool physics_enabled = true;
    bool show_overlay = true;
    
    while (!renderer->should_close()) {
        float delta_time = clock.restart().asSeconds();
        
        renderer->update_camera();
        
        // Controls
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P)) {
            static sf::Clock key_timer;
            if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                physics_enabled = !physics_enabled;
                std::cout << "Physics " << (physics_enabled ? "enabled" : "disabled") << std::endl;
                key_timer.restart();
            }
        }
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O)) {
            static sf::Clock key_timer;
            if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                show_overlay = !show_overlay;
                std::cout << "Overlay " << (show_overlay ? "shown" : "hidden") << std::endl;
                key_timer.restart();
            }
        }
        
        if (physics_enabled) {
            graph3d->update_physics(delta_time * 0.1f); // Gentle physics for inventory data
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