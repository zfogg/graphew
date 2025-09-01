#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <array>
#include <vector>
#include <string>
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
    float render_dimension = 3.0f; // 1..3 for render-space scaling
    
    GraphRenderer();
    ~GraphRenderer();
    
    void init_window(const std::string& title);
    void update_camera();
    void render_frame(const Graph3D& graph, const Pixels& overlay = Pixels());
    bool should_close();
    void cleanup();
    bool window_has_focus() const { return window.hasFocus(); }
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
    
    // Expose UI drawing for sliders
    void draw_ui_sliders();
    
    // Simple UI sliders
    struct UISlider {
        std::string label;
        float* target;
        float min_value;
        float max_value;
        float last_value;
        struct RectPx { float left, top, width, height; } rect_px; // in screen pixels
        bool dragging;
        UISlider() : target(nullptr), min_value(0), max_value(1), last_value(0), rect_px(), dragging(false) {}
    };
    
    struct UICheckbox {
        std::string label;
        bool* target;
        bool last_value;
        struct RectPx { float left, top, width, height; } rect_px;
        UICheckbox() : target(nullptr), last_value(false), rect_px() {}
    };
    
    void clear_sliders();
    void add_slider(const std::string& label, float* target, float min_value, float max_value);
    
    // Checkbox methods
    void clear_checkboxes();
    void add_checkbox(const std::string& label, bool* target);
    void draw_ui_checkboxes();
    
private:
    void handle_events();
    void handle_ui_event(const sf::Event& ev);
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
    std::vector<UISlider> ui_sliders;
    void layout_ui_sliders();
    
    // Checkboxes
    std::vector<UICheckbox> ui_checkboxes;
    void layout_ui_checkboxes();
    void handle_checkbox_event(const sf::Event& ev);
    float slider_panel_x = 50.0f;
    float slider_panel_y = 80.0f;
    float slider_panel_padding = 12.0f;
    float slider_track_width = 260.0f;
    float slider_track_height = 12.0f;
    float slider_vertical_spacing = 38.0f;
    bool ui_mouse_captured = false;
    bool is_mouse_over_ui(const sf::Vector2f& mps) const;
    


    // Render-dimension helpers
    inline float axis_weight_render(float dimension, float axisIndex) const {
        float t = std::min(1.0f, std::max(0.0f, dimension - axisIndex));
        return t; // for pure 2D, axis weight becomes 0
    }
    inline Vector3 scale_for_render(const Vector3& v) const {
        float wy = axis_weight_render(render_dimension, 1.0f);
        float wz = axis_weight_render(render_dimension, 2.0f);
        return Vector3(v.x, v.y * wy, v.z * wz);
    }
public:
    void set_render_dimension(float d) { render_dimension = std::max(1.0f, std::min(3.0f, d)); }
    float get_render_dimension() const { return render_dimension; }
    

};

sf::Texture pixels_to_sfml_texture(const Pixels& pixels);
void update_sfml_texture_from_pixels(sf::Texture& texture, const Pixels& pixels);