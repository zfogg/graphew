#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include "graph.hpp"

// Represents a state in the filtered inventory space
struct InventoryState {
    std::map<std::string, int> items;  // item_name -> quantity
    int timestep;
    int agent_id;
    
    // Create a unique key for this state based on selected items (ignoring timestep)
    std::string get_key(const std::set<std::string>& selected_items) const {
        std::string key;
        
        // If no items selected, use all items in state
        if (selected_items.empty()) {
            for (const auto& [item, quantity] : items) {
                if (!key.empty()) key += ",";
                key += item + ":" + std::to_string(quantity);
            }
        } else {
            // Use only selected items
            for (const auto& item : selected_items) {
                auto it = items.find(item);
                int quantity = (it != items.end()) ? it->second : 0;
                if (!key.empty()) key += ",";
                key += item + ":" + std::to_string(quantity);
            }
        }
        return key;
    }
    
    // Calculate similarity/distance to another state
    float distance_to(const InventoryState& other, const std::set<std::string>& tracked) const {
        float dist = 0.0f;
        std::set<std::string> all_items;
        
        // Collect all items from both states
        for (const auto& [item, _] : items) {
            if (tracked.empty() || tracked.count(item) > 0) {
                all_items.insert(item);
            }
        }
        for (const auto& [item, _] : other.items) {
            if (tracked.empty() || tracked.count(item) > 0) {
                all_items.insert(item);
            }
        }
        
        // Calculate Euclidean distance in item space
        for (const auto& item : all_items) {
            auto it1 = items.find(item);
            auto it2 = other.items.find(item);
            int q1 = (it1 != items.end()) ? it1->second : 0;
            int q2 = (it2 != other.items.end()) ? it2->second : 0;
            dist += (q1 - q2) * (q1 - q2);
        }
        
        return std::sqrt(dist);
    }
};

// Configuration for inventory-based filtering
struct InventoryFilterConfig {
    // Items to track (if empty, track all)
    std::set<std::string> tracked_items;
    
    // Whether to create separate nodes for each agent (usually false)
    bool separate_by_agent = false;
    
    // Whether to show only states where inventory changed
    bool only_changes = true;
    
    // Minimum quantity to consider (filter out zero quantities)
    int min_quantity = 0;
    
    // Time window to consider (-1 for all time)
    int min_timestep = -1;
    int max_timestep = -1;
    
    // Visual options
    bool color_by_total = false;  // Color nodes by total inventory value
    bool size_by_frequency = false;  // Size nodes by how often state occurs
    float node_scale = 5.0f;  // Scale factor for positioning
    
    // Layout algorithm for abstract space
    enum LayoutMode {
        FORCE_DIRECTED,  // Use force-directed layout in N dimensions
        PCA,            // Principal component analysis for dimensionality reduction
        MDS,            // Multi-dimensional scaling based on state similarity
        TSNE            // t-SNE for complex state space visualization
    } layout_mode = FORCE_DIRECTED;
    
    // Reset to defaults
    void reset() {
        *this = InventoryFilterConfig();
    }
};

class InventoryFilter {
public:
    // Build a filtered graph based on inventory states
    static void build_inventory_graph(
        Graph3D& output_graph,
        const std::vector<InventoryState>& states,
        const InventoryFilterConfig& config
    );
    
    // Extract inventory states from replay data
    static std::vector<InventoryState> extract_inventory_states(
        const std::string& replay_file
    );
    
