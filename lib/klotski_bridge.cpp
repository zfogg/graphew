#include "klotski_bridge.hpp"
#include "fileutils.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <cjson/cJSON.h>
#include <algorithm>
#include <random>

// KlotskiGraph implementation
void KlotskiGraph::add_node(const std::string& board_rep, const Vector3& pos, bool is_sol) {
    KlotskiNode node;
    node.board_representation = board_rep;
    node.position = pos;
    node.hash = string_to_hash(board_rep);
    node.is_solution = is_sol;
    node.color = is_sol ? Color(255, 215, 0, 255) : hash_to_color(node.hash); // Gold for solutions
    node.radius = is_sol ? 1.0f : 0.5f;
    node.age = nodes.size();
    node.label = is_sol ? "SOLUTION" : ("State" + std::to_string(nodes.size()));
    
    hash_to_index[node.hash] = nodes.size();
    nodes.push_back(node);
}

void KlotskiGraph::add_edge(double from_hash, double to_hash, const KlotskiMove& move) {
    KlotskiEdge edge;
    edge.from_hash = from_hash;
    edge.to_hash = to_hash;
    edge.move = move;
    edge.color = WHITE;
    edge.thickness = 1.5f;
    
    edges.push_back(edge);
}

void KlotskiGraph::convert_to_graph3d(Graph3D& graph3d) const {
    // Add all nodes
    for (const auto& node : nodes) {
        graph3d.add_node(node.position, node.color, node.radius, node.label);
    }
    
    // Add all edges
    for (const auto& edge : edges) {
        auto from_iter = hash_to_index.find(edge.from_hash);
        auto to_iter = hash_to_index.find(edge.to_hash);
        
        if (from_iter != hash_to_index.end() && to_iter != hash_to_index.end()) {
            graph3d.add_edge(from_iter->second, to_iter->second, edge.color, edge.thickness);
        }
    }
}

double KlotskiGraph::string_to_hash(const std::string& board_rep) const {
    std::hash<std::string> hasher;
    return static_cast<double>(hasher(board_rep));
}

Color KlotskiGraph::hash_to_color(double hash) const {
    uint32_t color_hash = static_cast<uint32_t>(std::abs(hash));
    
    uint8_t r = (color_hash * 123) % 180 + 75;
    uint8_t g = (color_hash * 456) % 180 + 75;
    uint8_t b = (color_hash * 789) % 180 + 75;
    
    return Color(r, g, b, 255);
}

Vector3 KlotskiGraph::generate_position(int index) const {
    // Generate positions in a spiral pattern
    float angle = index * 0.5f;
    float radius = 2.0f + index * 0.3f;
    
    return Vector3(
        radius * std::cos(angle),
        radius * std::sin(angle),
        (index % 3 - 1) * 2.0f
    );
}

bool KlotskiGraph::load_from_json(const std::string& filename) {
    size_t file_size;
    char* data = read_file_raw(filename.c_str(), &file_size);
    if (!data) return false;
    
    cJSON* json = cJSON_Parse(data);
    std::free(data);
    
    if (!json) return false;
    
    // Clear existing data
    nodes.clear();
    edges.clear();
    hash_to_index.clear();
    
    // Parse nodes
    cJSON* nodes_json = cJSON_GetObjectItem(json, "nodes");
    if (cJSON_IsArray(nodes_json)) {
        cJSON* node_json;
        cJSON_ArrayForEach(node_json, nodes_json) {
            cJSON* pos = cJSON_GetObjectItem(node_json, "position");
            cJSON* board_state = cJSON_GetObjectItem(node_json, "board_state");
            cJSON* is_solution = cJSON_GetObjectItem(node_json, "is_solution");
            
            Vector3 position(0, 0, 0);
            if (cJSON_IsArray(pos) && cJSON_GetArraySize(pos) >= 3) {
                position.x = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 0)));
                position.y = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 1)));
                position.z = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 2)));
            }
            
            std::string board_rep = cJSON_IsString(board_state) ? cJSON_GetStringValue(board_state) : "";
            bool is_sol = cJSON_IsTrue(is_solution);
            
            if (!board_rep.empty()) {
                add_node(board_rep, position, is_sol);
            }
        }
    }
    
    // Parse edges
    cJSON* edges_json = cJSON_GetObjectItem(json, "edges");
    if (cJSON_IsArray(edges_json)) {
        cJSON* edge_json;
        cJSON_ArrayForEach(edge_json, edges_json) {
            cJSON* from = cJSON_GetObjectItem(edge_json, "from");
            cJSON* to = cJSON_GetObjectItem(edge_json, "to");
            cJSON* move = cJSON_GetObjectItem(edge_json, "move");
            
            if (cJSON_IsString(from) && cJSON_IsString(to)) {
                double from_hash = std::stod(cJSON_GetStringValue(from));
                double to_hash = std::stod(cJSON_GetStringValue(to));
                
                KlotskiMove klotski_move;
                if (cJSON_IsObject(move)) {
                    cJSON* piece = cJSON_GetObjectItem(move, "piece");
                    cJSON* dx = cJSON_GetObjectItem(move, "dx");
                    cJSON* dy = cJSON_GetObjectItem(move, "dy");
                    
                    if (cJSON_IsString(piece)) {
                        std::string piece_str = cJSON_GetStringValue(piece);
                        klotski_move.piece = piece_str.empty() ? '.' : piece_str[0];
                    }
                    klotski_move.dx = cJSON_IsNumber(dx) ? static_cast<int>(cJSON_GetNumberValue(dx)) : 0;
                    klotski_move.dy = cJSON_IsNumber(dy) ? static_cast<int>(cJSON_GetNumberValue(dy)) : 0;
                }
                
                add_edge(from_hash, to_hash, klotski_move);
            }
        }
    }
    
    cJSON_Delete(json);
    return true;
}

