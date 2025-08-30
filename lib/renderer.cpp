#include "renderer.hpp"
#include <cmath>
#include <iostream>

GraphRenderer::GraphRenderer() 
    : zoom_level(1.0f), zoom_speed(0.1f), view_center(0, 0),
      camera_position(0, 0, 15), camera_target(0, 0, 0), camera_distance(15.0f),
      camera_angle_h(0.0f), camera_angle_v(0.3f), auto_rotate(false), auto_rotate_speed(0.3f),
      camera_move_speed(10.0f), camera_rotate_speed(2.0f), field_of_view(60.0f),
      smooth_camera(true), camera_velocity(0, 0, 0), show_axes(false), show_grid(true),
      scene_center(0, 0, 0),
      ui_font_loaded(false),
      show_help(false) {
    // Initialize default camera presets
    camera_presets[0].position = Vector3(15, 10, 15);
    camera_presets[0].target = Vector3(0, 0, 0);
    camera_presets[0].angle_h = 0.785f;  // 45 degrees
    camera_presets[0].angle_v = 0.5f;
    camera_presets[0].distance = 25.0f;
    camera_presets[0].fov = 60.0f;
    camera_presets[0].valid = true;
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
    load_ui_font();
    clear_sliders();
}
void GraphRenderer::clear_sliders() {
    ui_sliders.clear();
}

void GraphRenderer::add_slider(const std::string& label, float* target, float min_value, float max_value) {
    UISlider s;
    s.label = label;
    s.target = target;
    s.min_value = min_value;
    s.max_value = max_value;
    s.last_value = target ? *target : 0.0f;
    s.rect_px = {0.f, 0.f, 0.f, 0.f};
    s.dragging = false;
    ui_sliders.push_back(s);
}

void GraphRenderer::layout_ui_sliders() {
    float x = slider_panel_x + slider_panel_padding;
    float y = slider_panel_y + slider_panel_padding + 22.0f;
    for (size_t i = 0; i < ui_sliders.size(); ++i) {
        ui_sliders[i].rect_px = {x, y + static_cast<float>(i) * slider_vertical_spacing, slider_track_width, slider_track_height};
    }
}

bool GraphRenderer::is_mouse_over_ui(const sf::Vector2f& mps) const {
    float panel_w = slider_track_width + slider_panel_padding * 2.0f;
    float panel_h = slider_panel_padding * 2.0f + 22.0f + static_cast<float>(ui_sliders.size()) * slider_vertical_spacing;
    return mps.x >= slider_panel_x && mps.x <= slider_panel_x + panel_w &&
           mps.y >= slider_panel_y && mps.y <= slider_panel_y + panel_h;
}

void GraphRenderer::handle_ui_event(const sf::Event& ev) {
    if (ui_sliders.empty()) return;
    if (!window.hasFocus()) return;
    layout_ui_sliders();
    
    if (auto* pressed = ev.getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2i mp = sf::Mouse::getPosition(window);
        sf::Vector2f mps(static_cast<float>(mp.x), static_cast<float>(mp.y));
        ui_mouse_captured = is_mouse_over_ui(mps);
        for (auto& s : ui_sliders) {
            if (mps.x >= s.rect_px.left && mps.x <= s.rect_px.left + s.rect_px.width &&
                mps.y >= s.rect_px.top && mps.y <= s.rect_px.top + s.rect_px.height) {
                s.dragging = true;
                if (s.target) {
                    float t = (mps.x - s.rect_px.left) / s.rect_px.width;
                    t = std::max(0.0f, std::min(1.0f, t));
                    *s.target = s.min_value + t * (s.max_value - s.min_value);
                }
            }
        }
    } else if (auto* moved = ev.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2i mp = sf::Mouse::getPosition(window);
        sf::Vector2f mps(static_cast<float>(mp.x), static_cast<float>(mp.y));
        for (auto& s : ui_sliders) {
            if (s.dragging && s.target) {
                float t = (mps.x - s.rect_px.left) / s.rect_px.width;
                t = std::max(0.0f, std::min(1.0f, t));
                *s.target = s.min_value + t * (s.max_value - s.min_value);
            }
        }
    } else if (auto* released = ev.getIf<sf::Event::MouseButtonReleased>()) {
        for (auto& s : ui_sliders) s.dragging = false;
        ui_mouse_captured = false;
    }
}

