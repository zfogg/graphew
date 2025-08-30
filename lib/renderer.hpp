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
    
    // 3D camera parameters
    Vector3 camera_position;
    Vector3 camera_target;
    float camera_distance;
    float camera_angle_h;    // Horizontal rotation
    float camera_angle_v;    // Vertical rotation
    bool auto_rotate;
    float auto_rotate_speed;
    sf::Clock rotation_clock;
    
    GraphRenderer();
    ~GraphRenderer();
    
    void init_window(const std::string& title);
    void update_camera();
    void render_frame(const Graph3D& graph, const Pixels& overlay = Pixels());
    bool should_close();
    void cleanup();
    void reset_camera();
    void calculate_graph_bounds(const Graph3D& graph, Vector3& min_bounds, Vector3& max_bounds);
    
private:
    void handle_events();
    void draw_grid();
    void update_camera_position();
    sf::Vector2f world_to_screen_3d(const Vector3& world_pos);
    float apply_perspective(float z_depth);
    void draw_3d_line(const Vector3& start, const Vector3& end, const sf::Color& color, float thickness = 1.0f);
    void draw_3d_sphere(const Vector3& center, float radius, const sf::Color& color);
};

sf::Texture pixels_to_sfml_texture(const Pixels& pixels);
void update_sfml_texture_from_pixels(sf::Texture& texture, const Pixels& pixels);