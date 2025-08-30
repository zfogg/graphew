#include "graph.hpp"
#include "fileutils.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cjson/cJSON.h>
#include <iostream>
#include <string>

// Vector3 implementation
float Vector3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3 Vector3::normalize() const {
    float len = length();
    if (len == 0.0f) return Vector3(0, 0, 0);
    return Vector3(x / len, y / len, z / len);
}

float Vector3::distance(const Vector3& a, const Vector3& b) {
    Vector3 diff = a - b;
    return diff.length();
}

Vector3 Vector3::scale(const Vector3& v, float scalar) {
    return v * scalar;
}

Vector3 Vector3::add(const Vector3& a, const Vector3& b) {
    return a + b;
}

Vector3 Vector3::subtract(const Vector3& a, const Vector3& b) {
    return a - b;
}

// Color constants
const Color RED(255, 0, 0, 255);
const Color GREEN(0, 255, 0, 255);
const Color BLUE(0, 0, 255, 255);
const Color YELLOW(255, 255, 0, 255);
const Color MAGENTA(255, 0, 255, 255);
const Color DARKBLUE(0, 0, 139, 255);
const Color ORANGE(255, 165, 0, 255);
const Color PURPLE(128, 0, 128, 255);
const Color WHITE(255, 255, 255, 255);
const Color GRAY(128, 128, 128, 255);
const Color BLACK(0, 0, 0, 255);

// Graph3D implementation
Graph3D::Graph3D() : node_count(0), edge_count(0), center(0, 0, 0), scale(1.0f) {
    std::memset(nodes, 0, sizeof(nodes));
    std::memset(edges, 0, sizeof(edges));
}

Graph3D::~Graph3D() {
}

uint32_t Graph3D::add_node(const Vector3& position, const Color& color, float radius, const std::string& label) {
    if (node_count >= MAX_NODES) return UINT32_MAX;
    
    uint32_t id = node_count;
    GraphNode* node = &nodes[id];
    
    node->position = position;
    node->velocity = Vector3(0.0f, 0.0f, 0.0f);
    node->force = Vector3(0.0f, 0.0f, 0.0f);
    node->color = color;
    node->radius = radius;
    node->visible = true;
    
    if (!label.empty()) {
        std::strncpy(node->label, label.c_str(), MAX_LABEL_LENGTH - 1);
        node->label[MAX_LABEL_LENGTH - 1] = '\0';
    } else {
        std::snprintf(node->label, MAX_LABEL_LENGTH, "Node%u", id);
    }
    
    node_count++;
    return id;
}

void Graph3D::add_edge(uint32_t from_id, uint32_t to_id, const Color& color, float thickness) {
    if (edge_count >= MAX_EDGES) return;
    if (from_id >= node_count || to_id >= node_count) return;
    
    GraphEdge* edge = &edges[edge_count];
    edge->from_id = from_id;
    edge->to_id = to_id;
    edge->color = color;
    edge->thickness = thickness;
    edge->visible = true;
    
    edge_count++;
}

void Graph3D::update_physics(float delta_time) {
    const float damping = 0.95f;
    const float repulsion_strength = 50.0f;
    const float attraction_strength = 0.1f;
    const float min_distance = 0.1f;
    
    // Reset forces
    for (uint32_t i = 0; i < node_count; i++) {
        nodes[i].force = Vector3(0.0f, 0.0f, 0.0f);
    }
    
    // Repulsion between all nodes
    for (uint32_t i = 0; i < node_count; i++) {
        for (uint32_t j = i + 1; j < node_count; j++) {
            Vector3 diff = nodes[i].position - nodes[j].position;
            float distance = diff.length();
            
            if (distance < min_distance) distance = min_distance;
            
            Vector3 normalized = diff.normalize();
            float force_magnitude = repulsion_strength / (distance * distance);
            Vector3 force = normalized * force_magnitude;
            
            nodes[i].force = nodes[i].force + force;
            nodes[j].force = nodes[j].force - force;
        }
    }
    
    // Attraction along edges
    for (uint32_t i = 0; i < edge_count; i++) {
        GraphEdge* edge = &edges[i];
        if (!edge->visible) continue;
        
        GraphNode* from = &nodes[edge->from_id];
        GraphNode* to = &nodes[edge->to_id];
        
        Vector3 diff = to->position - from->position;
        float distance = diff.length();
        
        if (distance > min_distance) {
            Vector3 normalized = diff.normalize();
            float force_magnitude = attraction_strength * distance;
            Vector3 force = normalized * force_magnitude;
            
            from->force = from->force + force;
            to->force = to->force - force;
        }
    }
    
    // Update positions
    for (uint32_t i = 0; i < node_count; i++) {
        GraphNode* node = &nodes[i];
        
        node->velocity = node->velocity + (node->force * delta_time);
        node->velocity = node->velocity * damping;
        node->position = node->position + (node->velocity * delta_time);
    }
}

