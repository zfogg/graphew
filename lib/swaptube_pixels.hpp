#pragma once

#include <vector>
#include <cstdlib>
#include <cmath>
#include <stack>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

// Utility functions
inline double clamp(double bottom, double val, double top) { return std::min(top, std::max(val, bottom)); }
inline double square(double x) { return x * x; }
inline double lerp(double a, double b, double w) { return a * (1 - w) + b * w; }

// Color functions
inline int argb(int a, int r, int g, int b) {
    return (a << 24) + (r << 16) + (g << 8) + (b);
}

inline int geta(int col) { return (col & 0xff000000) >> 24; }
inline int getr(int col) { return (col & 0x00ff0000) >> 16; }
inline int getg(int col) { return (col & 0x0000ff00) >> 8; }
inline int getb(int col) { return (col & 0x000000ff); }

inline int colorlerp(int col1, int col2, float w) {
    return argb(round(lerp(geta(col1), geta(col2), w)),
                round(lerp(getr(col1), getr(col2), w)),
                round(lerp(getg(col1), getg(col2), w)),
                round(lerp(getb(col1), getb(col2), w)));
}

inline string color_to_string(int c) {
    return "(" + to_string(geta(c)) + ", " + to_string(getr(c)) + ", " + 
           to_string(getg(c)) + ", " + to_string(getb(c)) + ")";
}

inline int color_combine(int base_color, int over_color, float overlay_opacity_multiplier = 1) {
    float base_opacity = geta(base_color) / 255.0f;
    float over_opacity = geta(over_color) / 255.0f * overlay_opacity_multiplier;
    float final_opacity = 1.0f - (1.0f - base_opacity) * (1.0f - over_opacity);
    if (final_opacity == 0) return 0x00000000;
    int final_alpha = round(final_opacity * 255.0f);
    float chroma_weight = over_opacity / final_opacity;
    int final_rgb = colorlerp(base_color, over_color, chroma_weight) & 0x00ffffff;
    return (final_alpha << 24) | (final_rgb);
}

// Constants
inline int OPAQUE_BLACK = 0xFF000000;
inline int OPAQUE_WHITE = 0xFFFFFFFF;
inline int TRANSPARENT_BLACK = 0x00000000;
inline int TRANSPARENT_WHITE = 0x00FFFFFF;

// Pixels class from swaptube (essential parts only)
class Pixels {
public:
    int w;
    int h;
    vector<unsigned int> pixels;
    
    Pixels() : w(0), h(0), pixels(0) {}
    Pixels(int width, int height) : w(width), h(height), pixels(width * height) {}
    Pixels(const Pixels& other) : w(other.w), h(other.h), pixels(other.pixels) {}

    inline bool out_of_range(int x, int y) const {
        return x < 0 || x >= w || y < 0 || y >= h;
    }

    inline int get_pixel(int x, int y) const {
        if (out_of_range(x, y)) return 0;
        return pixels[w * y + x];
    }

    inline void set_pixel(int x, int y, int col) {
        if (out_of_range(x, y)) return;
        pixels[w * y + x] = col;
    }

    inline void overlay_pixel(int x, int y, int col, double overlay_opacity_multiplier = 1) {
        set_pixel(x, y, color_combine(get_pixel(x, y), col, overlay_opacity_multiplier));
    }

    inline int get_alpha(int x, int y) const {
        return geta(get_pixel(x, y));
    }

    void fill_rect(int x, int y, int rw, int rh, int col) {
        for (int dx = 0; dx < rw; dx++)
            for (int dy = 0; dy < rh; dy++)
                set_pixel(x + dx, y + dy, col);
    }

    void fill_circle(double x, double y, double r, int col, double opa = 1) {
        double r_sq = square(r);
        for (double dx = -r + 1; dx < r; dx++) {
            double sdx = square(dx);
            for (double dy = -r + 1; dy < r; dy++) {
                if (sdx + square(dy) < r_sq)
                    overlay_pixel(x + dx, y + dy, col, opa);
            }
        }
    }

    void fill(int col) {
        fill_rect(0, 0, w, h, col);
    }

    void bresenham_line(int x1, int y1, int x2, int y2, int col, float opacity = 1.0f, int thickness = 1) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        
        if (dx > 10000 || dy > 10000) return;

        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            overlay_pixel(x1, y1, col, opacity);
            for (int i = 1; i < thickness; i++) {
                overlay_pixel(x1 + i, y1, col, opacity);
                overlay_pixel(x1 - i, y1, col, opacity);
                overlay_pixel(x1, y1 + i, col, opacity);
                overlay_pixel(x1, y1 - i, col, opacity);
            }

            if (x1 == x2 && y1 == y2) break;

            int e2 = err * 2;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    void overlay(const Pixels& p, int dx, int dy, double overlay_opacity_multiplier = 1) {
        for (int x = 0; x < p.w; x++) {
            int xpdx = x + dx;
            for (int y = 0; y < p.h; y++) {
                overlay_pixel(xpdx, y + dy, p.get_pixel(x, y), overlay_opacity_multiplier);
            }
        }
    }

    bool is_empty() const {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                if (get_alpha(x, y) != 0) {
                    return false;
                }
            }
        }
        return true;
    }
};