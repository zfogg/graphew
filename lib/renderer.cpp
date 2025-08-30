#include "renderer.hpp"
#include <cmath>
#include <iostream>

GraphRenderer::GraphRenderer() 
    : zoom_level(1.0f), zoom_speed(0.1f), view_center(0, 0),
      camera_position(0, 0, 15), camera_target(0, 0, 0), camera_distance(15.0f),
      camera_angle_h(0.0f), camera_angle_v(0.3f), auto_rotate(true), auto_rotate_speed(0.5f) {
}

GraphRenderer::~GraphRenderer() {
    cleanup();
}

void GraphRenderer::init_window(const std::string& title) {
    sf::VideoMode videoMode(sf::Vector2u(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT));
    window.create(videoMode, title, sf::Style::Default);
    window.setFramerateLimit(TARGET_FPS);
    
    view = window.getDefaultView();
    view.setCenter(view_center);
    window.setView(view);
}

void GraphRenderer::handle_events() {
    std::optional<sf::Event> event;
    while ((event = window.pollEvent())) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        else if (auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            float delta = mouseWheel->delta;
            zoom_level *= (1.0f + delta * zoom_speed);
            zoom_level = std::max(0.1f, std::min(zoom_level, 5.0f));
            
            view.setSize(sf::Vector2f(DEFAULT_SCREEN_WIDTH / zoom_level, DEFAULT_SCREEN_HEIGHT / zoom_level));
            window.setView(view);
        }
        else if (auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPress->code == sf::Keyboard::Key::R) {
                reset_camera();
            }
            else if (keyPress->code == sf::Keyboard::Key::Space) {
                auto_rotate = !auto_rotate;
                std::cout << "Auto-rotation " << (auto_rotate ? "enabled" : "disabled") << std::endl;
            }
        }
    }
    
    // Handle mouse dragging for panning
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
        sf::Vector2f current_mouse = static_cast<sf::Vector2f>(mouse_pos);
        
        if (last_mouse_position.x != 0 || last_mouse_position.y != 0) {
            sf::Vector2f delta = last_mouse_position - current_mouse;
            delta /= zoom_level; // Scale delta by zoom level
            
            view_center += delta;
            view.setCenter(view_center);
            window.setView(view);
        }
        
        last_mouse_position = current_mouse;
    } else {
        last_mouse_position = sf::Vector2f(0, 0);
    }
}

void GraphRenderer::update_camera() {
    handle_events();
    
    // Auto-rotation for full graph visibility
    if (auto_rotate) {
        float time = rotation_clock.getElapsedTime().asSeconds();
        camera_angle_h += auto_rotate_speed * time;
        rotation_clock.restart();
        
        // Gentle vertical oscillation
        camera_angle_v = 0.3f + 0.2f * std::sin(time * 0.3f);
    }
    
    update_camera_position();
}