bool Graph3D::load_from_json(const std::string& filename) {
    size_t file_size;
    char* data = read_file_raw(filename.c_str(), &file_size);
    if (!data) return false;
    
    cJSON* json = cJSON_Parse(data);
    std::free(data);
    
    if (!json) return false;
    
    cJSON* nodes_json = cJSON_GetObjectItem(json, "nodes");
    if (cJSON_IsArray(nodes_json)) {
        cJSON* node;
        cJSON_ArrayForEach(node, nodes_json) {
            cJSON* pos = cJSON_GetObjectItem(node, "position");
            cJSON* color = cJSON_GetObjectItem(node, "color");
            cJSON* radius = cJSON_GetObjectItem(node, "radius");
            cJSON* label = cJSON_GetObjectItem(node, "label");
            
            Vector3 position(0, 0, 0);
            if (cJSON_IsArray(pos) && cJSON_GetArraySize(pos) >= 3) {
                position.x = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 0)));
                position.y = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 1)));
                position.z = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 2)));
            }
            
            Color node_color = RED;
            if (cJSON_IsArray(color) && cJSON_GetArraySize(color) >= 3) {
                node_color.r = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 0)));
                node_color.g = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 1)));
                node_color.b = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 2)));
                node_color.a = cJSON_GetArraySize(color) >= 4 ? 
                    static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 3))) : 255;
            }
            
            float node_radius = cJSON_IsNumber(radius) ? static_cast<float>(cJSON_GetNumberValue(radius)) : 0.5f;
            std::string node_label = cJSON_IsString(label) ? cJSON_GetStringValue(label) : "";
            
            add_node(position, node_color, node_radius, node_label);
        }
    }
    
    cJSON* edges_json = cJSON_GetObjectItem(json, "edges");
    if (cJSON_IsArray(edges_json)) {
        cJSON* edge;
        cJSON_ArrayForEach(edge, edges_json) {
            cJSON* from = cJSON_GetObjectItem(edge, "from");
            cJSON* to = cJSON_GetObjectItem(edge, "to");
            cJSON* color = cJSON_GetObjectItem(edge, "color");
            cJSON* thickness = cJSON_GetObjectItem(edge, "thickness");
            
            uint32_t from_id = cJSON_IsNumber(from) ? static_cast<uint32_t>(cJSON_GetNumberValue(from)) : 0;
            uint32_t to_id = cJSON_IsNumber(to) ? static_cast<uint32_t>(cJSON_GetNumberValue(to)) : 0;
            
            Color edge_color = BLUE;
            if (cJSON_IsArray(color) && cJSON_GetArraySize(color) >= 3) {
                edge_color.r = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 0)));
                edge_color.g = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 1)));
                edge_color.b = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 2)));
                edge_color.a = cJSON_GetArraySize(color) >= 4 ? 
                    static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 3))) : 255;
            }
            
            float edge_thickness = cJSON_IsNumber(thickness) ? static_cast<float>(cJSON_GetNumberValue(thickness)) : 1.0f;
            
            add_edge(from_id, to_id, edge_color, edge_thickness);
        }
    }
    
    cJSON_Delete(json);
    return true;
}