    // Create a graph showing transitions between inventory states
    static void create_transition_graph(
        Graph3D& graph,
        const std::vector<InventoryState>& states,
        const InventoryFilterConfig& config
    ) {
        // Map from state key to node index
        std::map<std::string, uint32_t> state_to_node;
        std::map<std::string, int> state_frequency;
        
        // First pass: identify unique states
        for (const auto& state : states) {
            std::string key = state.get_key(config.tracked_items);
            state_frequency[key]++;
        }
        
        // Create nodes for unique states
        // First, we need to assign positions based on the layout mode
        std::map<std::string, Vector3> state_positions;
        
        if (config.layout_mode == InventoryFilterConfig::FORCE_DIRECTED) {
            // Positions will be determined by force-directed layout later
            // For now, assign random initial positions
            int idx = 0;
            for (const auto& [key, freq] : state_frequency) {
                float angle = 2.0f * 3.14159f * idx / state_frequency.size();
                float radius = 10.0f;
                state_positions[key] = Vector3(
                    radius * std::cos(angle),
                    radius * std::sin(angle),
                    (state_frequency.size() > 20) ? (idx % 5 - 2) * 2.0f : 0.0f
                );
                idx++;
            }
        } else {
            // For other layout modes, compute positions based on state distances
            // This is a simplified MDS-like approach
            std::vector<std::string> keys;
            for (const auto& [key, _] : state_frequency) {
                keys.push_back(key);
            }
            
            // Build distance matrix
            std::vector<std::vector<float>> distances(keys.size(), std::vector<float>(keys.size(), 0.0f));
            std::map<std::string, InventoryState> key_to_state;
            
            // Find representative states
            for (const auto& state : states) {
                std::string key = state.get_key(config.tracked_items);
                if (key_to_state.find(key) == key_to_state.end()) {
                    key_to_state[key] = state;
                }
            }
            
            // Calculate distances
            for (size_t i = 0; i < keys.size(); i++) {
                for (size_t j = i + 1; j < keys.size(); j++) {
                    float dist = key_to_state[keys[i]].distance_to(key_to_state[keys[j]], config.tracked_items);
                    distances[i][j] = dist;
                    distances[j][i] = dist;
                }
            }
            
            // Simple force-based embedding in 3D based on distances
            for (size_t i = 0; i < keys.size(); i++) {
                float x = 0, y = 0, z = 0;
                for (size_t j = 0; j < keys.size(); j++) {
                    if (i != j && distances[i][j] > 0) {
                        float angle1 = 2.0f * 3.14159f * j / keys.size();
                        float angle2 = 3.14159f * j / keys.size();
                        x += distances[i][j] * std::cos(angle1);
                        y += distances[i][j] * std::sin(angle1);
                        z += distances[i][j] * std::cos(angle2) * 0.5f;
                    }
                }
                state_positions[keys[i]] = Vector3(x, y, z) * config.node_scale;
            }
        }
        
        // Create nodes with computed positions
        for (const auto& [key, freq] : state_frequency) {
            // Find a representative state for this key
            for (const auto& state : states) {
                if (state.get_key(config.tracked_items) == key) {
                    Vector3 pos = state_positions[key];
                    
                    // Color based on configuration
                    Color color = RED;
                    if (config.color_by_total) {
                        int total = 0;
                        for (const auto& [item, qty] : state.items) {
                            if (config.tracked_items.empty() || 
                                config.tracked_items.count(item) > 0) {
                                total += qty;
                            }
                        }
                        // Map total to color gradient
                        float t = std::min(1.0f, total / 100.0f);
                        color = Color(
                            static_cast<uint8_t>(255 * (1 - t)),
                            static_cast<uint8_t>(255 * t),
                            128
                        );
                    }
                    
                    // Size based on frequency
                    float radius = config.size_by_frequency ? 
                        0.3f + 0.7f * std::min(1.0f, freq / 10.0f) : 0.5f;
                    
                    uint32_t node_id = graph.add_node(pos, color, radius, key);
                    state_to_node[key] = node_id;
                    
                    // Store metadata in the node
                    if (node_id < MAX_NODES) {
                        graph.nodes[node_id].type = "inventory_state";
                        graph.nodes[node_id].value = static_cast<float>(freq);
                        
                        // Store the inventory items as properties
                        for (const auto& [item, qty] : state.items) {
                            if (config.tracked_items.empty() || 
                                config.tracked_items.count(item) > 0) {
                                graph.nodes[node_id].properties[item] = std::to_string(qty);
                            }
                        }
                    }
                    break;
                }
            }
        }
        
        // Second pass: create edges for transitions
        for (size_t i = 1; i < states.size(); i++) {
            const auto& prev_state = states[i-1];
            const auto& curr_state = states[i];
            
            // Only connect consecutive states from same agent
            if (config.separate_by_agent && prev_state.agent_id != curr_state.agent_id) {
                continue;
            }
            
            std::string prev_key = prev_state.get_key(config.tracked_items);
            std::string curr_key = curr_state.get_key(config.tracked_items);
            
            // Skip if no change and we only want changes
            if (config.only_changes && prev_key == curr_key) {
                continue;
            }
            
            auto prev_it = state_to_node.find(prev_key);
            auto curr_it = state_to_node.find(curr_key);
            
            if (prev_it != state_to_node.end() && curr_it != state_to_node.end()) {
                // Color edge based on what changed
                Color edge_color = BLUE;
                bool increased = false;
                bool decreased = false;
                
                for (const auto& item : config.tracked_items) {
                    auto p = prev_state.items.find(item);
                    auto c = curr_state.items.find(item);
                    int prev_qty = (p != prev_state.items.end()) ? p->second : 0;
                    int curr_qty = (c != curr_state.items.end()) ? c->second : 0;
                    
                    if (curr_qty > prev_qty) increased = true;
                    if (curr_qty < prev_qty) decreased = true;
                }
                
                if (increased && !decreased) {
                    edge_color = GREEN;  // Gained items
                } else if (decreased && !increased) {
                    edge_color = Color(255, 128, 0);  // Lost items (orange)
                } else if (increased && decreased) {
                    edge_color = YELLOW;  // Mixed change
                }
                
                graph.add_edge(prev_it->second, curr_it->second, edge_color, 1.0f);
            }
        }
    }
};

// Helper to create common inventory filter configurations
namespace InventoryFilterPresets {
    inline InventoryFilterConfig hearts_only() {
        InventoryFilterConfig config;
        config.tracked_items = {"heart"};
        config.size_by_frequency = true;
        return config;
    }
    
    inline InventoryFilterConfig ore_tracking() {
        InventoryFilterConfig config;
        config.tracked_items = {"red_ore", "blue_ore", "green_ore"};
        config.color_by_total = true;
        config.layout_mode = InventoryFilterConfig::MDS;  // Use MDS to show ore relationships
        return config;
    }
    
    inline InventoryFilterConfig hearts_and_ore() {
        InventoryFilterConfig config;
        config.tracked_items = {"heart", "red_ore", "blue_ore"};
        config.color_by_total = true;
        return config;
    }
    
    inline InventoryFilterConfig all_items() {
        InventoryFilterConfig config;
        // Track all items (empty set means all)
        config.tracked_items.clear();
        config.size_by_frequency = true;
        config.color_by_total = true;
        config.layout_mode = InventoryFilterConfig::FORCE_DIRECTED;
        return config;
    }
    
    inline InventoryFilterConfig specific_items(const std::set<std::string>& items) {
        InventoryFilterConfig config;
        config.tracked_items = items;
        config.size_by_frequency = true;
        // Use MDS for small sets, force-directed for large sets
        config.layout_mode = (items.size() <= 5) ? 
            InventoryFilterConfig::MDS : InventoryFilterConfig::FORCE_DIRECTED;
        return config;
    }
}