void GraphRenderer::draw_ui_sliders() {
    if (ui_sliders.empty()) return;
    sf::View original_view = window.getView();
    window.setView(window.getDefaultView());
    
    layout_ui_sliders();
    // Background panel
    float panel_w = slider_track_width + slider_panel_padding * 2.0f;
    float panel_h = slider_panel_padding * 2.0f + 22.0f + static_cast<float>(ui_sliders.size()) * slider_vertical_spacing;
    sf::RectangleShape panel(sf::Vector2f(panel_w, panel_h));
    panel.setPosition(sf::Vector2f(slider_panel_x, slider_panel_y));
    panel.setFillColor(sf::Color(20, 20, 26, 180));
    panel.setOutlineThickness(1.0f);
    panel.setOutlineColor(sf::Color(90, 90, 120, 200));
    window.draw(panel);
    
    for (const auto& s : ui_sliders) {
        // Track (thicker)
        sf::RectangleShape track(sf::Vector2f(s.rect_px.width, s.rect_px.height));
        track.setPosition(sf::Vector2f(s.rect_px.left, s.rect_px.top));
        track.setFillColor(sf::Color(82, 90, 130, 220));
        window.draw(track);
        
        // Thumb based on value
        if (s.target) {
            float t = (*s.target - s.min_value) / (s.max_value - s.min_value);
            t = std::max(0.0f, std::min(1.0f, t));
            float x = s.rect_px.left + t * s.rect_px.width;
            sf::RectangleShape thumb(sf::Vector2f(10.f, s.rect_px.height + 10.f));
            thumb.setOrigin(sf::Vector2f(5.f, 5.f));
            thumb.setPosition(sf::Vector2f(x, s.rect_px.top + s.rect_px.height * 0.5f));
            thumb.setFillColor(sf::Color(240, 240, 255, 245));
            window.draw(thumb);
        }
        
        // Label
        if (ui_font_loaded && s.target) {
            sf::Text txt(ui_font);
            char buf[128];
            std::snprintf(buf, sizeof(buf), "%s: %.2f", s.label.c_str(), *s.target);
            txt.setString(buf);
            txt.setCharacterSize(16);
            txt.setFillColor(sf::Color(220, 220, 235, 240));
            txt.setPosition(sf::Vector2f(s.rect_px.left, s.rect_px.top - 22.f));
            window.draw(txt);
        }
    }
    window.setView(original_view);
}

