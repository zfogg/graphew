#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include "graph.hpp"
#include "swaptube_pixels.hpp"

#define DEFAULT_SCREEN_WIDTH 1920
#define DEFAULT_SCREEN_HEIGHT 1080
#define TARGET_FPS 120

class GraphRenderer {
public:
    sf::RenderWindow window;
    sf::View view;
    sf::Vector2f last_mouse_position;
    float zoom_level;
    float zoom_speed;
    sf::Vector2f view_center;
    
    GraphRenderer();
    ~GraphRenderer();
    
    void init_window(const std::string& title);
    void update_camera();
    void render_frame(const Graph3D& graph, const Pixels& overlay = Pixels());
    bool should_close();
    void cleanup();
    
private:
    void handle_events();
    void draw_grid();
    sf::Vector2f world_to_screen(const Vector3& world_pos);
};

sf::Texture pixels_to_sfml_texture(const Pixels& pixels);
void update_sfml_texture_from_pixels(sf::Texture& texture, const Pixels& pixels);