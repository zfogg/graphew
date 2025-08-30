#include "replay_parser.hpp"
#include "fileutils.hpp"
#include <cjson/cJSON.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <map>

// AgentInventoryState methods
float AgentInventoryState::get_inventory_at_time(const std::string& item, int timestep) const {
    auto it = inventory_over_time.find(item);
    if (it == inventory_over_time.end()) return 0.0f;
    
    const auto& values = it->second;
    if (values.empty()) return 0.0f;
    
    // Find the most recent value before or at timestep
    float current_value = 0.0f;
    for (const auto& tv : values) {
        if (tv.timestep <= timestep) {
            current_value = tv.value;
        } else {
            break;
        }
    }
    return current_value;
}

float AgentInventoryState::get_reward_at_time(int timestep) const {
    for (const auto& tv : reward_over_time) {
        if (tv.timestep == timestep) return tv.value;
    }
    return 0.0f;
}

float AgentInventoryState::get_total_reward_at_time(int timestep) const {
    float current_total = 0.0f;
    for (const auto& tv : total_reward_over_time) {
        if (tv.timestep <= timestep) {
            current_total = tv.value;
        } else {
            break;
        }
    }
    return current_total;
}

Vector3 AgentInventoryState::get_location_at_time(int timestep) const {
    Vector3 current_location(0, 0, 0);
    for (const auto& loc : location_over_time) {
        if (loc.first <= timestep) {
            current_location = loc.second;
        } else {
            break;
        }
    }
    return current_location;
}

// ReplayData methods
void ReplayData::add_agent(const AgentInventoryState& agent) {
    agents.push_back(agent);
}

const AgentInventoryState* ReplayData::get_agent(int agent_id) const {
    for (const auto& agent : agents) {
        if (agent.agent_id == agent_id) {
            return &agent;
        }
    }
    return nullptr;
}

std::vector<int> ReplayData::get_agent_ids() const {
    std::vector<int> ids;
    for (const auto& agent : agents) {
        ids.push_back(agent.agent_id);
    }
    return ids;
}

// ReplayParser methods
bool ReplayParser::parse_replay_file(const std::string& filename, ReplayData& replay_data) {
    size_t file_size;
    char* data = read_file_raw(filename.c_str(), &file_size);
    if (!data) return false;
    
    bool result = parse_json_data(data, replay_data);
    std::free(data);
    return result;
}

bool ReplayParser::parse_compressed_replay_file(const std::string& filename, ReplayData& replay_data) {
    size_t decompressed_size;
    char* data = read_compressed_file(filename.c_str(), &decompressed_size);
    if (!data) return false;
    
    bool result = parse_json_data(data, replay_data);
    std::free(data);
    return result;
}

bool ReplayParser::parse_json_data(const char* json_str, ReplayData& replay_data) {
    cJSON* json = cJSON_Parse(json_str);
    if (!json) return false;
    
    // Parse inventory items list
    cJSON* inventory_items = cJSON_GetObjectItem(json, "inventory_items");
    if (cJSON_IsArray(inventory_items)) {
        cJSON* item;
        cJSON_ArrayForEach(item, inventory_items) {
            if (cJSON_IsString(item)) {
                replay_data.inventory_items.push_back(cJSON_GetStringValue(item));
            }
        }
    }
    
    // Parse object types
    cJSON* object_types = cJSON_GetObjectItem(json, "object_types");
    if (cJSON_IsArray(object_types)) {
        cJSON* type;
        cJSON_ArrayForEach(type, object_types) {
            if (cJSON_IsString(type)) {
                replay_data.object_types.push_back(cJSON_GetStringValue(type));
            }
        }
    }
    
    // Parse grid objects and extract agents
    cJSON* grid_objects = cJSON_GetObjectItem(json, "grid_objects");
    if (cJSON_IsArray(grid_objects)) {
        cJSON* obj;
        cJSON_ArrayForEach(obj, grid_objects) {
            cJSON* type = cJSON_GetObjectItem(obj, "type");
            
            // Only process agents (type 0)
            if (cJSON_IsNumber(type) && cJSON_GetNumberValue(type) == 0) {
                AgentInventoryState agent;
                
                cJSON* agent_id = cJSON_GetObjectItem(obj, "agent_id");
                agent.agent_id = cJSON_IsNumber(agent_id) ? static_cast<int>(cJSON_GetNumberValue(agent_id)) : 0;
                
                // Parse location trajectory
                cJSON* location = cJSON_GetObjectItem(obj, "location");
                if (cJSON_IsArray(location)) {
                    cJSON* loc_entry;
                    cJSON_ArrayForEach(loc_entry, location) {
                        if (cJSON_IsArray(loc_entry) && cJSON_GetArraySize(loc_entry) >= 2) {
                            int timestep = static_cast<int>(cJSON_GetNumberValue(cJSON_GetArrayItem(loc_entry, 0)));
                            cJSON* pos_array = cJSON_GetArrayItem(loc_entry, 1);
                            
                            Vector3 position = parse_location_array(pos_array);
                            agent.location_over_time.push_back({timestep, position});
                        }
                    }
                }
                
                // Parse inventory data for each item
                for (const std::string& item : replay_data.inventory_items) {
                    std::string inv_key = "inv:" + item;
                    cJSON* inv_data = cJSON_GetObjectItem(obj, inv_key.c_str());
                    if (cJSON_IsArray(inv_data)) {
                        std::vector<TimestampValue> values;
                        parse_timestamp_array(inv_data, values);
                        agent.inventory_over_time[item] = values;
                    }
                }
                
                // Parse reward data
                cJSON* reward = cJSON_GetObjectItem(obj, "reward");
                if (cJSON_IsArray(reward)) {
                    parse_timestamp_array(reward, agent.reward_over_time);
                }
                
                cJSON* total_reward = cJSON_GetObjectItem(obj, "total_reward");
                if (cJSON_IsArray(total_reward)) {
                    parse_timestamp_array(total_reward, agent.total_reward_over_time);
                }
                
                replay_data.add_agent(agent);
            }
        }
    }
    
    // Calculate max timestep
    replay_data.max_timestep = 0;
    for (const auto& agent : replay_data.agents) {
        for (const auto& loc : agent.location_over_time) {
            replay_data.max_timestep = std::max(replay_data.max_timestep, loc.first);
        }
    }
    
    cJSON_Delete(json);
    return true;
}

