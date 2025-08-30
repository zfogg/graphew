#include "force_layout.hpp"
#include <cmath>
#include <iostream>
#include <random>

// Smooth fractional axis weight: 0 -> near-locked (epsilon), 1 -> fully enabled
static inline float axis_weight(float dimension, float axisIndex) { return 0.001f + 0.999f * std::min(1.0f, std::max(0.0f, dimension - axisIndex)); }

void ForceLayoutEngine::apply_force_layout(Graph3D& graph, const PhysicsParams& params) {
    if (graph.node_count == 0) return;
    
    std::cout << "Applying force layout with " << params.iterations << " iterations" << std::endl;
    
    // Initialize physics nodes with smart data-driven positions
    std::vector<NodePhysics> physics_nodes(graph.node_count);
    
    for (uint32_t i = 0; i < graph.node_count; i++) {
        physics_nodes[i].node_id = i;
        
        // Start with the existing node position as initial guess
        // This preserves any meaningful spatial relationships from data
        physics_nodes[i].position = graph.nodes[i].position;
        
        // If position is at origin, use smart initialization based on node properties
        if (physics_nodes[i].position.length() < 0.1f) {
            // Use node color as a hint for positioning
            float hue = static_cast<float>(graph.nodes[i].color.r + graph.nodes[i].color.g + graph.nodes[i].color.b) / 765.0f;
            float radius = 2.0f + hue * 3.0f;
            float angle = i * 0.618f * 2.0f * M_PI; // Golden angle
            
            physics_nodes[i].position = Vector3(
                radius * std::cos(angle),
                radius * std::sin(angle),
                (hue - 0.5f) * 4.0f  // Spread in Z based on color
            );
        }
        
        physics_nodes[i].velocity = Vector3(0, 0, 0);
        physics_nodes[i].force = Vector3(0, 0, 0);
    }
    
    // Run physics simulation
    for (int iteration = 0; iteration < params.iterations; iteration++) {
        // Reset forces
        for (auto& node : physics_nodes) {
            node.force = Vector3(0, 0, 0);
        }
        
        // Compute all forces with fractional dimensionality and ramp multiplier
        compute_repulsion_forces(physics_nodes, params.repel * params.force_multiplier, params.dimension);
        compute_attraction_forces(physics_nodes, graph, params.attract * params.force_multiplier, params.dimension);
        apply_centering_force(physics_nodes, params.centering_strength * params.force_multiplier, params.dimension);
        
        // Integrate physics
        integrate_physics(physics_nodes, params.decay, params.dimension);
        
        if (iteration % 10 == 0) {
            std::cout << "Physics iteration " << iteration << "/" << params.iterations << std::endl;
        }
    }
    
    // Copy final positions back to graph
    for (uint32_t i = 0; i < graph.node_count; i++) {
        graph.nodes[i].position = physics_nodes[i].position;
    }
    
    std::cout << "Force layout complete!" << std::endl;
}

bool ForceLayoutEngine::apply_force_layout_step(Graph3D& graph, const PhysicsParams& params, int& remaining_iterations) {
    if (graph.node_count == 0 || remaining_iterations <= 0) return false;
    
    static std::vector<NodePhysics> physics_nodes;
    static bool initialized = false;
    static uint32_t last_node_count = 0;
    
    // Initialize on first call OR when graph changes OR when nodes have been reset
    bool nodes_were_reset = false;
    if (initialized && physics_nodes.size() == graph.node_count) {
        // Check if node positions have been externally reset (indicating 'R' key press)
        for (uint32_t i = 0; i < std::min(3u, graph.node_count); i++) {
            float pos_diff = std::abs(physics_nodes[i].position.x - graph.nodes[i].position.x) +
                           std::abs(physics_nodes[i].position.y - graph.nodes[i].position.y) +
                           std::abs(physics_nodes[i].position.z - graph.nodes[i].position.z);
            if (pos_diff > 5.0f) { // Large position change indicates reset
                nodes_were_reset = true;
                break;
            }
        }
    }
    
    if (!initialized || physics_nodes.size() != graph.node_count || nodes_were_reset) {
        physics_nodes.resize(graph.node_count);
        
        for (uint32_t i = 0; i < graph.node_count; i++) {
            physics_nodes[i].node_id = i;
            physics_nodes[i].position = graph.nodes[i].position;
            physics_nodes[i].velocity = Vector3(0, 0, 0);
            physics_nodes[i].force = Vector3(0, 0, 0);
        }
        initialized = true;
    }
    
    // Apply physics for this frame (multiple iterations for faster convergence)
    int iterations_per_frame = std::min(5, remaining_iterations);
    
    for (int iter = 0; iter < iterations_per_frame; iter++) {
        // Reset forces
        for (auto& node : physics_nodes) {
            node.force = Vector3(0, 0, 0);
        }
        
        // Compute all forces with fractional dimensionality and ramp multiplier
        compute_repulsion_forces(physics_nodes, params.repel * params.force_multiplier, params.dimension);
        compute_attraction_forces(physics_nodes, graph, params.attract * params.force_multiplier, params.dimension);
        apply_centering_force(physics_nodes, params.centering_strength * params.force_multiplier, params.dimension);
        
        // Integrate physics
        integrate_physics(physics_nodes, params.decay, params.dimension);
        
        remaining_iterations--;
        if (remaining_iterations <= 0) break;
    }
    
    // Copy positions back to graph
    for (uint32_t i = 0; i < graph.node_count; i++) {
        graph.nodes[i].position = physics_nodes[i].position;
    }
    
    return remaining_iterations > 0;
}

