#pragma once

#include "graph.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations to avoid heavy includes
class KlotskiBoard;

// Simplified structures based on swaptube but standalone
struct KlotskiMove {
    char piece = '.';
    int dx = 0;
    int dy = 0;
};

struct KlotskiNode {
    std::string board_representation;
    Vector3 position;
    Color color;
    float radius;
    double hash;
    bool is_solution;
    int age;
    std::string label;
};

struct KlotskiEdge {
    double from_hash;
    double to_hash;
    KlotskiMove move;
    Color color;
    float thickness;
};

class KlotskiGraph {
public:
    std::vector<KlotskiNode> nodes;
    std::vector<KlotskiEdge> edges;
    std::unordered_map<double, int> hash_to_index;
    
    void add_node(const std::string& board_rep, const Vector3& pos, bool is_sol = false);
    void add_edge(double from_hash, double to_hash, const KlotskiMove& move);
    void convert_to_graph3d(Graph3D& graph3d) const;
    bool load_from_json(const std::string& filename);
    void export_to_json(const std::string& filename) const;
    
    double string_to_hash(const std::string& board_rep) const;
    Color hash_to_color(double hash) const;
    Vector3 generate_position(int index) const;
};

class KlotskiBridge {
public:
    static KlotskiGraph generate_sample_klotski_graph();
    static bool load_klotski_json(Graph3D& graph3d, const std::string& filename);
};