void ReplayParser::parse_timestamp_array(cJSON* array, std::vector<TimestampValue>& output) {
    if (!cJSON_IsArray(array)) return;
    
    cJSON* entry;
    cJSON_ArrayForEach(entry, array) {
        if (cJSON_IsArray(entry) && cJSON_GetArraySize(entry) >= 2) {
            int timestep = static_cast<int>(cJSON_GetNumberValue(cJSON_GetArrayItem(entry, 0)));
            float value = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(entry, 1)));
            output.emplace_back(timestep, value);
        }
    }
}

Vector3 ReplayParser::parse_location_array(cJSON* location_array) {
    if (!cJSON_IsArray(location_array) || cJSON_GetArraySize(location_array) < 2) {
        return Vector3(0, 0, 0);
    }
    
    float x = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(location_array, 0)));
    float y = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(location_array, 1)));
    float z = 0.0f; // 2D gridworld, ignore z or use for layers
    
    if (cJSON_GetArraySize(location_array) >= 3) {
        z = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(location_array, 2)));
    }
    
    return Vector3(x, y, z);
}

// AgentGraphBuilder methods
void AgentGraphBuilder::build_inventory_dimensional_graph(const ReplayData& replay, Graph3D& graph3d, 
                                                          const std::vector<std::string>& inventory_dims) {
    std::cout << "Building agent state space graph (nodes = unique inventory states)\n";
    
    // Create nodes for unique inventory state combinations
    std::map<std::string, uint32_t> state_to_node_id;
    std::vector<std::string> node_id_to_state;
    
    // Find all unique inventory state combinations across all agents and timesteps
    for (const auto& agent : replay.agents) {
        // Sample key timesteps for this agent
        std::vector<int> key_timesteps;
        for (const auto& [item, values] : agent.inventory_over_time) {
            for (const auto& tv : values) {
                key_timesteps.push_back(tv.timestep);
            }
        }
        
        // Add reward change timesteps
        for (const auto& tv : agent.total_reward_over_time) {
            key_timesteps.push_back(tv.timestep);
        }
        
        // Remove duplicates and sort
        std::sort(key_timesteps.begin(), key_timesteps.end());
        key_timesteps.erase(std::unique(key_timesteps.begin(), key_timesteps.end()), key_timesteps.end());
        
        // Create nodes for each unique state
        for (int timestep : key_timesteps) {
            // Create state signature
            std::string state_sig = "";
            for (const std::string& item : inventory_dims) {
                if (item == "time") {
                    state_sig += "T" + std::to_string(timestep / 100) + "_"; // Coarse time buckets
                } else {
                    int qty = static_cast<int>(agent.get_inventory_at_time(item, timestep));
                    state_sig += item + std::to_string(qty) + "_";
                }
            }
            int reward_bucket = static_cast<int>(agent.get_total_reward_at_time(timestep));
            state_sig += "R" + std::to_string(reward_bucket);
            
            // Add node if this state is new
            if (state_to_node_id.find(state_sig) == state_to_node_id.end()) {
                // Generate position based on state properties, not coordinates
                float angle = static_cast<float>(state_to_node_id.size()) * 0.618f; // Golden angle
                float radius = 3.0f + static_cast<float>(reward_bucket) * 0.5f;
                float height = static_cast<float>(reward_bucket) * 2.0f;
                
                Vector3 position(
                    radius * std::cos(angle),
                    height,
                    radius * std::sin(angle)
                );
                
                Color color = reward_to_color(static_cast<float>(reward_bucket), 10.0f);
                float node_radius = 0.3f + static_cast<float>(reward_bucket) * 0.1f;
                
                std::string label = "State_R" + std::to_string(reward_bucket);
                uint32_t node_id = graph3d.add_node(position, color, node_radius, label);
                
                state_to_node_id[state_sig] = node_id;
                node_id_to_state.push_back(state_sig);
            }
        }
    }
    
    // Create edges between similar states (representing possible transitions)
    for (size_t i = 0; i < node_id_to_state.size(); i++) {
        for (size_t j = i + 1; j < node_id_to_state.size(); j++) {
            // Connect states that differ by small inventory changes
            const std::string& state1 = node_id_to_state[i];
            const std::string& state2 = node_id_to_state[j];
            
            // Simple heuristic: connect if states are "similar" 
            // (In real implementation, this would use actual game transition rules)
            if (state1.length() == state2.length()) {
                int differences = 0;
                for (size_t k = 0; k < state1.length(); k++) {
                    if (state1[k] != state2[k]) differences++;
                }
                
                // Connect if only a few characters differ (indicating small inventory change)
                if (differences >= 2 && differences <= 8) {
                    graph3d.add_edge(static_cast<uint32_t>(i), static_cast<uint32_t>(j), GRAY, 1.0f);
                }
            }
        }
    }
    
    std::cout << "Created " << state_to_node_id.size() << " unique inventory states\n";
}