void GraphRenderer::handle_events() {
    std::optional<sf::Event> event;
    while ((event = window.pollEvent())) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        else if (auto* ev = event->getIf<sf::Event::MouseButtonPressed>()) {
            handle_ui_event(*event);
        }
        else if (auto* ev = event->getIf<sf::Event::MouseButtonReleased>()) {
            handle_ui_event(*event);
        }
        else if (auto* ev = event->getIf<sf::Event::MouseMoved>()) {
            handle_ui_event(*event);
        }
        else if (auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            float delta = mouseWheel->delta;
            
            // Shift+scroll adjusts FOV
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
                field_of_view = std::max(20.0f, std::min(120.0f, field_of_view - delta * 5.0f));
                std::cout << "FOV: " << field_of_view << "°" << std::endl;
            }
            // Ctrl+scroll adjusts camera speed
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                camera_move_speed = std::max(1.0f, std::min(50.0f, camera_move_speed + delta * 2.0f));
                std::cout << "Camera speed: " << camera_move_speed << std::endl;
            }
            // Normal scroll for zoom
            else {
                camera_distance = std::max(5.0f, std::min(100.0f, camera_distance - delta * 2.0f));
            }
        }
        else if (auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
            // Camera presets (0-9)
            if (keyPress->code == sf::Keyboard::Key::Num0) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(0);
                } else {
                    load_camera_preset(0);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num1) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(1);
                } else {
                    load_camera_preset(1);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num2) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(2);
                } else {
                    load_camera_preset(2);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num3) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(3);
                } else {
                    load_camera_preset(3);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num4) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(4);
                } else {
                    load_camera_preset(4);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num5) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(5);
                } else {
                    load_camera_preset(5);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num6) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(6);
                } else {
                    load_camera_preset(6);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num7) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(7);
                } else {
                    load_camera_preset(7);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num8) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(8);
                } else {
                    load_camera_preset(8);
                }
            }
            else if (keyPress->code == sf::Keyboard::Key::Num9) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                    save_camera_preset(9);
                } else {
                    load_camera_preset(9);
                }
            }
            // R key is handled by main application for graph reset
            // (removed camera reset from here)
            else if (keyPress->code == sf::Keyboard::Key::Space) {
                auto_rotate = !auto_rotate;
                std::cout << "Auto-rotation " << (auto_rotate ? "enabled" : "disabled") << std::endl;
            }
            else if (keyPress->code == sf::Keyboard::Key::G) {
                show_grid = !show_grid;
                std::cout << "Grid " << (show_grid ? "shown" : "hidden") << std::endl;
            }
            else if (keyPress->code == sf::Keyboard::Key::X) {
                show_axes = !show_axes;
                std::cout << "Axes " << (show_axes ? "shown" : "hidden") << std::endl;
            }
            else if (keyPress->code == sf::Keyboard::Key::H) {
                show_help = !show_help;
                std::cout << "Help " << (show_help ? "shown" : "hidden") << std::endl;
            }
            else if (keyPress->code == sf::Keyboard::Key::F) {
                lighting.fog_density = (lighting.fog_density > 0) ? 0.0f : 0.02f;
                std::cout << "Fog " << ((lighting.fog_density > 0) ? "enabled" : "disabled") << std::endl;
            }
            else if (keyPress->code == sf::Keyboard::Key::S && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
                toggle_shadows();
                std::cout << "Shadows " << (lighting.shadows_enabled ? "enabled" : "disabled") << std::endl;
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
    
    float delta_time = rotation_clock.restart().asSeconds();
    
    // Auto-rotation for full graph visibility
    if (auto_rotate) {
        camera_angle_h += auto_rotate_speed * delta_time;
        // Gentle vertical oscillation
        camera_angle_v = 0.3f + 0.2f * std::sin(rotation_clock.getElapsedTime().asSeconds() * 0.3f);
    } else {
        // Manual camera movement
        handle_camera_movement(delta_time);
    }
    
    // Lighting controls
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I)) {
        adjust_lighting(0.01f, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
        adjust_lighting(-0.01f, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
        adjust_lighting(0, 0.01f);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J)) {
        adjust_lighting(0, -0.01f);
    }
    
    // Light rotation with numpad
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad4)) {
        rotate_light(-delta_time, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad6)) {
        rotate_light(delta_time, 0);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad8)) {
        rotate_light(0, delta_time);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Numpad2)) {
        rotate_light(0, -delta_time);
    }
    
    update_camera_position();
}