void KlotskiGraph::export_to_json(const std::string& filename) const {
    cJSON* root = cJSON_CreateObject();
    cJSON* nodes_array = cJSON_CreateArray();
    cJSON* edges_array = cJSON_CreateArray();
    
    // Export nodes
    for (const auto& node : nodes) {
        cJSON* node_obj = cJSON_CreateObject();
        
        cJSON_AddStringToObject(node_obj, "id", std::to_string(static_cast<long long>(node.hash)).c_str());
        
        cJSON* pos_array = cJSON_CreateArray();
        cJSON_AddItemToArray(pos_array, cJSON_CreateNumber(node.position.x));
        cJSON_AddItemToArray(pos_array, cJSON_CreateNumber(node.position.y));
        cJSON_AddItemToArray(pos_array, cJSON_CreateNumber(node.position.z));
        cJSON_AddItemToObject(node_obj, "position", pos_array);
        
        cJSON* color_array = cJSON_CreateArray();
        cJSON_AddItemToArray(color_array, cJSON_CreateNumber(node.color.r));
        cJSON_AddItemToArray(color_array, cJSON_CreateNumber(node.color.g));
        cJSON_AddItemToArray(color_array, cJSON_CreateNumber(node.color.b));
        cJSON_AddItemToArray(color_array, cJSON_CreateNumber(node.color.a));
        cJSON_AddItemToObject(node_obj, "color", color_array);
        
        cJSON_AddNumberToObject(node_obj, "radius", node.radius);
        cJSON_AddStringToObject(node_obj, "label", node.label.c_str());
        cJSON_AddStringToObject(node_obj, "board_state", node.board_representation.c_str());
        cJSON_AddNumberToObject(node_obj, "hash", node.hash);
        cJSON_AddNumberToObject(node_obj, "age", node.age);
        cJSON_AddBoolToObject(node_obj, "is_solution", node.is_solution);
        
        cJSON_AddItemToArray(nodes_array, node_obj);
    }
    
    // Export edges
    for (const auto& edge : edges) {
        cJSON* edge_obj = cJSON_CreateObject();
        
        cJSON_AddStringToObject(edge_obj, "from", std::to_string(static_cast<long long>(edge.from_hash)).c_str());
        cJSON_AddStringToObject(edge_obj, "to", std::to_string(static_cast<long long>(edge.to_hash)).c_str());
        
        cJSON* move_obj = cJSON_CreateObject();
        char piece_str[2] = {edge.move.piece, '\0'};
        cJSON_AddStringToObject(move_obj, "piece", piece_str);
        cJSON_AddNumberToObject(move_obj, "dx", edge.move.dx);
        cJSON_AddNumberToObject(move_obj, "dy", edge.move.dy);
        cJSON_AddItemToObject(edge_obj, "move", move_obj);
        
        cJSON* edge_color = cJSON_CreateArray();
        cJSON_AddItemToArray(edge_color, cJSON_CreateNumber(edge.color.r));
        cJSON_AddItemToArray(edge_color, cJSON_CreateNumber(edge.color.g));
        cJSON_AddItemToArray(edge_color, cJSON_CreateNumber(edge.color.b));
        cJSON_AddItemToArray(edge_color, cJSON_CreateNumber(edge.color.a));
        cJSON_AddItemToObject(edge_obj, "color", edge_color);
        
        cJSON_AddNumberToObject(edge_obj, "thickness", edge.thickness);
        
        cJSON_AddItemToArray(edges_array, edge_obj);
    }
    
    cJSON_AddStringToObject(root, "type", "klotski_state_graph");
    cJSON_AddItemToObject(root, "nodes", nodes_array);
    cJSON_AddItemToObject(root, "edges", edges_array);
    
    // Add metadata
    cJSON* metadata = cJSON_CreateObject();
    cJSON_AddNumberToObject(metadata, "total_states", nodes.size());
    cJSON_AddStringToObject(metadata, "generated_by", "graphew_klotski_bridge");
    cJSON_AddItemToObject(root, "metadata", metadata);
    
    // Write to file
    char* json_string = cJSON_Print(root);
    std::ofstream file(filename);
    file << json_string;
    file.close();
    
    std::free(json_string);
    cJSON_Delete(root);
    
    std::cout << "Exported Klotski graph to " << filename << std::endl;
}

