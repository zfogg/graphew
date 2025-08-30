#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>

#define MAX_NODES 1000
#define MAX_EDGES 2000
#define MAX_LABEL_LENGTH 64

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    float length() const;
    Vector3 normalize() const;
    static float distance(const Vector3& a, const Vector3& b);
    static Vector3 scale(const Vector3& v, float scalar);
    static Vector3 add(const Vector3& a, const Vector3& b);
    static Vector3 subtract(const Vector3& a, const Vector3& b);
};

struct Color {
    uint8_t r, g, b, a;
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
};

// Common colors
extern const Color RED;
extern const Color GREEN;
extern const Color BLUE;
extern const Color YELLOW;
extern const Color MAGENTA;
extern const Color DARKBLUE;
extern const Color ORANGE;
extern const Color PURPLE;
extern const Color WHITE;
extern const Color GRAY;
extern const Color BLACK;

struct GraphNode {
    Vector3 position;
    Vector3 velocity;
    Vector3 force;
    Color color;
    Color original_color;  // Store original color for filtering
    float radius;
    float original_radius; // Store original radius for filtering
    char label[MAX_LABEL_LENGTH];
    bool visible;
    
    // Metadata for filtering
    std::string type;        // e.g., "agent", "item", "location", "state"
    std::string subtype;     // e.g., "red_ore", "blue_ore", "inventory"
    std::map<std::string, std::string> properties; // Generic key-value pairs
    std::vector<std::string> tags;  // e.g., ["has_red_ore", "solution", "visited"]
    int agent_id;            // For agent-specific nodes
    int timestep;            // For time-based filtering
    float value;             // Generic numeric value (reward, quantity, etc.)
};

struct GraphEdge {
    uint32_t from_id;
    uint32_t to_id;
    Color color;
    float thickness;
    bool visible;
};

class Graph3D {
public:
    GraphNode nodes[MAX_NODES];
    GraphEdge edges[MAX_EDGES];
    uint32_t node_count;
    uint32_t edge_count;
    Vector3 center;
    float scale;

    Graph3D();
    ~Graph3D();
    
    uint32_t add_node(const Vector3& position, const Color& color, float radius, const std::string& label);
    void add_edge(uint32_t from_id, uint32_t to_id, const Color& color, float thickness);
    void update_physics(float delta_time);
    bool load_from_json(const std::string& filename);
    bool load_from_compressed_json(const std::string& filename);
    void generate_sample();
    void center_graph(); // Center all nodes around origin
};