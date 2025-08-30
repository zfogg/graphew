#include "replay_parser.hpp"
#include "fileutils.hpp"
#include "force_layout.hpp"
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
    
    // Detect format type and parse inventory items list
    bool is_grid_objects_format = cJSON_GetObjectItem(json, "grid_objects") != nullptr;
    bool is_objects_format = cJSON_GetObjectItem(json, "objects") != nullptr;
    bool is_pufferbox_format = false; // Pufferbox format with agent:inv: prefix
    
    // Check for pufferbox format by looking at inventory_items field (reliable method)
    if (is_grid_objects_format) {
        cJSON* inventory_items = cJSON_GetObjectItem(json, "inventory_items");
        if (cJSON_IsArray(inventory_items) && cJSON_GetArraySize(inventory_items) > 0) {
            cJSON* first_item = cJSON_GetArrayItem(inventory_items, 0);
            if (cJSON_IsString(first_item)) {
                std::string item_name = cJSON_GetStringValue(first_item);
                // Pufferbox format uses dots (ore.red), regular format uses underscores (ore_red)
                if (item_name.find('.') != std::string::npos) {
                    is_pufferbox_format = true;
                    std::cout << "Detected pufferbox format (dots in item names)" << std::endl;
                }
            }
        }
    }
    
    if (is_grid_objects_format) {
        // Parse grid_objects format (sample.json)
        cJSON* inventory_items = cJSON_GetObjectItem(json, "inventory_items");
        if (cJSON_IsArray(inventory_items)) {
            cJSON* item;
            cJSON_ArrayForEach(item, inventory_items) {
                if (cJSON_IsString(item)) {
                    replay_data.inventory_items.push_back(cJSON_GetStringValue(item));
                }
            }
        }
    } else if (is_objects_format) {
        // Parse objects format (sample2.json, sample3.json)
        cJSON* item_names = cJSON_GetObjectItem(json, "item_names");
        if (cJSON_IsArray(item_names)) {
            cJSON* item;
            cJSON_ArrayForEach(item, item_names) {
                if (cJSON_IsString(item)) {
                    replay_data.inventory_items.push_back(cJSON_GetStringValue(item));
                }
            }
        }
    }
    
    // Parse object types (different field names)
    cJSON* object_types = nullptr;
    if (is_grid_objects_format) {
        object_types = cJSON_GetObjectItem(json, "object_types");
    } else if (is_objects_format) {
        object_types = cJSON_GetObjectItem(json, "type_names");
    }
    
    if (cJSON_IsArray(object_types)) {
        cJSON* type;
        cJSON_ArrayForEach(type, object_types) {
            if (cJSON_IsString(type)) {
                replay_data.object_types.push_back(cJSON_GetStringValue(type));
            }
        }
    }
    
    // Parse agents from either format
    cJSON* objects_array = nullptr;
    if (is_grid_objects_format) {
        objects_array = cJSON_GetObjectItem(json, "grid_objects");
    } else if (is_objects_format) {
        objects_array = cJSON_GetObjectItem(json, "objects");
    }
    
    if (cJSON_IsArray(objects_array)) {
        cJSON* obj;
        cJSON_ArrayForEach(obj, objects_array) {
            // Check for agent type in both formats
            bool is_agent = false;
            if (is_grid_objects_format) {
                cJSON* type = cJSON_GetObjectItem(obj, "type");
                is_agent = (cJSON_IsNumber(type) && cJSON_GetNumberValue(type) == 0);
            } else if (is_objects_format) {
                cJSON* type_id = cJSON_GetObjectItem(obj, "type_id");
                is_agent = (cJSON_IsNumber(type_id) && cJSON_GetNumberValue(type_id) == 0);
            }
            
            if (is_agent) {
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
                
                std::cout << "Parsing agent " << agent.agent_id << " - format: " 
                          << (is_pufferbox_format ? "pufferbox" : (is_objects_format ? "objects" : "grid_objects")) << std::endl;
                
                // Parse inventory data based on format
                if (is_grid_objects_format && !is_pufferbox_format) {
                    // grid_objects format: "inv:item_name" arrays
                    for (const std::string& item : replay_data.inventory_items) {
                        std::string inv_key = "inv:" + item;
                        cJSON* inv_data = cJSON_GetObjectItem(obj, inv_key.c_str());
                        if (cJSON_IsArray(inv_data)) {
                            std::vector<TimestampValue> values;
                            parse_timestamp_array(inv_data, values);
                            agent.inventory_over_time[item] = values;
                        }
                    }
                    
                    // Parse reward data for grid_objects format
                    cJSON* reward = cJSON_GetObjectItem(obj, "reward");
                    if (cJSON_IsArray(reward)) {
                        parse_timestamp_array(reward, agent.reward_over_time);
                    }
                    
                    cJSON* total_reward = cJSON_GetObjectItem(obj, "total_reward");
                    if (cJSON_IsArray(total_reward)) {
                        parse_timestamp_array(total_reward, agent.total_reward_over_time);
                    }
                } else if (is_pufferbox_format) {
                    // Pufferbox format with agent:inv: prefix and dots in names
                    std::vector<std::string> agent_inv_items = {
                        "ore.red", "ore.blue", "ore.green", 
                        "battery", "heart", "armor", "laser", "blueprint"
                    };
                    for (const std::string& item : agent_inv_items) {
                        std::string inv_key = "agent:inv:" + item;
                        cJSON* inv_data = cJSON_GetObjectItem(obj, inv_key.c_str());
                        if (inv_data && cJSON_IsArray(inv_data) && cJSON_GetArraySize(inv_data) > 0) {
                            std::vector<TimestampValue> values;
                            parse_timestamp_array(inv_data, values);
                            // Convert dot notation to underscore for consistency
                            std::string clean_item = item;
                            std::replace(clean_item.begin(), clean_item.end(), '.', '_');
                            agent.inventory_over_time[clean_item] = values;
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
                } else if (is_objects_format) {
                    // objects format: "inventory" with item_id arrays
                    cJSON* inventory = cJSON_GetObjectItem(obj, "inventory");
                    if (cJSON_IsArray(inventory)) {
                        parse_objects_inventory(inventory, agent, replay_data.inventory_items);
                    }
                    
                    // Parse reward data (different structure)
                    cJSON* current_reward = cJSON_GetObjectItem(obj, "current_reward");
                    if (cJSON_IsArray(current_reward)) {
                        parse_timestamp_array(current_reward, agent.reward_over_time);
                    }
                    
                    cJSON* total_reward = cJSON_GetObjectItem(obj, "total_reward");
                    if (cJSON_IsArray(total_reward)) {
                        parse_timestamp_array(total_reward, agent.total_reward_over_time);
                    }
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

void ReplayParser::parse_objects_inventory(cJSON* inventory_array, AgentInventoryState& agent, 
                                          const std::vector<std::string>& item_names) {
    if (!cJSON_IsArray(inventory_array)) {
        std::cout << "ERROR: objects inventory is not an array!" << std::endl;
        return;
    }
    
    std::cout << "Parsing objects inventory with " << cJSON_GetArraySize(inventory_array) << " entries" << std::endl;
    
    // Initialize inventory tracking for all items
    for (const std::string& item : item_names) {
        agent.inventory_over_time[item] = std::vector<TimestampValue>();
    }
    
    cJSON* inv_entry;
    cJSON_ArrayForEach(inv_entry, inventory_array) {
        if (!cJSON_IsArray(inv_entry) || cJSON_GetArraySize(inv_entry) < 2) continue;
        
        int timestep = static_cast<int>(cJSON_GetNumberValue(cJSON_GetArrayItem(inv_entry, 0)));
        cJSON* items_array = cJSON_GetArrayItem(inv_entry, 1);
        
        if (!cJSON_IsArray(items_array)) continue;
        
        // Parse each item in inventory at this timestep
        cJSON* item_entry;
        cJSON_ArrayForEach(item_entry, items_array) {
            if (!cJSON_IsArray(item_entry) || cJSON_GetArraySize(item_entry) < 2) continue;
            
            int item_id = static_cast<int>(cJSON_GetNumberValue(cJSON_GetArrayItem(item_entry, 0)));
            float quantity = static_cast<float>(cJSON_GetNumberValue(cJSON_GetArrayItem(item_entry, 1)));
            
            // Map item_id to item name
            if (item_id >= 0 && item_id < static_cast<int>(item_names.size())) {
                const std::string& item_name = item_names[item_id];
                agent.inventory_over_time[item_name].emplace_back(timestep, quantity);
            }
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
            float total_reward = agent.get_total_reward_at_time(timestep);
            int reward_bucket = static_cast<int>(total_reward * REWARD_BUCKET_SCALE); // Scale up for better bucketing
            state_sig += "R" + std::to_string(reward_bucket);
            
            // Add node if this state is new
            if (state_to_node_id.find(state_sig) == state_to_node_id.end()) {
                // Smart initialization based on inventory state properties
                float ore_qty = 0, battery_qty = 0, heart_qty = 0;
                for (const std::string& item : inventory_dims) {
                    if (item == "ore_red") ore_qty = agent.get_inventory_at_time(item, timestep);
                    else if (item == "battery_red") battery_qty = agent.get_inventory_at_time(item, timestep);
                    else if (item == "heart") heart_qty = agent.get_inventory_at_time(item, timestep);
                }
                
                // Initialize in organized grid pattern with reward/temporal Z-dimension
                // Calculate Z based on reward progression over time
                float max_reward_in_data = 20.0f; // Reasonable max for scaling
                float reward_z = (total_reward / max_reward_in_data) * 10.0f; // Scale to reasonable Z range
                
                // Add temporal component - later timesteps get pushed forward
                float max_timestep = 1000.0f; // Reasonable max timestep
                float temporal_z = (static_cast<float>(timestep) / max_timestep) * 5.0f;
                
                Vector3 position(
                    (ore_qty - 5.0f) * 2.0f,        // X: Center around 5 ore (typical mid-range)
                    (battery_qty - 1.0f) * 3.0f,    // Y: Center around 1 battery  
                    reward_z + temporal_z - 7.5f    // Z: Reward + time progression, centered around -7.5
                );
                
                // DEBUG: Print first few positions to compare formats
                if (state_to_node_id.size() < 5) {
                    std::cout << "Initial node " << state_to_node_id.size() << " position: (" 
                              << position.x << "," << position.y << "," << position.z 
                              << ") from inventory: ore=" << ore_qty << " battery=" << battery_qty << " heart=" << heart_qty
                              << " reward=" << total_reward << " timestep=" << timestep << std::endl;
                }
                
                Color color = reward_to_color(total_reward, MAX_REWARD_FOR_COLOR); // Use actual reward value, not bucket
                float node_radius = 0.3f + static_cast<float>(reward_bucket) * 0.1f;
                
                // DEBUG: Print color assignment for first few states
                static int debug_count = 0;
                if (debug_count < 5) {
                    std::cout << "State with reward " << reward_bucket << " gets color: (" 
                              << (int)color.r << "," << (int)color.g << "," << (int)color.b << ")" << std::endl;
                    debug_count++;
                }
                
                std::string label = "State_R" + std::to_string(reward_bucket);
                uint32_t node_id = graph3d.add_node(position, color, node_radius, label);
                
                state_to_node_id[state_sig] = node_id;
                node_id_to_state.push_back(state_sig);
            }
        }
    }
    
    // Create edges based on actual temporal agent transitions
    std::map<std::pair<std::string, std::string>, int> transition_count;
    
    // Track agent state transitions over time
    for (const auto& agent : replay.agents) {
        std::vector<int> key_timesteps;
        
        // Get all timesteps where inventory changes
        for (const auto& [item, values] : agent.inventory_over_time) {
            for (const auto& tv : values) {
                key_timesteps.push_back(tv.timestep);
            }
        }
        for (const auto& tv : agent.total_reward_over_time) {
            key_timesteps.push_back(tv.timestep);
        }
        
        // Sort and remove duplicates
        std::sort(key_timesteps.begin(), key_timesteps.end());
        key_timesteps.erase(std::unique(key_timesteps.begin(), key_timesteps.end()), key_timesteps.end());
        
        // Create state signatures for consecutive timesteps
        for (size_t t = 0; t < key_timesteps.size() - 1; t++) {
            int current_time = key_timesteps[t];
            int next_time = key_timesteps[t + 1];
            
            // Generate state signatures for current and next states
            auto create_state_sig = [&](int timestep) -> std::string {
                std::string sig = "";
                for (const std::string& item : inventory_dims) {
                    if (item == "time") {
                        sig += "T" + std::to_string(timestep / 100) + "_";
                    } else {
                        int qty = static_cast<int>(agent.get_inventory_at_time(item, timestep));
                        sig += item + std::to_string(qty) + "_";
                    }
                }
                int reward_bucket = static_cast<int>(agent.get_total_reward_at_time(timestep) * REWARD_BUCKET_SCALE);
                sig += "R" + std::to_string(reward_bucket);
                return sig;
            };
            
            std::string current_state = create_state_sig(current_time);
            std::string next_state = create_state_sig(next_time);
            
            // Only create transition if states are different
            if (current_state != next_state) {
                transition_count[{current_state, next_state}]++;
            }
        }
    }
    
    // Create edges from observed transitions
    for (const auto& [transition, count] : transition_count) {
        const std::string& from_state = transition.first;
        const std::string& to_state = transition.second;
        
        auto from_iter = state_to_node_id.find(from_state);
        auto to_iter = state_to_node_id.find(to_state);
        
        if (from_iter != state_to_node_id.end() && to_iter != state_to_node_id.end()) {
            uint32_t from_id = from_iter->second;
            uint32_t to_id = to_iter->second;
            
            // Extract reward levels for edge styling
            int from_reward = 0, to_reward = 0;
            size_t r_pos = from_state.find('R');
            if (r_pos != std::string::npos) from_reward = std::stoi(from_state.substr(r_pos + 1));
            r_pos = to_state.find('R');
            if (r_pos != std::string::npos) to_reward = std::stoi(to_state.substr(r_pos + 1));
            
            // Color edges based on reward change
            Color edge_color;
            if (to_reward > from_reward) {
                edge_color = Color(100, 255, 100, 255); // Green for reward increase
            } else if (to_reward < from_reward) {
                edge_color = Color(255, 100, 100, 255); // Red for reward decrease
            } else {
                edge_color = GRAY; // Gray for no reward change
            }
            
            // Thickness based on transition frequency
            float thickness = 1.0f + static_cast<float>(count) * 0.5f;
            
            graph3d.add_edge(from_id, to_id, edge_color, thickness);
        }
    }
    
    std::cout << "Created " << transition_count.size() << " temporal transitions between states\n";
    
    std::cout << "Created " << state_to_node_id.size() << " unique inventory states\n";
    
    // DEBUG: Check if objects format is creating any inventory data
    if (replay.inventory_items.size() > 0 && replay.agents.size() > 0) {
        const auto& sample_agent = replay.agents[0];
        std::cout << "DEBUG: Sample agent inventory data size: " << sample_agent.inventory_over_time.size() << std::endl;
        for (const auto& [item, values] : sample_agent.inventory_over_time) {
            std::cout << "  " << item << ": " << values.size() << " timesteps" << std::endl;
            if (!values.empty()) {
                std::cout << "    First value: " << values[0].value << " at timestep " << values[0].timestep << std::endl;
            }
        }
    }
    
    // Don't apply force layout here - let it happen in real-time for visualization
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
            Color color = reward_to_color(total_reward, TEMPORAL_MAX_REWARD);
            
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