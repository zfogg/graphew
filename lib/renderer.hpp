#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <array>
#include "graph.hpp"
#include "swaptube_pixels.hpp"

#define DEFAULT_SCREEN_WIDTH 1920
#define DEFAULT_SCREEN_HEIGHT 1080
#define TARGET_FPS 120

// Camera preset structure
struct CameraPreset {
    Vector3 position;
    Vector3 target;
    float angle_h;
    float angle_v;
    float distance;
    float fov;
    bool valid;
    
    CameraPreset() : position(0,0,15), target(0,0,0), angle_h(0), angle_v(0.3f), 
                     distance(15.0f), fov(60.0f), valid(false) {}
};

// Lighting parameters
struct LightingParams {
    Vector3 directional_light_dir;
    float directional_intensity;
    float ambient_intensity;
    sf::Color light_color;
    bool shadows_enabled;
    float fog_density;
    float fog_start;
    float fog_end;
    // Advanced shading
    float specular_intensity;   // strength of specular highlight
    float shininess;            // specular exponent
    float rim_intensity;        // rim (Fresnel-like) intensity
    float contour_intensity;    // strength of height contour effect
    float contour_frequency;    // frequency of contour bands
    float contour_offset;       // phase/offset of contour bands
    
    LightingParams() : directional_light_dir(0.5f, -0.7f, 0.5f), 
                       directional_intensity(0.8f),
                       ambient_intensity(0.3f),
                       light_color(255, 245, 220),  // Warm white
                       shadows_enabled(false),
                       fog_density(0.02f),
                       fog_start(10.0f),
                       fog_end(50.0f),
                       specular_intensity(0.35f),
                       shininess(32.0f),
                       rim_intensity(0.25f),
                       contour_intensity(0.15f),
                       contour_frequency(0.25f),
                       contour_offset(0.0f) {}
};

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
    
    // Enhanced camera controls
    float camera_move_speed;
    float camera_rotate_speed;
    float field_of_view;
    bool smooth_camera;
    Vector3 camera_velocity;
    
    // Camera presets (0-9 keys)
    std::array<CameraPreset, 10> camera_presets;
    
    // Lighting system
    LightingParams lighting;
    bool show_axes;
    bool show_grid;
    Vector3 scene_center;
    
    // UI font
    sf::Font ui_font;
    bool ui_font_loaded;
    
    GraphRenderer();
    ~GraphRenderer();
    
    void init_window(const std::string& title);
    void update_camera();
    void render_frame(const Graph3D& graph, const Pixels& overlay = Pixels());
    bool should_close();
    void cleanup();
    void reset_camera();
    void calculate_graph_bounds(const Graph3D& graph, Vector3& min_bounds, Vector3& max_bounds);
    
    // Camera preset management
    void save_camera_preset(int slot);
    void load_camera_preset(int slot);
    
    // Lighting controls
    void adjust_lighting(float ambient_delta, float directional_delta);
    void rotate_light(float horizontal, float vertical);
    void toggle_shadows() { lighting.shadows_enabled = !lighting.shadows_enabled; }
    void set_fog_density(float density) { lighting.fog_density = density; }
    
    // Help overlay
    Pixels create_help_overlay();
    bool show_help;
    
private:
    void handle_events();
    void handle_camera_movement(float delta_time);
    void draw_grid();
    void draw_axes();
    void update_camera_position();
    sf::Vector2f world_to_screen_3d(const Vector3& world_pos);
    float apply_perspective(float z_depth);
    sf::Color apply_lighting(const Vector3& position, const Vector3& normal, const sf::Color& base_color);
    sf::Color apply_fog(const sf::Color& color, float depth);
    float calculate_depth_shade(float depth);
    void draw_3d_line(const Vector3& start, const Vector3& end, const sf::Color& color, float thickness = 1.0f);
    void draw_3d_sphere(const Vector3& center, float radius, const sf::Color& color);
    void load_ui_font();
    void draw_help_overlay_sfml();
};

sf::Texture pixels_to_sfml_texture(const Pixels& pixels);
void update_sfml_texture_from_pixels(sf::Texture& texture, const Pixels& pixels);