#include "renderer.hpp"
#include <cmath>
#include <iostream>

GraphRenderer::GraphRenderer() 
    : zoom_level(1.0f), zoom_speed(0.1f), view_center(0, 0) {
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
                // Reset view
                zoom_level = 1.0f;
                view_center = sf::Vector2f(0, 0);
                view.setCenter(view_center);
                view.setSize(sf::Vector2f(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT));
                window.setView(view);
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

sf::Vector2f GraphRenderer::world_to_screen(const Vector3& world_pos) {
    // Simple orthographic projection (ignoring Z for 2D display)
    float scale = 50.0f; // Scale factor to make graph visible
    return sf::Vector2f(world_pos.x * scale, -world_pos.y * scale); // Flip Y for screen coordinates
}

void GraphRenderer::render_frame(const Graph3D& graph, const Pixels& overlay) {
    window.clear(sf::Color::Black);
    
    draw_grid();
    
    // Draw edges first (so they appear behind nodes)
    std::vector<sf::Vertex> edge_vertices;
    for (uint32_t i = 0; i < graph.edge_count; i++) {
        const GraphEdge& edge = graph.edges[i];
        if (!edge.visible) continue;
        
        const GraphNode& from_node = graph.nodes[edge.from_id];
        const GraphNode& to_node = graph.nodes[edge.to_id];
        
        sf::Vector2f from_pos = world_to_screen(from_node.position);
        sf::Vector2f to_pos = world_to_screen(to_node.position);
        
        sf::Color edge_color(edge.color.r, edge.color.g, edge.color.b, edge.color.a);
        sf::Vertex v1, v2;
        v1.position = from_pos;
        v1.color = edge_color;
        v2.position = to_pos;
        v2.color = edge_color;
        edge_vertices.push_back(v1);
        edge_vertices.push_back(v2);
    }
    
    if (!edge_vertices.empty()) {
        window.draw(edge_vertices.data(), edge_vertices.size(), sf::PrimitiveType::Lines);
    }
    
    // Draw nodes
    for (uint32_t i = 0; i < graph.node_count; i++) {
        const GraphNode& node = graph.nodes[i];
        if (!node.visible) continue;
        
        sf::Vector2f screen_pos = world_to_screen(node.position);
        
        sf::CircleShape circle(node.radius * 25.0f); // Scale radius for visibility
        circle.setFillColor(sf::Color(node.color.r, node.color.g, node.color.b, node.color.a));
        circle.setOutlineThickness(2.0f);
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