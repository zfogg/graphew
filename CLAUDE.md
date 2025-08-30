# Graphew Development Guide

This document provides context for AI assistants (like Claude) working on the Graphew project - a force-directed graph visualizer for Metta AI agent behavior analysis.

## Project Context

### What is Graphew?

Graphew is a C++ application that transforms Metta AI agent replay data into interactive 3D visualizations using force-directed graph layouts. It was inspired by a YouTube video about Klotski puzzle state spaces and implements similar graph topology analysis for AI agent behavior.

### The Klotski Connection

The project is based on insights from [swaptube's](https://www.youtube.com/@twoswap) analysis of Klotski puzzle state spaces:

**Key Insights from the YouTube Video:**
- **State Space as Graph**: Each puzzle configuration = node, each valid move = edge
- **Complex Topologies**: 25,955 interconnected states forming maze-like structures
- **Dimensional Complexity**: Number of blocks corresponds to graph dimensionality
- **Natural Clustering**: Similar configurations cluster together, isolated islands form
- **Solution Pathways**: Dense pits can be avoided by walking fine lines to solutions

**Application to AI Agents:**
Instead of puzzle pieces, we analyze **agent inventory states**:
- **Nodes**: Unique inventory combinations (ore_red: 5, hearts: 3, batteries: 1)
- **Edges**: Temporal transitions when agents change inventory states
- **Topology**: Natural clustering reveals successful vs unsuccessful strategies

### Metta AI Integration

Graphew visualizes data from [Metta AI](https://github.com/Metta-AI/metta) - a multi-agent RL research platform:

**Metta Environment:**
- **2D Gridworld**: Agents move, collect resources, interact
- **Inventory Management**: Ore (red/blue/green), batteries, hearts, armor, lasers, blueprints
- **Reward Structure**: Agents gain/lose rewards for actions
- **Social Dynamics**: Cooperation, competition, kinship structures

**Research Questions:**
- How do inventory strategies cluster?
- Which pathways lead to high rewards?
- How do agent behaviors evolve over time?
- What emergent cooperation patterns arise?

## Technical Architecture

### Core Technologies

**Graphics & Windowing:**
- **SFML 3.0**: Cross-platform graphics, chosen over raylib
- **Custom orthographic projection**: Handles centering and coordinate transformation
- **Interactive camera system**: Pan, zoom, rotate with proper 3D perspective

**Physics Simulation:**
- **Force-directed layout**: Swaptube does irt
- **Customizable parameters**: Repulsion, attraction, decay, centering forces
- **Gradual force ramp-up**: Cinematic ease-in curve for natural animation
- **Real-time tuning**: Interactive sliders for parameter adjustment

**Data Processing:**
- **Multi-format parser**: Supports 3 Metta AI replay formats
- **Temporal analysis**: Converts agent actions into state transition graphs
- **Compression support**: Native zlib decompression for .json.z files

### Supported Replay Formats

1. **Grid_objects format** (sample.json):
   ```json
   {
     "inventory_items": ["ore_red", "battery_red", "heart"],
     "grid_objects": [{"type": 0, "inv:ore_red": [[0,0], [100,5]]}]
   }
   ```

2. **Objects format** (sample2.json, sample3.json):
   ```json
   {
     "item_names": ["ore_red", "battery_red", "heart"],
     "objects": [{"type_id": 0, "inventory": [[0,[]], [100,[[0,5]]]]}]
   }
   ```

3. **Pufferbox format** (andre_pufferbox_33.json):
   ```json
   {
     "inventory_items": ["ore.red", "battery", "heart"],
     "grid_objects": [{"type": 0, "agent:inv:ore.red": [[0,0], [100,5]]}]
   }
   ```

## Development Guidelines

### Code Organization

**Library Structure:**
- `graph.hpp/cpp`: Core data structures (Vector3, Color, Graph3D)
- `renderer.hpp/cpp`: SFML-based 3D rendering and camera system
- `replay_parser.hpp/cpp`: Multi-format JSON parsing and agent data extraction
- `force_layout.hpp/cpp`: Physics simulation engine with swaptube-inspired algorithms

**Key Design Principles:**
- **Data-driven**: All visualization parameters calculated from actual replay data
- **Format-agnostic**: Universal parsing supports all Metta AI replay formats
- **Real-time responsive**: Interactive parameter tuning via sliders and controls
- **Performance-focused**: 120 FPS target with efficient force calculations

### Common Development Tasks

**Adding New Replay Formats:**
1. Extend `ReplayParser::parse_json_data()` with format detection
2. Add format-specific parsing logic for inventory and reward data
3. Test with sample files to ensure proper data extraction

**Modifying Visualization:**
1. **Node properties**: Adjust color/size calculations in `AgentGraphBuilder`
2. **Force parameters**: Modify `ForceLayoutEngine::PhysicsParams`
3. **Camera behavior**: Update projection and control logic in `GraphRenderer`

**Debugging Data Issues:**
1. **Check console output**: Parser provides detailed format detection and data extraction logs
2. **Add debug prints**: Instrument critical data paths (inventory parsing, reward extraction)
3. **Validate with jq**: Use command-line JSON tools to verify source data structure

### Build System

**Incremental Building:**
- Uses dependency tracking with `-MMD -MP` flags
- Only rebuilds changed files and their dependents
- Faster iteration during development

**Cross-platform Support:**
- macOS (Homebrew), Ubuntu/Debian (apt), Fedora (dnf)
- pkg-config integration for dependency management
- Platform-specific optimization flags

### Physics System Deep Dive

**Swaptube-Inspired Force Layout:**
The force simulation follows swaptube's proven approach:
- **Repulsion forces**: Push unrelated nodes apart
- **Attraction forces**: Pull connected nodes together (temporal transitions)
- **Centering forces**: Prevent overall drift, keep graph centered
- **Fractional dimensionality**: Smooth transitions between 1D/2D/3D layouts

**Force Ramp-Up System:**
```cpp
// Custom piecewise ease-in curve
if (t < 0.7f) {
    // First 70%: gentle cubic curve to 0.3
    force_multiplier = 0.3f * (t³);
} else {
    // Last 30%: steep quadratic acceleration to 1.0
    force_multiplier = 0.3f + 0.7f * ((t-0.7)/0.3)²;
}
```

**Real-time Parameter Tuning:**
Interactive sliders allow researchers to:
- Adjust repulsion/attraction balance
- Control decay (damping) forces
- Modify centering strength
- Change dimensional constraints
- Tune rendering parameters

## Research Applications

### Agent Behavior Analysis

**Inventory Strategy Clustering:**
- **High-reward clusters**: Groups of successful inventory combinations
- **Transition pathways**: Sequences of actions leading to success
- **Isolated strategies**: Unique or unsuccessful approaches
- **Community structure**: Natural organization of similar behaviors

**Temporal Dynamics:**
- **Learning progression**: How agent strategies evolve over episodes
- **Convergence patterns**: Whether agents develop similar successful strategies
- **Exploration vs exploitation**: Scattered vs clustered behavior patterns

### Visualization Insights

**Color Coding:**
- **Green nodes**: High total reward (successful agents)
- **Yellow nodes**: Medium reward (partially successful)
- **Red nodes**: Low/no reward (unsuccessful agents)
- **Edge colors**: Green (reward gain), red (reward loss), gray (neutral)

**Graph Topology:**
- **Dense clusters**: Common behavior patterns
- **Sparse regions**: Rare or transitional states
- **Bridge connections**: Critical transitions between strategy types
- **Isolated islands**: Dead-end or unique strategies

## Development Workflow

### Setting Up Development Environment

1. **Clone and build:**
   ```bash
   git clone <repository-url>
   cd graphew
   git submodule update --init --recursive  # Initialize swaptube
   make install-deps
   make
   ```

2. **Development cycle:**
   ```bash
   # Edit code
   make            # Incremental build
   ./bin/graphew -f sample.json  # Test
   ```

### Working with Metta AI Data

**Generating Test Data:**
```bash
# In Metta AI repository
./tools/run.py experiments.recipes.arena.train --args run=test_run
./tools/run.py experiments.recipes.arena.replay --overrides policy_uri=wandb://run/test_run
# Copy replay.json.z to graphew directory
```

**Data Validation:**
```bash
# Check replay structure
jq 'keys' replay.json
jq '.grid_objects | length' replay.json
jq '.grid_objects | map(.type) | unique' replay.json

# Verify agent inventory data
jq '.grid_objects | map(select(.type == 0)) | .[0] | keys | map(select(contains("inv")))' replay.json
```

### Debugging Common Issues

**Centering Problems:**
The most complex part of development has been ensuring graphs center properly. Key insights:
- **SFML view transformations** can interfere with custom projections
- **Force layout centering** must sync with camera target
- **Different data formats** require different centering approaches

**Performance Optimization:**
- **Force calculations**: Use efficient spatial data structures for large graphs
- **Rendering**: Depth-sort nodes for proper 3D appearance
- **Memory management**: Use efficient Vector3 operations and avoid unnecessary allocations

**Format Support:**
When adding new replay formats:
1. **Check inventory_items field** for reliable format detection
2. **Handle field name variations** (inv: vs agent:inv: vs inventory arrays)
3. **Test with actual data files** to ensure parsing robustness

## Integration Notes

### Swaptube Integration

**What we use from swaptube:**
- **Force layout algorithms**: Physics simulation approach
- **Pixels class**: 2D overlay graphics for UI elements
- **Graph concepts**: State space analysis methodology

**What we don't use:**
- **FFMPEG pipeline**: We use real-time SFML instead
- **Complex rendering**: We simplified for interactive use
- **Heavy dependencies**: Extracted only essential components

### SFML 3.0 Specifics

**Key differences from SFML 2.x:**
- **Event handling**: New `getIf<EventType>()` API
- **Texture creation**: `resize()` method required before `update()`
- **View system**: More strict about coordinate transformations

**Custom projection system:**
We bypass SFML's view system for precise control:
```cpp
// Force default view to prevent coordinate transformation
window.setView(window.getDefaultView());

// Custom orthographic projection
float screen_x = window_center_x + (world_x - camera_target_x) * scale;
float screen_y = window_center_y - (world_y - camera_target_y) * scale; // Flip Y
```

## Best Practices

### When Extending Functionality

1. **Maintain format universality**: New features should work with all replay formats
2. **Add debug output**: Log key calculations for troubleshooting
3. **Test incrementally**: Use sample files to verify each change
4. **Follow swaptube patterns**: Use established force layout approaches

### Performance Considerations

1. **Avoid full rebuilds**: Use `make` instead of `make clean && make`
2. **Profile force calculations**: Large graphs (500+ nodes) can become slow
3. **Optimize projection**: Minimize calculations in `world_to_screen_3d()`
4. **Cache UI elements**: Don't recreate textures every frame

### Research Workflow

1. **Generate diverse replay data** from different Metta AI training runs
2. **Compare visualization patterns** across different agent policies
3. **Use interactive controls** to explore clustering and transition patterns
4. **Document findings** about agent behavior emergence

This visualization tool provides unique insights into multi-agent RL research by revealing the natural topology of agent behavior spaces - something that traditional metrics and logs cannot capture as intuitively.

## Future Extensions

**Potential Research Directions:**
- **Temporal heatmaps**: Show how strategy popularity changes over time
- **Policy comparison**: Side-by-side visualization of different training runs
- **Interactive filtering**: Focus on specific agent types or time periods
- **Statistical overlays**: Add quantitative metrics to visual patterns
- **3D trajectory tracking**: Show individual agent paths through strategy space

The force-directed approach scales naturally to these extensions while maintaining the core insight that **agent behavior topology reveals emergent patterns** that guide RL research directions.