void GraphRenderer::draw_grid() {
    if (!show_grid) return;
    
    const float grid_size = 5.0f;  // Smaller grid for better scale
    const int grid_lines = 20;     // Fewer lines to reduce clutter
    const sf::Color grid_color(50, 50, 50, 80);
    
    // Center grid on the scene center (graph data center)
    Vector3 grid_center = scene_center;
    
    // Create vertex arrays for grid lines
    std::vector<sf::Vertex> vertices;
    
    // Draw vertical lines (X-Z plane)
    for (int i = -grid_lines; i <= grid_lines; i++) {
        Vector3 start(grid_center.x + i * grid_size, grid_center.y, grid_center.z - grid_lines * grid_size);
        Vector3 end(grid_center.x + i * grid_size, grid_center.y, grid_center.z + grid_lines * grid_size);
        
        sf::Vector2f start_2d = world_to_screen_3d(start);
        sf::Vector2f end_2d = world_to_screen_3d(end);
        
        sf::Vertex v1, v2;
        v1.position = start_2d;
        v1.color = grid_color;
        v2.position = end_2d;
        v2.color = grid_color;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    // Draw horizontal lines (X-Z plane)
    for (int i = -grid_lines; i <= grid_lines; i++) {
        Vector3 start(grid_center.x - grid_lines * grid_size, grid_center.y, grid_center.z + i * grid_size);
        Vector3 end(grid_center.x + grid_lines * grid_size, grid_center.y, grid_center.z + i * grid_size);
        
        sf::Vector2f start_2d = world_to_screen_3d(start);
        sf::Vector2f end_2d = world_to_screen_3d(end);
        
        sf::Vertex v1, v2;
        v1.position = start_2d;
        v1.color = grid_color;
        v2.position = end_2d;
        v2.color = grid_color;
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    
    if (!vertices.empty()) {
        window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
    }
}

void GraphRenderer::draw_axes() {
    if (!show_axes) return;
    
    // Draw XYZ axes
    const float axis_length = 100.0f;
    
    // X axis - Red
    Vector3 x_start(0, 0, 0);
    Vector3 x_end(axis_length, 0, 0);
    sf::Vector2f x_start_2d = world_to_screen_3d(x_start);
    sf::Vector2f x_end_2d = world_to_screen_3d(x_end);
    
    // Y axis - Green  
    Vector3 y_start(0, 0, 0);
    Vector3 y_end(0, axis_length, 0);
    sf::Vector2f y_start_2d = world_to_screen_3d(y_start);
    sf::Vector2f y_end_2d = world_to_screen_3d(y_end);
    
    // Z axis - Blue
    Vector3 z_start(0, 0, 0);
    Vector3 z_end(0, 0, axis_length);
    sf::Vector2f z_start_2d = world_to_screen_3d(z_start);
    sf::Vector2f z_end_2d = world_to_screen_3d(z_end);
    
    // Draw axes
    std::vector<sf::Vertex> vertices;
    
    // X axis
    sf::Vertex vx1, vx2;
    vx1.position = x_start_2d;
    vx1.color = sf::Color::Red;
    vx2.position = x_end_2d;
    vx2.color = sf::Color::Red;
    vertices.push_back(vx1);
    vertices.push_back(vx2);
    
    // Y axis
    sf::Vertex vy1, vy2;
    vy1.position = y_start_2d;
    vy1.color = sf::Color::Green;
    vy2.position = y_end_2d;
    vy2.color = sf::Color::Green;
    vertices.push_back(vy1);
    vertices.push_back(vy2);
    
    // Z axis
    sf::Vertex vz1, vz2;
    vz1.position = z_start_2d;
    vz1.color = sf::Color::Blue;
    vz2.position = z_end_2d;
    vz2.color = sf::Color::Blue;
    vertices.push_back(vz1);
    vertices.push_back(vz2);
    
    window.draw(vertices.data(), vertices.size(), sf::PrimitiveType::Lines);
}

void GraphRenderer::handle_camera_movement(float delta_time) {
    // Camera rotation with arrow keys or mouse right-drag
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
        sf::Vector2f current_mouse = static_cast<sf::Vector2f>(mouse_pos);
        
        if (last_mouse_position.x != 0 || last_mouse_position.y != 0) {
            sf::Vector2f delta = current_mouse - last_mouse_position;
            camera_angle_h += delta.x * 0.01f * camera_rotate_speed;
            camera_angle_v -= delta.y * 0.01f * camera_rotate_speed;
            camera_angle_v = std::max(-1.5f, std::min(1.5f, camera_angle_v));
        }
        
        last_mouse_position = current_mouse;
    } else if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        last_mouse_position = sf::Vector2f(0, 0);
    }
    
    // Keyboard rotation
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        camera_angle_h -= camera_rotate_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        camera_angle_h += camera_rotate_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        camera_angle_v = std::min(1.5f, camera_angle_v + camera_rotate_speed * delta_time);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        camera_angle_v = std::max(-1.5f, camera_angle_v - camera_rotate_speed * delta_time);
    }
    
    // WASD movement relative to camera direction
    Vector3 movement(0, 0, 0);
    Vector3 forward = (camera_target - camera_position).normalize();
    Vector3 right = Vector3(forward.z, 0, -forward.x).normalize();
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        movement = movement + forward * camera_move_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        movement = movement - forward * camera_move_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        movement = movement - right * camera_move_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        movement = movement + right * camera_move_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
        movement.y -= camera_move_speed * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
        movement.y += camera_move_speed * delta_time;
    }
    
    // Apply movement with smoothing
    if (smooth_camera) {
        camera_velocity = camera_velocity * 0.9f + movement * 0.1f;
        camera_target = camera_target + camera_velocity;
    } else {
        camera_target = camera_target + movement;
    }
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
    float focal_length = DEFAULT_SCREEN_HEIGHT / (2.0f * std::tan(field_of_view * M_PI / 360.0f));
    
    // Prevent division by zero and clamp minimum distance
    float safe_z = std::max(z_depth, 0.1f);
    return focal_length / safe_z * zoom_level;
}