void AgentGraphBuilder::build_temporal_graph(const ReplayData& replay, Graph3D& graph3d, int target_agent_id) {
    std::cout << "Building temporal graph for agent trajectories\n";
    
    for (const auto& agent : replay.agents) {
        if (target_agent_id >= 0 && agent.agent_id != target_agent_id) continue;
        
        // Sample locations at intervals
        int interval = std::max(1, static_cast<int>(agent.location_over_time.size()) / 20);
        
        for (size_t i = 0; i < agent.location_over_time.size(); i += interval) {
            const auto& loc_entry = agent.location_over_time[i];
            int timestep = loc_entry.first;
            Vector3 grid_pos = loc_entry.second;
            
            // Scale grid coordinates for visualization
            Vector3 position(grid_pos.x * 0.5f, grid_pos.y * 0.5f, agent.agent_id * 2.0f);
            
            float total_reward = agent.get_total_reward_at_time(timestep);
            Color color = reward_to_color(total_reward, 10.0f);
            
            std::string label = "A" + std::to_string(agent.agent_id) + "_T" + std::to_string(timestep);
            
            graph3d.add_node(position, color, 0.4f, label);
        }
    }
    
    // Connect consecutive timesteps for each agent
    // (Implementation would track node indices per agent)
}

Vector3 AgentGraphBuilder::inventory_to_position(const AgentInventoryState& agent, 
                                                const std::vector<std::string>& dimensions, 
                                                int timestep) {
    float x, y, z;
    
    if (dimensions[0] == "time") {
        x = static_cast<float>(timestep);
    } else {
        x = agent.get_inventory_at_time(dimensions[0], timestep);
    }
    
    if (dimensions[1] == "time") {
        y = static_cast<float>(timestep);
    } else {
        y = agent.get_inventory_at_time(dimensions[1], timestep);
    }
    
    if (dimensions[2] == "time") {
        z = static_cast<float>(timestep);
    } else {
        z = agent.get_inventory_at_time(dimensions[2], timestep);
    }
    
    // Add small jitter based on agent ID to separate overlapping nodes
    float jitter = 0.3f;
    float agent_offset = agent.agent_id * 0.1f;
    x += std::sin(agent_offset) * jitter;
    y += std::cos(agent_offset) * jitter;
    z += std::sin(agent_offset * 1.7f) * jitter;
    
    // Scale to reasonable visualization range
    float scale = 2.0f;
    return Vector3(x * scale, y * scale, z * scale);
}

Color AgentGraphBuilder::reward_to_color(float total_reward, float max_reward) {
    float normalized = std::min(1.0f, total_reward / max_reward);
    
    if (normalized < 0.5f) {
        // Red to Yellow (low to medium reward)
        uint8_t red = 255;
        uint8_t green = static_cast<uint8_t>(normalized * 2.0f * 255);
        uint8_t blue = 0;
        return Color(red, green, blue, 255);
    } else {
        // Yellow to Green (medium to high reward)
        uint8_t red = static_cast<uint8_t>((1.0f - normalized) * 2.0f * 255);
        uint8_t green = 255;
        uint8_t blue = 0;
        return Color(red, green, blue, 255);
    }
}

float AgentGraphBuilder::calculate_agent_similarity(const AgentInventoryState& agent1, 
                                                   const AgentInventoryState& agent2, 
                                                   int timestep) {
    float similarity = 0.0f;
    int item_count = 0;
    
    // Compare inventory states
    for (const auto& [item, values] : agent1.inventory_over_time) {
        float val1 = agent1.get_inventory_at_time(item, timestep);
        float val2 = agent2.get_inventory_at_time(item, timestep);
        
        float diff = std::abs(val1 - val2);
        similarity += 1.0f / (1.0f + diff); // Inverse distance similarity
        item_count++;
    }
    
    return item_count > 0 ? similarity / item_count : 0.0f;
}