void GraphRenderer::draw_grid() {
    const float grid_size = 50.0f;
    const int grid_lines = 40;
    const sf::Color grid_color(50, 50, 50, 100);
    
    sf::Vector2f view_size = view.getSize();
    sf::Vector2f view_center_pos = view.getCenter();
    
    // Create vertex arrays for grid lines
    std::vector<sf::Vertex> vertices;
    
    // Draw vertical lines
    for (int i = -grid_lines; i <= grid_lines; i++) {
        float x = i * grid_size;
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(x, view_center_pos.y - view_size.y);
        v1.color = grid_color;
        v2.position = sf::Vector2f(x, view_center_pos.y + view_size.y);
        v2.color = grid_color;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    // Draw horizontal lines
    for (int i = -grid_lines; i <= grid_lines; i++) {
        float y = i * grid_size;
        sf::Vertex v1, v2;
        v1.position = sf::Vector2f(view_center_pos.x - view_size.x, y);
        v1.color = grid_color;
        v2.position = sf::Vector2f(view_center_pos.x + view_size.x, y);
        v2.color = grid_color;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
}

void GraphRenderer::update_camera_position() {
    // Calculate 3D camera position based on angles
    camera_position.x = camera_target.x + camera_distance * std::cos(camera_angle_v) * std::cos(camera_angle_h);
    camera_position.y = camera_target.y + camera_distance * std::sin(camera_angle_v);
    camera_position.z = camera_target.z + camera_distance * std::cos(camera_angle_v) * std::sin(camera_angle_h);
}

sf::Vector2f GraphRenderer::world_to_screen_3d(const Vector3& world_pos) {
    // Transform world position relative to camera
    Vector3 relative_pos = world_pos - camera_position;
    
    // Create view matrix (simplified)
    Vector3 forward = (camera_target - camera_position).normalize();
    Vector3 right = Vector3(forward.z, 0, -forward.x).normalize(); // Cross with up(0,1,0)
    Vector3 up = Vector3(-forward.x * forward.y, forward.x * forward.x + forward.z * forward.z, -forward.z * forward.y).normalize();
    
    // Transform to camera space
    float cam_x = relative_pos.x * right.x + relative_pos.y * right.y + relative_pos.z * right.z;
    float cam_y = relative_pos.x * up.x + relative_pos.y * up.y + relative_pos.z * up.z;
    float cam_z = relative_pos.x * forward.x + relative_pos.y * forward.y + relative_pos.z * forward.z;
    
    // Perspective projection
    float perspective_scale = apply_perspective(cam_z);
    float screen_x = cam_x * perspective_scale + DEFAULT_SCREEN_WIDTH / 2.0f;
    float screen_y = -cam_y * perspective_scale + DEFAULT_SCREEN_HEIGHT / 2.0f; // Flip Y
    
    return sf::Vector2f(screen_x, screen_y);
}

float GraphRenderer::apply_perspective(float z_depth) {
    float fov = 60.0f; // Field of view in degrees
    float focal_length = DEFAULT_SCREEN_HEIGHT / (2.0f * std::tan(fov * M_PI / 360.0f));
    
    // Prevent division by zero and clamp minimum distance
    float safe_z = std::max(z_depth, 0.1f);
    return focal_length / safe_z * zoom_level;
}

void GraphRenderer::reset_camera() {
    camera_distance = 15.0f;
    camera_angle_h = 0.0f;
    camera_angle_v = 0.3f;
    camera_target = Vector3(0, 0, 0);
    zoom_level = 1.0f;
    view_center = sf::Vector2f(0, 0);
    view.setCenter(view_center);
    view.setSize(sf::Vector2f(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT));
    window.setView(view);
    update_camera_position();
}

void GraphRenderer::calculate_graph_bounds(const Graph3D& graph, Vector3& min_bounds, Vector3& max_bounds) {
    if (graph.node_count == 0) {
        min_bounds = max_bounds = Vector3(0, 0, 0);
        return;
    }
    
    min_bounds = max_bounds = graph.nodes[0].position;
    
    for (uint32_t i = 1; i < graph.node_count; i++) {
        const Vector3& pos = graph.nodes[i].position;
        
        min_bounds.x = std::min(min_bounds.x, pos.x);
        min_bounds.y = std::min(min_bounds.y, pos.y);
        min_bounds.z = std::min(min_bounds.z, pos.z);
        
        max_bounds.x = std::max(max_bounds.x, pos.x);
        max_bounds.y = std::max(max_bounds.y, pos.y);
        max_bounds.z = std::max(max_bounds.z, pos.z);
    }
    
    // Set camera to view entire graph
    Vector3 center = (min_bounds + max_bounds) * 0.5f;
    Vector3 size = max_bounds - min_bounds;
    float max_dimension = std::max({size.x, size.y, size.z});
    
    camera_target = center;
    camera_distance = std::min(max_dimension * 1.5f + 10.0f, 100.0f); // Closer camera, max distance cap
    update_camera_position();
}

void GraphRenderer::render_frame(const Graph3D& graph, const Pixels& overlay) {
    window.clear(sf::Color::Black);
    
    // Auto-adjust camera to fit graph on first frame
    static bool first_frame = true;
    if (first_frame && graph.node_count > 0) {
        Vector3 min_bounds, max_bounds;
        calculate_graph_bounds(graph, min_bounds, max_bounds);
        std::cout << "Graph bounds: (" << min_bounds.x << "," << min_bounds.y << "," << min_bounds.z 
                  << ") to (" << max_bounds.x << "," << max_bounds.y << "," << max_bounds.z << ")" << std::endl;
        std::cout << "Camera distance set to: " << camera_distance << std::endl;
        first_frame = false;
    }
    
    draw_grid();
    
    // Draw edges with 3D perspective
    std::vector<sf::Vertex> edge_vertices;
    
    // Debug: Print edge count
    static bool printed_debug = false;
    if (!printed_debug) {
        std::cout << "Rendering " << graph.edge_count << " edges" << std::endl;
        printed_debug = true;
    }
    
    for (uint32_t i = 0; i < graph.edge_count; i++) {
        const GraphEdge& edge = graph.edges[i];
        if (!edge.visible) continue;
        
        const GraphNode& from_node = graph.nodes[edge.from_id];
        const GraphNode& to_node = graph.nodes[edge.to_id];
        
        sf::Vector2f from_pos = world_to_screen_3d(from_node.position);
        sf::Vector2f to_pos = world_to_screen_3d(to_node.position);
        
        // Make edges visible with good contrast and thickness
        sf::Color edge_color(200, 200, 200, 180); // Light gray, semi-transparent
        
        // Draw multiple lines for thickness
        for (int offset = -1; offset <= 1; offset++) {
            sf::Vector2f offset_from = from_pos + sf::Vector2f(offset, 0);
            sf::Vector2f offset_to = to_pos + sf::Vector2f(offset, 0);
            
            sf::Vertex v1, v2;
            v1.position = offset_from;
            v1.color = edge_color;
            v2.position = offset_to;
            v2.color = edge_color;
            edge_vertices.push_back(v1);
            edge_vertices.push_back(v2);
            
            // Also add vertical offset for extra thickness
            if (offset == 0) {
                sf::Vector2f v_offset_from = from_pos + sf::Vector2f(0, 1);
                sf::Vector2f v_offset_to = to_pos + sf::Vector2f(0, 1);
                
                sf::Vertex v3, v4;
                v3.position = v_offset_from;
                v3.color = edge_color;
                v4.position = v_offset_to;
                v4.color = edge_color;
                edge_vertices.push_back(v3);
                edge_vertices.push_back(v4);
            }
        }
    }
    
    if (!edge_vertices.empty()) {
        window.draw(edge_vertices.data(), edge_vertices.size(), sf::PrimitiveType::Lines);
    }
    
    // Draw nodes with 3D perspective and depth-based sizing
    for (uint32_t i = 0; i < graph.node_count; i++) {
        const GraphNode& node = graph.nodes[i];
        if (!node.visible) continue;
        
        sf::Vector2f screen_pos = world_to_screen_3d(node.position);
        
        // Calculate depth for perspective scaling
        Vector3 relative_pos = node.position - camera_position;
        Vector3 forward = (camera_target - camera_position).normalize();
        float depth = relative_pos.x * forward.x + relative_pos.y * forward.y + relative_pos.z * forward.z;
        
        float perspective_scale = apply_perspective(depth);
        float visual_radius = node.radius * perspective_scale * 0.5f; // Scale down for better visibility
        visual_radius = std::max(2.0f, std::min(visual_radius, 50.0f)); // Clamp radius
        
        sf::CircleShape circle(visual_radius);
        circle.setFillColor(sf::Color(node.color.r, node.color.g, node.color.b, node.color.a));
        circle.setOutlineThickness(1.0f);
        circle.setOutlineColor(sf::Color::White);
        circle.setPosition(sf::Vector2f(screen_pos.x - circle.getRadius(), screen_pos.y - circle.getRadius()));
        
        window.draw(circle);
    }
    
    // Draw overlay if provided
    if (!overlay.is_empty()) {
        sf::Texture overlay_texture = pixels_to_sfml_texture(overlay);
        sf::Sprite overlay_sprite(overlay_texture);
        
        // Position overlay in screen space (not affected by view transforms)
        sf::View original_view = window.getView();
        window.setView(window.getDefaultView());
        
        sf::Vector2u window_size = window.getSize();
        sf::Vector2u texture_size = overlay_texture.getSize();
        overlay_sprite.setPosition(sf::Vector2f(static_cast<float>(window_size.x - texture_size.x - 10), 10));
        window.draw(overlay_sprite);
        
        window.setView(original_view);
    }
    
    window.display();
}

bool GraphRenderer::should_close() {
    return !window.isOpen();
}

void GraphRenderer::cleanup() {
    if (window.isOpen()) {
        window.close();
    }
}

// Utility functions for swaptube Pixels integration
sf::Texture pixels_to_sfml_texture(const Pixels& pixels) {
    sf::Texture texture;
    if (pixels.w == 0 || pixels.h == 0) return texture;
    
    // Create RGBA pixel data for SFML
    std::vector<uint8_t> sfml_pixels(pixels.w * pixels.h * 4);
    
    for (int i = 0; i < pixels.w * pixels.h; i++) {
        unsigned int pixel = pixels.pixels[i];
        sfml_pixels[i * 4 + 0] = (pixel >> 16) & 0xFF; // R
        sfml_pixels[i * 4 + 1] = (pixel >> 8) & 0xFF;  // G
        sfml_pixels[i * 4 + 2] = (pixel) & 0xFF;       // B
        sfml_pixels[i * 4 + 3] = (pixel >> 24) & 0xFF; // A
    }
    
    if (!texture.resize(sf::Vector2u(pixels.w, pixels.h))) return texture;
    texture.update(sfml_pixels.data());
    
    return texture;
}

void update_sfml_texture_from_pixels(sf::Texture& texture, const Pixels& pixels) {
    if (pixels.w == 0 || pixels.h == 0) return;
    if (texture.getSize().x != (unsigned int)pixels.w || texture.getSize().y != (unsigned int)pixels.h) return;
    
    // Create RGBA pixel data for SFML
    std::vector<uint8_t> sfml_pixels(pixels.w * pixels.h * 4);
    
    for (int i = 0; i < pixels.w * pixels.h; i++) {
        unsigned int pixel = pixels.pixels[i];
        sfml_pixels[i * 4 + 0] = (pixel >> 16) & 0xFF; // R
        sfml_pixels[i * 4 + 1] = (pixel >> 8) & 0xFF;  // G
        sfml_pixels[i * 4 + 2] = (pixel) & 0xFF;       // B
        sfml_pixels[i * 4 + 3] = (pixel >> 24) & 0xFF; // A
    }
    
    texture.update(sfml_pixels.data());
}