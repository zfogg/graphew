#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <set>
#include <algorithm>
#include <cstdlib>
#include "graph.hpp"
#include "renderer.hpp"
#include "options.hpp"
#include "swaptube_pixels.hpp"
#include "replay_parser.hpp"
#include "force_layout.hpp"
#include "inventory_filter.hpp"

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
    std::vector<InventoryState> inventory_states;  // Keep states for rebuilding
    std::map<std::string, bool> item_tracking;     // Keep tracking state
    std::set<std::string> tracked_items;           // Current tracked items
    
    // Color mode selection
    bool color_by_hearts = false;
    bool color_by_reward = false;
    bool color_by_specific = false;
    std::string specific_item = "red_ore";
    
    // Graph structure mode (not needed - temporal is always preserved)
    
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
        
        // Check if we should use inventory filtering mode
        if (args.inventory_mode) {
            std::cout << "\nUsing inventory transition graph mode\n";
            
            // Initialize checkboxes for each available item
            for (const auto& item : replay.inventory_items) {
                item_tracking[item] = false;  // Start with all unchecked
            }
            
            // Parse filter items if provided via command line
            if (args.filter_items) {
                std::stringstream ss(args.filter_items);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    tracked_items.insert(item);
                    if (item_tracking.find(item) != item_tracking.end()) {
                        item_tracking[item] = true;  // Check the box for this item
                    }
                    std::cout << "  Tracking: " << item << "\n";
                }
            } else {
                // If no items specified, track all by default
                for (auto& [item, track] : item_tracking) {
                    track = true;
                    tracked_items.insert(item);
                }
            }
            
            // Add checkboxes to the UI
            renderer->clear_checkboxes();
            for (auto& [item, track] : item_tracking) {
                renderer->add_checkbox(item, &track);
            }
            
            // Add visual option checkboxes
            renderer->add_checkbox("--- Visual Options ---", nullptr);  // Separator
            renderer->add_checkbox("Size by Frequency", &args.size_by_freq);
            renderer->add_checkbox("--- Color Modes ---", nullptr);  // Separator
            renderer->add_checkbox("Default Color", nullptr);  // Will handle this specially
            renderer->add_checkbox("Color by Total Items", &args.color_by_total);
            renderer->add_checkbox("Color by Hearts", &color_by_hearts);
            renderer->add_checkbox("Color by Specific Item", &color_by_specific);
            
            // Build inventory states from replay (store in outer scope for rebuilding)
            inventory_states.clear();
            
            // For each agent, collect all timesteps where inventory changed
            for (const auto& agent : replay.agents) {
                std::set<int> timesteps;
                
                // Collect all unique timesteps for this agent
                for (const auto& [item, values] : agent.inventory_over_time) {
                    // Skip if filtering and item not tracked
                    if (!tracked_items.empty() && tracked_items.find(item) == tracked_items.end()) {
                        continue;
                    }
                    for (const auto& tv : values) {
                        if (args.min_timestep >= 0 && tv.timestep < args.min_timestep) continue;
                        if (args.max_timestep >= 0 && tv.timestep > args.max_timestep) continue;
                        timesteps.insert(tv.timestep);
                    }
                }
                
                // Create complete inventory states for each timestep
                for (int timestep : timesteps) {
                    InventoryState state;
                    state.timestep = timestep;
                    state.agent_id = agent.agent_id;
                    
                    // Fill in all tracked items at this timestep
                    for (const auto& [item, values] : agent.inventory_over_time) {
                        // Skip if filtering and item not tracked
                        if (!tracked_items.empty() && tracked_items.find(item) == tracked_items.end()) {
                            continue;
                        }
                        
                        // Find value at this timestep (or most recent before it)
                        int value = 0;
                        for (const auto& tv : values) {
                            if (tv.timestep <= timestep) {
                                value = static_cast<int>(tv.value);
                            } else {
                    break;
                            }
                        }
                        
                        if (value > 0 || tracked_items.empty()) {  // Include zero values if tracking all
                            state.items[item] = value;
                        }
                    }
                    
                    // Only add state if it has any items
                    if (!state.items.empty()) {
                        inventory_states.push_back(state);
                    }
                }
            }
            
            // Sort states by agent and timestep for proper transition creation
            std::sort(inventory_states.begin(), inventory_states.end(), [](const InventoryState& a, const InventoryState& b) {
                if (a.agent_id != b.agent_id) return a.agent_id < b.agent_id;
                return a.timestep < b.timestep;
            });
            
            std::cout << "  Created " << inventory_states.size() << " inventory observations\n";
            
            // Configure the filter
            InventoryFilterConfig config;
            config.tracked_items = tracked_items;
            config.size_by_frequency = args.size_by_freq;
            config.min_timestep = args.min_timestep;
            config.max_timestep = args.max_timestep;
            config.separate_by_agent = false;  // Aggregate across all agents
            config.only_changes = true;  // Only show transitions that change inventory
            config.layout_mode = InventoryFilterConfig::FORCE_DIRECTED;
            
            // Set color mode based on checkboxes
            if (args.color_by_total) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_TOTAL;
            } else if (color_by_hearts) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_HEARTS;
            } else if (color_by_specific) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_SPECIFIC_ITEM;
                config.color_by_item = specific_item;
            } else {
                config.color_mode = InventoryFilterConfig::ColorMode::DEFAULT;
            }
            
            // Create the transition graph
            InventoryFilter::create_transition_graph(*graph3d, inventory_states, config);
            
        } else {
            // Build temporal graph with shared nodes for same inventory states
            // Different from inventory mode: keeps all transitions (not deduplicated)
            
            // Collect all states with their agent and timestep info
            struct TemporalState {
                InventoryState state;
                int agent_id;
                int timestep;
                std::string inventory_key;
            };
            std::vector<TemporalState> all_states;
            
            for (const auto& agent : replay.agents) {
                std::set<int> timesteps;
                
                // Collect all timesteps where something changed
                for (const auto& [item, values] : agent.inventory_over_time) {
                    for (const auto& tv : values) {
                        timesteps.insert(tv.timestep);
                    }
                }
                for (const auto& tv : agent.total_reward_over_time) {
                    timesteps.insert(tv.timestep);
                }
                
                // Create a state for each timestep
                for (int t : timesteps) {
                    TemporalState ts;
                    ts.agent_id = agent.agent_id;
                    ts.timestep = t;
                    ts.state.timestep = t;
                    ts.state.agent_id = agent.agent_id;
                    
                    // Get inventory at this time
                    for (const std::string& item : replay.inventory_items) {
                        int qty = static_cast<int>(agent.get_inventory_at_time(item, t));
                        if (qty > 0) {
                            ts.state.items[item] = qty;
                        }
                    }
                    
                    // Create inventory key (same states will have same key)
                    std::set<std::string> all_items;
                    for (const auto& [item, _] : ts.state.items) {
                        all_items.insert(item);
                    }
                    ts.inventory_key = ts.state.get_key(all_items);
                    
                    all_states.push_back(ts);
                }
            }
            
            // Sort by timestep then agent
            std::sort(all_states.begin(), all_states.end(),
                     [](const auto& a, const auto& b) { 
                         if (a.timestep != b.timestep) return a.timestep < b.timestep;
                         return a.agent_id < b.agent_id;
                     });
            
            // Create nodes - share nodes for same inventory states
            std::map<std::string, uint32_t> inventory_to_node;  // Map inventory state to node
            std::map<std::pair<int, int>, uint32_t> agent_time_to_node;  // Map (agent,time) to node
            
            for (const auto& ts : all_states) {
                uint32_t node_id;
                
                // Check if this inventory state already has a node
                auto it = inventory_to_node.find(ts.inventory_key);
                if (it != inventory_to_node.end()) {
                    // Reuse existing node for this inventory state
                    node_id = it->second;
                } else {
                    // Create new node for this inventory state
                    Vector3 pos(
                        (rand() % 200 - 100) * 0.1f,
                        (rand() % 200 - 100) * 0.1f,
                        (rand() % 200 - 100) * 0.1f
                    );
                    
                    // Color based on inventory contents
                    int total_items = 0;
                    for (const auto& [item, qty] : ts.state.items) {
                        total_items += qty;
                    }
                    
                    // Gradient from blue (empty) to green (many items)
                    float item_ratio = std::min(1.0f, total_items / 20.0f);
                    Color color(
                        50,
                        100 + static_cast<uint8_t>(155 * item_ratio),
                        200 - static_cast<uint8_t>(100 * item_ratio)
                    );
                    
                    float radius = 0.4f + 0.1f * item_ratio;
                    
                    std::string label = "State_" + std::to_string(inventory_to_node.size());
                    
                    node_id = graph3d->add_node(pos, color, radius, label);
                    inventory_to_node[ts.inventory_key] = node_id;
                    
                    // Store metadata
                    if (node_id < MAX_NODES) {
                        for (const auto& [item, qty] : ts.state.items) {
                            graph3d->nodes[node_id].properties[item] = std::to_string(qty);
                        }
                    }
                }
                
                // Map this agent-time pair to the node
                agent_time_to_node[{ts.agent_id, ts.timestep}] = node_id;
            }
            
            // Create edges for each agent's trajectory
            std::map<int, std::vector<std::pair<int, int>>> agent_timeline;
            for (const auto& ts : all_states) {
                agent_timeline[ts.agent_id].push_back({ts.timestep, ts.agent_id});
            }
            
            for (const auto& [agent_id, timeline] : agent_timeline) {
                for (size_t i = 1; i < timeline.size(); i++) {
                    auto prev_key = std::make_pair(agent_id, timeline[i-1].first);
                    auto curr_key = std::make_pair(agent_id, timeline[i].first);
                    
                    auto prev_it = agent_time_to_node.find(prev_key);
                    auto curr_it = agent_time_to_node.find(curr_key);
                    
                    if (prev_it != agent_time_to_node.end() && curr_it != agent_time_to_node.end()) {
                        uint32_t from_node = prev_it->second;
                        uint32_t to_node = curr_it->second;
                        
                        // Color edges by agent
                        Color edge_color = Color(
                            150 + (agent_id * 37) % 105,
                            150 + (agent_id * 73) % 105,
                            150 + (agent_id * 113) % 105
                        );
                        
                        graph3d->add_edge(from_node, to_node, edge_color, 1.0f);
                    }
                }
            }
            
            std::cout << "Created " << inventory_to_node.size() << " unique inventory state nodes\n";
            std::cout << "Mapped " << agent_time_to_node.size() << " agent-timestep pairs\n";
        }
        
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
    
    // Center graph around origin for proper display
    graph3d->center_graph();
    
    // Set camera to look at origin with appropriate distance
    Vector3 min_bounds, max_bounds;
    renderer->calculate_graph_bounds(*graph3d, min_bounds, max_bounds);
    std::cout << "Graph centered at origin - bounds: (" << min_bounds.x << "," << min_bounds.y << "," << min_bounds.z 
              << ") to (" << max_bounds.x << "," << max_bounds.y << "," << max_bounds.z << ")" << std::endl;
    
    // Print enhanced camera and lighting controls
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ENHANCED CAMERA & LIGHTING CONTROLS              ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ CAMERA MOVEMENT:                                              ║\n";
    std::cout << "║   W/S         - Move forward/backward                         ║\n";
    std::cout << "║   A/D         - Move left/right                               ║\n";
    std::cout << "║   Q/E         - Move up/down                                  ║\n";
    std::cout << "║   Arrow Keys  - Rotate camera                                 ║\n";
    std::cout << "║   Mouse Wheel - Zoom in/out                                   ║\n";
    std::cout << "║   Right Drag  - Rotate camera view                            ║\n";
    std::cout << "║   Left Drag   - Pan view                                      ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ ADVANCED CAMERA:                                              ║\n";
    std::cout << "║   Shift+Wheel - Adjust field of view (FOV)                   ║\n";
    std::cout << "║   Ctrl+Wheel  - Adjust camera movement speed                  ║\n";
    std::cout << "║   0-9         - Load camera preset                            ║\n";
    std::cout << "║   Ctrl+0-9    - Save current camera to preset                 ║\n";
    std::cout << "║   R           - Reset camera to default                       ║\n";
    std::cout << "║   Space       - Toggle auto-rotation                          ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ LIGHTING CONTROLS:                                            ║\n";
    std::cout << "║   I/K         - Increase/decrease ambient light               ║\n";
    std::cout << "║   L/J         - Increase/decrease directional light           ║\n";
    std::cout << "║   Numpad 4/6  - Rotate light horizontally                     ║\n";
    std::cout << "║   Numpad 8/2  - Rotate light vertically                       ║\n";
    std::cout << "║   Shift+S     - Toggle shadows (when available)               ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ VISUAL EFFECTS:                                               ║\n";
    std::cout << "║   F           - Toggle fog effect                             ║\n";
    std::cout << "║   G           - Toggle grid display                           ║\n";
    std::cout << "║   X           - Toggle axis indicators                        ║\n";
    std::cout << "║   O           - Toggle info overlay                           ║\n";
    std::cout << "║   H           - Show/hide this help overlay                   ║\n";
    std::cout << "║   P           - Toggle physics simulation                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
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
    bool force_layout_running = true; // continuous by default
    
    // Setup force layout parameters for real-time simulation
    ForceLayoutEngine::PhysicsParams layout_params;
    layout_params.repel = 5.0f;
    layout_params.attract = 1.0f;
    layout_params.decay = 0.6f;
    layout_params.iterations = 1000000000; // effectively infinite when used for step batches
    layout_params.dimension = 3.0f;

    // Expose sliders in the UI for interactive tuning
    renderer->clear_sliders();
    renderer->add_slider("Repel", &layout_params.repel, 0.0f, 50.0f);
    renderer->add_slider("Attract", &layout_params.attract, 0.0f, 5.0f);
    renderer->add_slider("Decay", &layout_params.decay, 0.3f, 0.99f);
    renderer->add_slider("Centering", &layout_params.centering_strength, 0.0f, 1.0f);
    renderer->add_slider("Dimension", &layout_params.dimension, 1.0f, 3.0f);
    float render_dim = 3.0f;
    renderer->add_slider("RenderDim", &render_dim, 1.0f, 3.0f);
    
    // Continuous force layout (toggle with 'T')
    int layout_iterations_remaining = layout_params.iterations;
    
    bool needs_rebuild = false;
    std::set<std::string> last_tracked_items = tracked_items;
    bool last_color_by_total = args.color_by_total;
    bool last_size_by_freq = args.size_by_freq;
    bool last_color_by_hearts = color_by_hearts;
    bool last_color_by_specific = color_by_specific;
    
    while (!renderer->should_close()) {
        float delta_time = clock.restart().asSeconds();
        
        renderer->update_camera();
        // Sync render dimension from slider
        renderer->set_render_dimension(render_dim);
        
        // Check if item tracking or visual mode changed (only in inventory mode)
        if (args.inventory_mode) {
            std::set<std::string> current_tracked;
            for (const auto& [item, track] : item_tracking) {
                if (track) {
                    current_tracked.insert(item);
                }
            }
            
            // Check if we need to rebuild due to item changes
            if (current_tracked != last_tracked_items) {
                needs_rebuild = true;
                last_tracked_items = current_tracked;
                tracked_items = current_tracked;
            }
            
            // Enforce radio button behavior for color modes
            if (args.color_by_total && args.color_by_total != last_color_by_total) {
                color_by_hearts = false;
                color_by_specific = false;
            } else if (color_by_hearts && color_by_hearts != last_color_by_hearts) {
                args.color_by_total = false;
                color_by_specific = false;
            } else if (color_by_specific && color_by_specific != last_color_by_specific) {
                args.color_by_total = false;
                color_by_hearts = false;
            }
            
            // Check if visual mode changed (also needs rebuild to recolor)
            if (args.color_by_total != last_color_by_total || 
                args.size_by_freq != last_size_by_freq ||
                color_by_hearts != last_color_by_hearts ||
                color_by_specific != last_color_by_specific) {
                needs_rebuild = true;
                last_color_by_total = args.color_by_total;
                last_size_by_freq = args.size_by_freq;
                last_color_by_hearts = color_by_hearts;
                last_color_by_specific = color_by_specific;
            }
        }
        
        // Rebuild graph if needed
        if (needs_rebuild && args.inventory_mode) {
            needs_rebuild = false;
            
            std::cout << "\nRebuilding graph with items: ";
            for (const auto& item : tracked_items) {
                std::cout << item << " ";
            }
            std::cout << "\n";
            
            // Clear existing graph
            graph3d = std::make_unique<Graph3D>();
            
            // Rebuild with new tracking
            InventoryFilterConfig config;
            config.tracked_items = tracked_items;
            config.size_by_frequency = args.size_by_freq;
            config.min_timestep = args.min_timestep;
            config.max_timestep = args.max_timestep;
            config.separate_by_agent = false;
            config.only_changes = true;
            config.layout_mode = InventoryFilterConfig::FORCE_DIRECTED;
            
            // Set color mode based on checkboxes
            if (args.color_by_total) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_TOTAL;
            } else if (color_by_hearts) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_HEARTS;
            } else if (color_by_specific) {
                config.color_mode = InventoryFilterConfig::ColorMode::BY_SPECIFIC_ITEM;
                config.color_by_item = specific_item;
            } else {
                config.color_mode = InventoryFilterConfig::ColorMode::DEFAULT;
            }
            
            InventoryFilter::create_transition_graph(*graph3d, inventory_states, config);
            
            std::cout << "Rebuilt: " << graph3d->node_count << " nodes, " 
                     << graph3d->edge_count << " edges\n";
                     
            graph3d->center_graph();
            Vector3 min_bounds, max_bounds;
            renderer->calculate_graph_bounds(*graph3d, min_bounds, max_bounds);
        }
        
        // Controls (only when window has focus)
        if (renderer->window_has_focus()) {
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
        
            // Toggle continuous force layout with 'T'
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
                static sf::Clock key_timer;
                if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                    force_layout_running = !force_layout_running;
                    std::cout << "Force layout " << (force_layout_running ? "running" : "paused") << std::endl;
                    key_timer.restart();
                }
            }
        } // End of focus check
        
        // Apply force layout in real-time if running
        if (force_layout_running) {
            int dummy_remaining = layout_params.iterations; // large number, decremented internally but ignored
            (void)ForceLayoutEngine::apply_force_layout_step(*graph3d, layout_params, dummy_remaining);
        }
        
        // Regular gentle physics for fine-tuning
        if (physics_enabled && !force_layout_running) {
            graph3d->update_physics(delta_time * 0.02f);
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