#pragma once

#include "graph.hpp"
#include <vector>

class ForceLayoutEngine {
public:
    struct PhysicsParams {
        float repel;
        float attract;
        float decay;
        float centering_strength;
        float dimension;
        int iterations;
        
        PhysicsParams() : repel(0.5f), attract(0.1f), decay(0.8f), 
                         centering_strength(0.1f), dimension(3.0f), iterations(50) {}
    };
    
    static void apply_force_layout(Graph3D& graph, const PhysicsParams& params = PhysicsParams());
    static bool apply_force_layout_step(Graph3D& graph, const PhysicsParams& params, int& remaining_iterations);
    
private:
    struct NodePhysics {
        Vector3 position;
        Vector3 velocity;
        Vector3 force;
        uint32_t node_id;
    };
    
    static void compute_repulsion_forces(std::vector<NodePhysics>& physics_nodes, float repel_strength);
    static void compute_attraction_forces(std::vector<NodePhysics>& physics_nodes, 
                                         const Graph3D& graph, float attract_strength);
    static void apply_centering_force(std::vector<NodePhysics>& physics_nodes, float centering_strength);
    static void integrate_physics(std::vector<NodePhysics>& physics_nodes, float decay, float dimension);
    static Vector3 calculate_center_of_mass(const std::vector<NodePhysics>& physics_nodes);
};