sf::Color GraphRenderer::apply_lighting(const Vector3& /* position */, const Vector3& normal, const sf::Color& base_color) {
    // Lighting is world-relative: use a fixed world-space light direction
    Vector3 light_dir = lighting.directional_light_dir.normalize();
    
    // Calculate diffuse lighting (Lambertian)
    float dot_product = std::max(0.0f, -(normal.x * light_dir.x + normal.y * light_dir.y + normal.z * light_dir.z));
    float diffuse = lighting.directional_intensity * dot_product;
    
    // View direction assumed towards camera for simple specular/rim
    Vector3 view_dir = Vector3(0, 0, 1); // approximate in screen space; fine for stylized shading
    
    // Blinn-Phong specular highlight
    Vector3 half_vec = (light_dir + view_dir).normalize();
    float spec_angle = std::max(0.0f, -(normal.x * half_vec.x + normal.y * half_vec.y + normal.z * half_vec.z));
    float specular = lighting.specular_intensity * std::pow(spec_angle, lighting.shininess);
    
    // Rim lighting (Fresnel-like) enhances silhouettes
    float ndotv = std::max(0.0f, -(normal.x * view_dir.x + normal.y * view_dir.y + normal.z * view_dir.z));
    float rim = lighting.rim_intensity * std::pow(1.0f - ndotv, 2.0f);
    
    // Total light = ambient + diffuse + specular + rim
    float total_light = std::min(1.5f, lighting.ambient_intensity + diffuse + specular + rim);
    
    // Apply light color tint
    float r = base_color.r * total_light * (lighting.light_color.r / 255.0f);
    float g = base_color.g * total_light * (lighting.light_color.g / 255.0f);
    float b = base_color.b * total_light * (lighting.light_color.b / 255.0f);
    
    return sf::Color(
        static_cast<unsigned char>(std::min(255.0f, r)),
        static_cast<unsigned char>(std::min(255.0f, g)),
        static_cast<unsigned char>(std::min(255.0f, b)),
        base_color.a
    );
}

sf::Color GraphRenderer::apply_fog(const sf::Color& color, float depth) {
    if (lighting.fog_density <= 0) return color;
    
    // Calculate fog factor based on depth
    float fog_factor = 0.0f;
    if (depth < lighting.fog_start) {
        fog_factor = 0.0f;
    } else if (depth > lighting.fog_end) {
        fog_factor = 1.0f;
    } else {
        fog_factor = (depth - lighting.fog_start) / (lighting.fog_end - lighting.fog_start);
        fog_factor = std::pow(fog_factor, 2.0f); // Quadratic fog
    }
    
    fog_factor *= lighting.fog_density;
    fog_factor = std::min(1.0f, fog_factor);
    
    // Fog color (slightly bluish-gray)
    sf::Color fog_color(100, 110, 120);
    
    return sf::Color(
        color.r * (1 - fog_factor) + fog_color.r * fog_factor,
        color.g * (1 - fog_factor) + fog_color.g * fog_factor,
        color.b * (1 - fog_factor) + fog_color.b * fog_factor,
        color.a
    );
}

float GraphRenderer::calculate_depth_shade(float depth) {
    // Create depth-based shading for better 3D perception
    float normalized_depth = (depth - 5.0f) / 50.0f; // Normalize to 0-1 range
    normalized_depth = std::max(0.0f, std::min(1.0f, normalized_depth));
    
    // Darker when further away
    return 1.0f - normalized_depth * 0.5f;
}

void GraphRenderer::save_camera_preset(int slot) {
    if (slot < 0 || slot >= 10) return;
    
    camera_presets[slot].position = camera_position;
    camera_presets[slot].target = camera_target;
    camera_presets[slot].angle_h = camera_angle_h;
    camera_presets[slot].angle_v = camera_angle_v;
    camera_presets[slot].distance = camera_distance;
    camera_presets[slot].fov = field_of_view;
    camera_presets[slot].valid = true;
    
    std::cout << "Saved camera preset to slot " << slot << std::endl;
}