// KlotskiBridge implementation
KlotskiGraph KlotskiBridge::generate_sample_klotski_graph() {
    KlotskiGraph graph;
    
    // Create a more complex branching state space (simulating actual puzzle complexity)
    std::vector<std::string> sample_states = {
        "abbcabbceddhefghi..j",  // Root state
        "abbcabbce.dhefghijj.",  // Branch 1: j moves
        "abbcabbceddh.fghi.ej",  // Branch 2: e moves  
        ".bbca.bceddhefghi.aj",  // Branch 3: a moves
        "abbcabbce.dhef.ghijj",  // Branch 1.1: continue j path
        "abbcabbcef.he.ghijj",   // Branch 1.2: h moves
        "abbc.bbceddhefghiaej",  // Branch 2.1: continue e path
        ".bbcabbceddhefghi.aj",  // Branch 3.1: continue a path
        "abbcabbcefghe.ghij.",   // Convergence point
        "abbcabbcefghe.ghi.j",   // Solution branch
        ".bbcabbceddh.fghiaej",  // Alternative path 1
        "ab.cabbceddhefghij.e",  // Alternative path 2
        "abbcabbcefghefghij..",  // Near-solution 1
        "abbcabbcefghe.ghij.",   // Near-solution 2 (duplicate for branching)
        "abbcabbcefghe.ghi.j"    // Final solution
    };
    
    // Create positions in a more interesting 3D structure
    std::vector<Vector3> positions = {
        Vector3(0, 0, 0),        // Root at center
        Vector3(3, 1, 1),        // Branch right-up-forward
        Vector3(-2, 2, -1),      // Branch left-up-back
        Vector3(1, -2, 2),       // Branch right-down-forward
        Vector3(5, 0, 2),        // Extend branch 1
        Vector3(4, 3, 0),        // Extend branch 1 alt
        Vector3(-4, 3, -2),      // Extend branch 2
        Vector3(2, -4, 3),       // Extend branch 3
        Vector3(2, 1, 4),        // Convergence point
        Vector3(0, 2, 6),        // Solution (elevated)
        Vector3(-3, 4, -1),      // Alternative path
        Vector3(3, -3, 1),       // Another alternative  
        Vector3(1, 3, 5),        // Near solution 1
        Vector3(-1, 1, 5),       // Near solution 2
        Vector3(0, 2, 7)         // Final solution (highest)
    };
    
    // Add nodes with custom positions
    for (size_t i = 0; i < sample_states.size(); i++) {
        Vector3 pos = positions[i];
        bool is_solution = (i >= sample_states.size() - 2); // Last 2 states are solutions
        graph.add_node(sample_states[i], pos, is_solution);
    }
    
    // Create a complex edge structure (not just linear)
    std::vector<std::pair<size_t, size_t>> edges = {
        {0, 1}, {0, 2}, {0, 3},     // Root branches to 3 children
        {1, 4}, {1, 5},             // Branch 1 splits
        {2, 6},                     // Branch 2 continues
        {3, 7},                     // Branch 3 continues
        {4, 8}, {5, 8},             // Convergence to point 8
        {6, 10}, {7, 11},           // Branches to alternatives
        {8, 9}, {8, 12}, {8, 13},   // Multiple paths to solutions
        {10, 13}, {11, 12},         // Cross connections
        {12, 14}, {13, 14}          // Final solution paths
    };
    
    for (const auto& edge_pair : edges) {
        double from_hash = graph.string_to_hash(sample_states[edge_pair.first]);
        double to_hash = graph.string_to_hash(sample_states[edge_pair.second]);
        
        KlotskiMove move{'x', 1, 0}; // Placeholder move
        graph.add_edge(from_hash, to_hash, move);
    }
    
    return graph;
}

bool KlotskiBridge::load_klotski_json(Graph3D& graph3d, const std::string& filename) {
    KlotskiGraph klotski_graph;
    if (!klotski_graph.load_from_json(filename)) {
        return false;
    }
    
    klotski_graph.convert_to_graph3d(graph3d);
    return true;
}