bool Graph3D::load_from_compressed_json(const std::string& filename) {
    size_t decompressed_size;
    char* data = read_compressed_file(filename.c_str(), &decompressed_size);
    if (!data) return false;
    
    cJSON* json = cJSON_Parse(data);
    std::free(data);
    
    if (!json) return false;
    
    // Same parsing logic as load_from_json
    cJSON* nodes_json = cJSON_GetObjectItem(json, "nodes");
    if (cJSON_IsArray(nodes_json)) {
        cJSON* node;
        cJSON_ArrayForEach(node, nodes_json) {
            cJSON* pos = cJSON_GetObjectItem(node, "position");
            cJSON* color = cJSON_GetObjectItem(node, "color");
            cJSON* radius = cJSON_GetObjectItem(node, "radius");
            cJSON* label = cJSON_GetObjectItem(node, "label");
            
            Vector3 position(0, 0, 0);
            if (cJSON_IsArray(pos) && cJSON_GetArraySize(pos) >= 3) {
                position.x = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 0)));
                position.y = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 1)));
                position.z = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(pos, 2)));
            }
            
            Color node_color = RED;
            if (cJSON_IsArray(color) && cJSON_GetArraySize(color) >= 3) {
                node_color.r = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 0)));
                node_color.g = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 1)));
                node_color.b = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 2)));
                node_color.a = cJSON_GetArraySize(color) >= 4 ? 
                    static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 3))) : 255;
            }
            
            float node_radius = cJSON_IsNumber(radius) ? static_cast<float>(cJSON_GetNumberValue(radius)) : 0.5f;
            std::string node_label = cJSON_IsString(label) ? cJSON_GetStringValue(label) : "";
            
            add_node(position, node_color, node_radius, node_label);
        }
    }
    
    cJSON* edges_json = cJSON_GetObjectItem(json, "edges");
    if (cJSON_IsArray(edges_json)) {
        cJSON* edge;
        cJSON_ArrayForEach(edge, edges_json) {
            cJSON* from = cJSON_GetObjectItem(edge, "from");
            cJSON* to = cJSON_GetObjectItem(edge, "to");
            cJSON* color = cJSON_GetObjectItem(edge, "color");
            cJSON* thickness = cJSON_GetObjectItem(edge, "thickness");
            
            uint32_t from_id = cJSON_IsNumber(from) ? static_cast<uint32_t>(cJSON_GetNumberValue(from)) : 0;
            uint32_t to_id = cJSON_IsNumber(to) ? static_cast<uint32_t>(cJSON_GetNumberValue(to)) : 0;
            
            Color edge_color = BLUE;
            if (cJSON_IsArray(color) && cJSON_GetArraySize(color) >= 3) {
                edge_color.r = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 0)));
                edge_color.g = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 1)));
                edge_color.b = static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 2)));
                edge_color.a = cJSON_GetArraySize(color) >= 4 ? 
                    static_cast<uint8_t>(cJSON_GetNumberValue(cJSON_GetArrayItem(color, 3))) : 255;
            }
            
            float edge_thickness = cJSON_IsNumber(thickness) ? static_cast<float>(cJSON_GetNumberValue(thickness)) : 1.0f;
            
            add_edge(from_id, to_id, edge_color, edge_thickness);
        }
    }
    
    cJSON_Delete(json);
    return true;
}

void Graph3D::generate_sample() {
    const float radius = 5.0f;
    const uint32_t node_count_sample = 8;
    
    for (uint32_t i = 0; i < node_count_sample; i++) {
        float angle = (2.0f * M_PI * i) / node_count_sample;
        Vector3 pos(
            radius * std::cos(angle),
            std::sin(angle * 2.0f) * 2.0f,
            radius * std::sin(angle)
        );
        
        Color colors[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, DARKBLUE, ORANGE, PURPLE};
        Color color = colors[i % 8];
        
        std::string label = "Node" + std::to_string(i);
        
        add_node(pos, color, 0.5f, label);
    }
    
    for (uint32_t i = 0; i < node_count_sample; i++) {
        uint32_t next = (i + 1) % node_count_sample;
        add_edge(i, next, WHITE, 2.0f);
        
        if (i < node_count_sample / 2) {
            uint32_t opposite = (i + node_count_sample / 2) % node_count_sample;
            add_edge(i, opposite, GRAY, 1.0f);
        }
    }
}

void Graph3D::center_graph() {
    if (node_count == 0) return;
    
    // Calculate center of mass
    Vector3 center_of_mass(0, 0, 0);
    for (uint32_t i = 0; i < node_count; i++) {
        center_of_mass = center_of_mass + nodes[i].position;
    }
    center_of_mass = center_of_mass * (1.0f / node_count);
    
    // Translate all nodes to center around origin
    for (uint32_t i = 0; i < node_count; i++) {
        nodes[i].position = nodes[i].position - center_of_mass;
    }
}