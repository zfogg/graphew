#ifdef __APPLE__

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include "macos_gestures.hpp"
#include <iostream>

// Custom NSView subclass to handle trackpad gestures
@interface GestureView : NSView
@property (nonatomic, assign) MagnificationCallback magnificationCallback;
@end

@implementation GestureView
- (void)magnifyWithEvent:(NSEvent *)event {
    if (self.magnificationCallback) {
        float magnification = [event magnification];
        NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];

        // Call the C++ callback with view-relative coordinates
        self.magnificationCallback(magnification, location.x, location.y);
    }
    [super magnifyWithEvent:event];
}

- (BOOL)acceptsFirstResponder {
    return YES; // Accept gesture events
}
@end

// Static members
MagnificationCallback MacOSGestureMonitor::magnification_callback = nullptr;
void* MacOSGestureMonitor::event_monitor = nullptr; // Now stores the gesture view

void MacOSGestureMonitor::initialize() {
    if (event_monitor != nullptr) {
        return; // Already initialized
    }
}

void MacOSGestureMonitor::cleanup() {
    if (event_monitor != nullptr) {
        // Release the gesture view if we created one
        event_monitor = nullptr;
    }
}

void MacOSGestureMonitor::set_magnification_callback(MagnificationCallback callback) {
    magnification_callback = callback;
}

// Helper function to integrate with SFML window
void MacOSGestureMonitor::integrate_with_sfml_window(void* sfml_window_handle) {
    if (magnification_callback == nullptr) {
        return;
    }

    // Cast SFML window handle to NSWindow
    NSWindow* window = (__bridge NSWindow*)sfml_window_handle;
    if (window == nil) {
        std::cout << "Failed to get NSWindow from SFML" << std::endl;
        return;
    }

    // Create our gesture view and set it as the content view
    GestureView* gestureView = [[GestureView alloc] init];
    gestureView.magnificationCallback = magnification_callback;

    // Add gesture view as overlay or replace content view
    NSView* contentView = [window contentView];
    [gestureView setFrame:[contentView bounds]];
    [gestureView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [contentView addSubview:gestureView];

    // Store reference for cleanup
    event_monitor = gestureView;
}

#endif // __APPLE__