void ForceLayoutEngine::compute_repulsion_forces(std::vector<NodePhysics>& physics_nodes, float repel_strength, float dimension) {
    const float min_distance = 0.1f;
    
    for (size_t i = 0; i < physics_nodes.size(); i++) {
        for (size_t j = i + 1; j < physics_nodes.size(); j++) {
            Vector3 diff = physics_nodes[i].position - physics_nodes[j].position;
            diff.y *= axis_weight(dimension, 1.0f);
            diff.z *= axis_weight(dimension, 2.0f);
            float distance = diff.length();
            
            if (distance < min_distance) distance = min_distance;
            
            Vector3 normalized = diff.normalize();
            float force_magnitude = repel_strength / (distance * distance + 0.01f);
            Vector3 force = normalized * force_magnitude;
            
            physics_nodes[i].force = physics_nodes[i].force + force;
            physics_nodes[j].force = physics_nodes[j].force - force;
        }
    }
}

void ForceLayoutEngine::compute_attraction_forces(std::vector<NodePhysics>& physics_nodes, 
                                                  const Graph3D& graph, float attract_strength, float dimension) {
    const float max_distance = 50.0f;
    
    for (uint32_t e = 0; e < graph.edge_count; e++) {
        const GraphEdge& edge = graph.edges[e];
        if (!edge.visible) continue;
        
        uint32_t from_id = edge.from_id;
        uint32_t to_id = edge.to_id;
        
        if (from_id >= physics_nodes.size() || to_id >= physics_nodes.size()) continue;
        
        Vector3 diff = physics_nodes[to_id].position - physics_nodes[from_id].position;
        diff.y *= axis_weight(dimension, 1.0f);
        diff.z *= axis_weight(dimension, 2.0f);
        float distance = diff.length();
        
        if (distance > max_distance) distance = max_distance;
        if (distance < 0.1f) continue;
        
        Vector3 normalized = diff.normalize();
        float force_magnitude = attract_strength * distance;
        Vector3 force = normalized * force_magnitude;
        
        physics_nodes[from_id].force = physics_nodes[from_id].force + force;
        physics_nodes[to_id].force = physics_nodes[to_id].force - force;
    }
}

void ForceLayoutEngine::apply_centering_force(std::vector<NodePhysics>& physics_nodes, float centering_strength, float dimension) {
    // Center around a fixed point instead of center of mass - prevents drift
    Vector3 target_center(0, 0, 0); // Force layout centers around origin
    
    for (auto& node : physics_nodes) {
        Vector3 to_center = target_center - node.position;
        to_center.y *= axis_weight(dimension, 1.0f);
        to_center.z *= axis_weight(dimension, 2.0f);
        node.force = node.force + (to_center * centering_strength);
    }
}

void ForceLayoutEngine::integrate_physics(std::vector<NodePhysics>& physics_nodes, float decay, float dimension) {
    const float dt = 0.1f; // Larger timestep for faster convergence
    const float max_velocity = 50.0f; // Allow much faster movement
    const float max_position = 100.0f; // Larger bounds
    
    for (auto& node : physics_nodes) {
        // Update velocity with smaller timestep
        node.velocity = node.velocity + (node.force * dt);
        
        // Clamp velocity to prevent explosion
        float vel_magnitude = node.velocity.length();
        if (vel_magnitude > max_velocity) {
            node.velocity = node.velocity * (max_velocity / vel_magnitude);
        }
        
        // Apply decay (damping)
        node.velocity = node.velocity * decay;
        
        // Apply fractional-dimension axis damping and small jitter if near-plane
        float wy = axis_weight(dimension, 1.0f);
        float wz = axis_weight(dimension, 2.0f);
        node.velocity.y *= wy;
        node.velocity.z *= wz;
        if (wy > 0.05f && std::abs(node.position.y) < 1e-3f && std::abs(node.velocity.y) < 1e-4f) {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            node.velocity.y += dist(rng) * 0.02f * wy;
        }
        if (wz > 0.05f && std::abs(node.position.z) < 1e-3f && std::abs(node.velocity.z) < 1e-4f) {
            static thread_local std::mt19937 rng2(std::random_device{}());
            std::uniform_real_distribution<float> dist2(-1.0f, 1.0f);
            node.velocity.z += dist2(rng2) * 0.02f * wz;
        }
        
        // Update position
        node.position = node.position + (node.velocity * dt);
        
        // Clamp positions to prevent runaway
        node.position.x = std::max(-max_position, std::min(node.position.x, max_position));
        node.position.y = std::max(-max_position, std::min(node.position.y, max_position));
        node.position.z = std::max(-max_position, std::min(node.position.z, max_position));
    }
}

Vector3 ForceLayoutEngine::calculate_center_of_mass(const std::vector<NodePhysics>& physics_nodes) {
    Vector3 sum(0, 0, 0);
    
    for (const auto& node : physics_nodes) {
        sum = sum + node.position;
    }
    
    return sum * (1.0f / physics_nodes.size());
}