void GraphRenderer::load_camera_preset(int slot) {
    if (slot < 0 || slot >= 10) return;
    if (!camera_presets[slot].valid) {
        std::cout << "No preset saved in slot " << slot << std::endl;
        return;
    }
    
    camera_position = camera_presets[slot].position;
    camera_target = camera_presets[slot].target;
    camera_angle_h = camera_presets[slot].angle_h;
    camera_angle_v = camera_presets[slot].angle_v;
    camera_distance = camera_presets[slot].distance;
    field_of_view = camera_presets[slot].fov;
    
    std::cout << "Loaded camera preset from slot " << slot << std::endl;
}

void GraphRenderer::adjust_lighting(float ambient_delta, float directional_delta) {
    lighting.ambient_intensity = std::max(0.0f, std::min(1.0f, lighting.ambient_intensity + ambient_delta));
    lighting.directional_intensity = std::max(0.0f, std::min(1.0f, lighting.directional_intensity + directional_delta));
    
    if (ambient_delta != 0) {
        std::cout << "Ambient light: " << (int)(lighting.ambient_intensity * 100) << "%" << std::endl;
    }
    if (directional_delta != 0) {
        std::cout << "Directional light: " << (int)(lighting.directional_intensity * 100) << "%" << std::endl;
    }
}

void GraphRenderer::rotate_light(float horizontal, float vertical) {
    // Convert world-space light direction to spherical coordinates
    float length = lighting.directional_light_dir.length();
    if (length < 1e-4f) length = 1.0f;
    float theta = std::atan2(lighting.directional_light_dir.z, lighting.directional_light_dir.x);
    float phi = std::acos(std::max(-1.0f, std::min(1.0f, lighting.directional_light_dir.y / length)));
    
    // Apply rotation
    theta += horizontal;
    phi += vertical;
    phi = std::max(0.1f, std::min(3.14f, phi));
    
    // Convert back to Cartesian (world space)
    lighting.directional_light_dir.x = length * std::sin(phi) * std::cos(theta);
    lighting.directional_light_dir.y = length * std::cos(phi);
    lighting.directional_light_dir.z = length * std::sin(phi) * std::sin(theta);
}

void GraphRenderer::reset_camera() {
    camera_distance = 15.0f;
    camera_angle_h = 0.0f;
    camera_angle_v = 0.3f;
    camera_target = Vector3(0, 0, 0);
    field_of_view = 60.0f;
    zoom_level = 1.0f;
    view_center = sf::Vector2f(0, 0);
    view.setCenter(view_center);
    view.setSize(sf::Vector2f(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT));
    window.setView(view);
    update_camera_position();
}

Pixels GraphRenderer::create_help_overlay() {
    Pixels help(600, 700);
    help.fill(argb(200, 30, 30, 40));
    
    // Title
    help.fill_rect(0, 0, 600, 40, argb(255, 50, 50, 70));
    
    // Create text sections (simulated with colored blocks for now)
    int y = 50;
    int line_height = 25;
    
    // Camera controls section
    help.fill_rect(10, y, 580, 2, argb(255, 100, 150, 200));
    y += line_height;
    
    // Movement controls
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // W
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // A
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // S
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // D
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // Q
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // E
    y += line_height;
    
    // Rotation controls
    y += 10;
    help.fill_rect(10, y, 580, 2, argb(255, 100, 150, 200));
    y += line_height;
    
    // Arrow keys
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200));
    y += line_height;
    
    // Lighting controls section
    y += 10;
    help.fill_rect(10, y, 580, 2, argb(255, 200, 150, 100));
    y += line_height;
    
    // Light intensity
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // I
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // K
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // L
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // J
    y += line_height;
    
    // Visual effects section
    y += 10;
    help.fill_rect(10, y, 580, 2, argb(255, 150, 200, 150));
    y += line_height;
    
    // Toggle controls
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // F
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // G
    y += line_height;
    help.fill_rect(20, y, 10, 10, argb(255, 200, 200, 200)); // X
    y += line_height;
    
    // Camera presets section
    y += 10;
    help.fill_rect(10, y, 580, 2, argb(255, 200, 200, 100));
    y += line_height;
    
    // Preset indicators
    for (int i = 0; i < 10; i++) {
        int x = 20 + i * 55;
        if (camera_presets[i].valid) {
            help.fill_rect(x, y, 45, 20, argb(255, 100, 200, 100));
        } else {
            help.fill_rect(x, y, 45, 20, argb(100, 100, 100, 100));
        }
    }
    
    return help;
}

