#include <iostream>
#include <memory>
#include <vector>
#include "graph.hpp"
#include "renderer.hpp"
#include "options.hpp"
#include "swaptube_pixels.hpp"
#include "replay_parser.hpp"
#include "force_layout.hpp"

void print_replay_info(const ReplayData& replay) {
    // Print enhanced camera and lighting controls
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
    std::vector<Vector3> initial_positions; // Declare at function scope for R key access

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


        print_replay_info(replay);

        // Find inventory items that agents actually collected
        std::vector<std::string> inventory_dims;
        std::vector<std::string> active_items;

        // Check which items have actual timestep data (not just non-zero values)
        for (const std::string& item : replay.inventory_items) {
            bool has_timestep_data = false;
            for (const auto& agent : replay.agents) {
                auto it = agent.inventory_over_time.find(item);
                if (it != agent.inventory_over_time.end() && it->second.size() > 0) {
                    has_timestep_data = true;
                    break;
                }
            }
            if (has_timestep_data) {
                active_items.push_back(item);
                std::cout << "Active item found: " << item << std::endl;
            } else {
                std::cout << "Skipping item with no data: " << item << std::endl;
            }
        }

        // Use active items or fallback to first available items
        if (active_items.size() >= 3) {
            inventory_dims = {active_items[0], active_items[1], active_items[2]};
        } else if (active_items.size() >= 2) {
            inventory_dims = {active_items[0], active_items[1], "time"}; // Use time as Z dimension
        } else {
            inventory_dims = {"ore_red", "battery_red", "time"}; // fallback with time
        }


        AgentGraphBuilder::build_inventory_dimensional_graph(replay, *graph3d, inventory_dims);

        // Store initial positions IMMEDIATELY after graph building for 'R' key reset
        for (uint32_t i = 0; i < graph3d->node_count; i++) {
            initial_positions.push_back(graph3d->nodes[i].position);
        }
        std::cout << "Stored " << initial_positions.size() << " initial positions for reset functionality" << std::endl;

        // DEBUG: Print first few initial positions to see what we're storing
        for (uint32_t i = 0; i < std::min(3u, (uint32_t)initial_positions.size()); i++) {
            std::cout << "  Initial pos " << i << ": (" << initial_positions[i].x
                      << "," << initial_positions[i].y << "," << initial_positions[i].z << ")" << std::endl;
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

    // DON'T center graph - use original positions and point camera at their geometric center
    Vector3 min_bounds, max_bounds;
    renderer->calculate_graph_bounds(*graph3d, min_bounds, max_bounds);

    // SYNC WITH FORCE LAYOUT: Camera target matches where force layout centers nodes

    Vector3 actual_graph_center(0, 0, 0); // Match the force layout target center

    auto renderer_ptr = renderer.get();
    renderer_ptr->camera_target = actual_graph_center; // Look at visual center for perfect centering
    renderer_ptr->camera_distance = 100.0f; // Standard distance

    // Set reasonable camera angles
    renderer_ptr->camera_angle_v = 0.3f; // Look down slightly
    renderer_ptr->camera_angle_h = 0.5f; // Angle for 3D view
    renderer_ptr->update_camera_position();

    // Initial positions already stored right after graph building

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

    // Reset pause countdown and key state tracking (declared at loop scope)
    int reset_pause_frames = 0;
    bool r_key_was_pressed = false;

    while (!renderer->should_close()) {
        float delta_time = clock.restart().asSeconds();

        renderer->update_camera();
        // Sync render dimension from slider
        renderer->set_render_dimension(render_dim);

        // Handle reset pause countdown first
        if (reset_pause_frames > 0) {
            reset_pause_frames--;
            if (reset_pause_frames == 0 && physics_enabled) {
                force_layout_running = true;
                std::cout << "Force layout restarting after pause" << std::endl;
            }
        }

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

        // Toggle continuous force layout with 'T'
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::T)) {
            static sf::Clock key_timer;
            if (key_timer.getElapsedTime().asSeconds() > 0.5f) {
                force_layout_running = !force_layout_running;
                std::cout << "Force layout " << (force_layout_running ? "running" : "paused") << std::endl;
                key_timer.restart();
            }
        }

        // R key resets graph to initial layout (no timer - works immediately)
        static bool r_key_was_pressed = false;
        bool r_key_is_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
        // Only trigger on key press (not while held down)
        if (r_key_is_pressed && !r_key_was_pressed) {
            std::cout << "Resetting graph to initial layout..." << std::endl;

            // Restore initial positions before any force layout
            std::cout << "Restoring " << initial_positions.size() << " positions..." << std::endl;
            for (uint32_t i = 0; i < graph3d->node_count && i < initial_positions.size(); i++) {
                Vector3 old_pos = graph3d->nodes[i].position;
                graph3d->nodes[i].position = initial_positions[i];
                graph3d->nodes[i].velocity = Vector3(0, 0, 0); // Reset velocities
                graph3d->nodes[i].force = Vector3(0, 0, 0);    // Reset forces

                if (i < 3) {
                    std::cout << "  Node " << i << ": (" << old_pos.x << "," << old_pos.y << "," << old_pos.z
                                << ") -> (" << initial_positions[i].x << "," << initial_positions[i].y << "," << initial_positions[i].z << ")" << std::endl;
                }
            }

            // Pause force layout briefly so you can see the reset visually
            force_layout_running = false;
            physics_enabled = true; // Ensure physics is enabled for later restart

            // Set the loop variable to pause force layout
            reset_pause_frames = 5; // Pause for 60 frames (~1 second at 60fps)

            std::cout << "Force layout paused - will restart in 1 second" << std::endl;

        }

        // Update key state for next frame (MUST be outside the if block)
        r_key_was_pressed = r_key_is_pressed;

        // Apply force layout in real-time if running
        if (force_layout_running) {
            int dummy_remaining = layout_params.iterations; // large number, decremented internally but ignored
            ForceLayoutEngine::apply_force_layout_step(*graph3d, layout_params, dummy_remaining);

            // DON'T override camera target - let the user control it
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