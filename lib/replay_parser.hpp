#pragma once

#include "graph.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cjson/cJSON.h>

struct TimestampValue {
    int timestep;
    float value;
    
    TimestampValue(int t, float v) : timestep(t), value(v) {}
};

struct AgentInventoryState {
    int agent_id;
    std::unordered_map<std::string, std::vector<TimestampValue>> inventory_over_time;
    std::vector<TimestampValue> reward_over_time;
    std::vector<TimestampValue> total_reward_over_time;
    std::vector<std::pair<int, Vector3>> location_over_time;
    
    // Get inventory value at specific timestep
    float get_inventory_at_time(const std::string& item, int timestep) const;
    float get_reward_at_time(int timestep) const;
    float get_total_reward_at_time(int timestep) const;
    Vector3 get_location_at_time(int timestep) const;
};

struct ReplayData {
    std::vector<std::string> inventory_items;
    std::vector<std::string> object_types;
    std::vector<AgentInventoryState> agents;
    int max_timestep;
    
    void add_agent(const AgentInventoryState& agent);
    const AgentInventoryState* get_agent(int agent_id) const;
    std::vector<int> get_agent_ids() const;
};

class ReplayParser {
public:
    static bool parse_replay_file(const std::string& filename, ReplayData& replay_data);
    static bool parse_compressed_replay_file(const std::string& filename, ReplayData& replay_data);
    
private:
    static bool parse_json_data(const char* json_str, ReplayData& replay_data);
    static void parse_timestamp_array(cJSON* array, std::vector<TimestampValue>& output);
    static Vector3 parse_location_array(cJSON* location_array);
};

class AgentGraphBuilder {
public:
    // Build graph where each node represents agent state at timestep
    static void build_temporal_graph(const ReplayData& replay, Graph3D& graph3d, int agent_id = -1);
    
    // Build graph where position encodes inventory dimensions
    static void build_inventory_dimensional_graph(const ReplayData& replay, Graph3D& graph3d, 
                                                  const std::vector<std::string>& inventory_dims);
    
    // Build graph where nodes are agents and edges represent similarity
    static void build_agent_similarity_graph(const ReplayData& replay, Graph3D& graph3d, int timestep = -1);

private:
    static Vector3 inventory_to_position(const AgentInventoryState& agent, 
                                        const std::vector<std::string>& dimensions, 
                                        int timestep);
    static Color reward_to_color(float total_reward, float max_reward);
    static float calculate_agent_similarity(const AgentInventoryState& agent1, 
                                           const AgentInventoryState& agent2, 
                                           int timestep);
};