void GraphRenderer::load_ui_font() {
    // Try project font first
    if (ui_font.openFromFile("assets/fonts/Inter-Regular.ttf")) {
        ui_font_loaded = true;
        return;
    }
    // Fallbacks (common macOS/Linux paths)
    const char* fallbacks[] = {
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };
    for (const char* path : fallbacks) {
        if (ui_font.openFromFile(path)) {
            ui_font_loaded = true;
            return;
        }
    }
    ui_font_loaded = false;
}

void GraphRenderer::draw_help_overlay_sfml() {
    // Panel background
    sf::RectangleShape panel(sf::Vector2f(620.f, 760.f));
    panel.setPosition(sf::Vector2f(10.f, 10.f));
    panel.setFillColor(sf::Color(30, 30, 40, 220));
    panel.setOutlineThickness(1.0f);
    panel.setOutlineColor(sf::Color(80, 120, 180, 200));
    window.draw(panel);

    auto addText = [&](const std::string& s, float x, float y, unsigned size, sf::Color color) {
        sf::Text t(ui_font);
        t.setString(s);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setPosition(sf::Vector2f(x, y));
        window.draw(t);
    };

    float y = 25;
    addText("Controls", 30.f, y, 28, sf::Color(240,240,255)); y += 40.f;

    addText("Camera", 30.f, y, 22, sf::Color(200,220,255)); y += 28.f;
    addText("W/S: Forward/Back, A/D: Left/Right, Q/E: Down/Up", 40.f, y, 18, sf::Color(230,230,240)); y += 22.f;
    addText("Arrows or Right-Drag: Rotate, Left-Drag: Pan", 40.f, y, 18, sf::Color(230,230,240)); y += 22.f;
    addText("Mouse Wheel: Zoom, Shift+Wheel: FOV, Ctrl+Wheel: Speed", 40.f, y, 18, sf::Color(230,230,240)); y += 28.f;

    addText("Presets", 30.f, y, 22, sf::Color(200,220,255)); y += 28.f;
    addText("0-9: Load preset, Ctrl+0-9: Save preset, R: Reset", 40.f, y, 18, sf::Color(230,230,240)); y += 28.f;

    addText("Lighting", 30.f, y, 22, sf::Color(200,220,255)); y += 28.f;
    addText("I/K: Ambient +/-, L/J: Directional +/-, Numpad 4/6/8/2: Rotate light", 40.f, y, 18, sf::Color(230,230,240)); y += 28.f;

    addText("Visuals", 30.f, y, 22, sf::Color(200,220,255)); y += 28.f;
    addText("F: Fog, G: Grid, X: Axes, O: Info overlay, H: Toggle this help", 40.f, y, 18, sf::Color(230,230,240)); y += 28.f;
    addText("Space: Auto-rotate, P: Physics", 40.f, y, 18, sf::Color(230,230,240));
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
    scene_center = center; // use world-relative center for lighting and overlays
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
    draw_axes();

    // Precompute camera forward direction for depth calculations
    Vector3 forward_dir = (camera_target - camera_position).normalize();
    
    // Draw edges with 3D perspective and lighting
    std::vector<sf::Vertex> edge_vertices;
    
    for (uint32_t i = 0; i < graph.edge_count; i++) {
        const GraphEdge& edge = graph.edges[i];
        if (!edge.visible) continue;
        
        const GraphNode& from_node = graph.nodes[edge.from_id];
        const GraphNode& to_node = graph.nodes[edge.to_id];
        
        sf::Vector2f from_pos = world_to_screen_3d(from_node.position);
        sf::Vector2f to_pos = world_to_screen_3d(to_node.position);
        
        // Calculate depth for fog
        Vector3 edge_center = (from_node.position + to_node.position) * 0.5f;
        Vector3 relative_pos = edge_center - camera_position;
        float depth = relative_pos.x * forward_dir.x + relative_pos.y * forward_dir.y + relative_pos.z * forward_dir.z;
        
        // Base edge color with transparency based on depth
        sf::Color edge_color(130, 130, 180, 170);
        
        // Apply depth and subtle contour shading to edges based on midpoint height
        float shade = calculate_depth_shade(depth);
        edge_color.r *= shade;
        edge_color.g *= shade;
        edge_color.b *= shade;
        float mid_height = edge_center.y - scene_center.y;
        float band_e = 0.5f + 0.5f * std::sin(mid_height * lighting.contour_frequency + lighting.contour_offset);
        float contour_mix_e = 1.0f - (lighting.contour_intensity * 0.5f) + (lighting.contour_intensity * 0.5f) * band_e;
        edge_color.r = static_cast<unsigned char>(std::min(255.0f, edge_color.r * contour_mix_e));
        edge_color.g = static_cast<unsigned char>(std::min(255.0f, edge_color.g * contour_mix_e));
        edge_color.b = static_cast<unsigned char>(std::min(255.0f, edge_color.b * contour_mix_e));
        
        // Apply fog
        edge_color = apply_fog(edge_color, depth);
        
        // Create edge vertices
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
    
    // Build draw order for nodes: sort by depth (far to near) to emulate z-buffer
    std::vector<uint32_t> node_indices;
    node_indices.reserve(graph.node_count);
    for (uint32_t i = 0; i < graph.node_count; i++) {
        if (graph.nodes[i].visible) node_indices.push_back(i);
    }
    std::sort(node_indices.begin(), node_indices.end(), [&](uint32_t a, uint32_t b) {
        const Vector3& pa = graph.nodes[a].position;
        const Vector3& pb = graph.nodes[b].position;
        Vector3 ra = pa - camera_position;
        Vector3 rb = pb - camera_position;
        float da = ra.x * forward_dir.x + ra.y * forward_dir.y + ra.z * forward_dir.z;
        float db = rb.x * forward_dir.x + rb.y * forward_dir.y + rb.z * forward_dir.z;
        return da > db; // draw far first
    });

    // Draw nodes with 3D perspective, lighting, and depth-based sizing
    for (uint32_t idx : node_indices) {
        const GraphNode& node = graph.nodes[idx];
        sf::Vector2f screen_pos = world_to_screen_3d(node.position);
        
        // Calculate depth for perspective scaling
        Vector3 relative_pos = node.position - camera_position;
        float depth = relative_pos.x * forward_dir.x + relative_pos.y * forward_dir.y + relative_pos.z * forward_dir.z;
        
        float perspective_scale = apply_perspective(depth);
        float visual_radius = node.radius * perspective_scale * 0.5f;
        visual_radius = std::max(2.0f, std::min(visual_radius, 50.0f));
        
        // Calculate node normal relative to scene center (world-relative lighting)
        Vector3 normal = (node.position - scene_center).normalize();
        
        // Apply lighting to node color
        sf::Color lit_color = apply_lighting(node.position, normal, 
            sf::Color(node.color.r, node.color.g, node.color.b, node.color.a));
        
        // Add contour banding based on world-space height relative to scene center
        float height = node.position.y - scene_center.y;
        float band = 0.5f + 0.5f * std::sin(height * lighting.contour_frequency + lighting.contour_offset);
        float contour_mix = 1.0f - lighting.contour_intensity + lighting.contour_intensity * band;
        lit_color.r = static_cast<unsigned char>(std::min(255.0f, lit_color.r * contour_mix));
        lit_color.g = static_cast<unsigned char>(std::min(255.0f, lit_color.g * contour_mix));
        lit_color.b = static_cast<unsigned char>(std::min(255.0f, lit_color.b * contour_mix));
        
        // Apply fog based on depth
        lit_color = apply_fog(lit_color, depth);
        
        // Create gradient effect for 3D appearance
        sf::CircleShape circle(visual_radius);
        circle.setFillColor(lit_color);
        
        // Depth-based outline
        float outline_alpha = std::max(50.0f, 255.0f * (1.0f - depth / 50.0f));
        circle.setOutlineThickness(std::max(0.5f, 2.0f - depth / 25.0f));
        circle.setOutlineColor(sf::Color(255, 255, 255, outline_alpha));
        
        circle.setPosition(sf::Vector2f(screen_pos.x - circle.getRadius(), screen_pos.y - circle.getRadius()));
        window.draw(circle);
    }
    
    // Draw help overlay if enabled
    if (show_help) {
        sf::View original_view = window.getView();
        window.setView(window.getDefaultView());
        draw_help_overlay_sfml();
        window.setView(original_view);
    }
    
    // Draw custom overlay if provided
    else if (!overlay.is_empty()) {
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
    
    // Draw interactive UI (sliders) on top of everything
    draw_ui_sliders();
    
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