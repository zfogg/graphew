#pragma once

#ifdef __APPLE__

#include <functional>

// Callback function type for magnification gestures
// Parameters: magnification_factor, cursor_x, cursor_y
using MagnificationCallback = std::function<void(float, float, float)>;

class MacOSGestureMonitor {
public:
    static void initialize();
    static void cleanup();
    static void set_magnification_callback(MagnificationCallback callback);
    static void integrate_with_sfml_window(void* sfml_window_handle);
    
private:
    static MagnificationCallback magnification_callback;
    static void* event_monitor; // Gesture view (void* to avoid Objective-C in header)
};

#endif // __APPLE__