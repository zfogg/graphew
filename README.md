# Graphew - 3D AI Agent State Space Visualizer

A performant C++ application for visualizing AI agent behavior from [Metta AI](https://github.com/Metta-AI/metta) replay data using force-directed graph layouts inspired by Klotski puzzle state spaces.

## Overview

Graphew transforms AI agent replay data into interactive 3D visualizations that reveal the structure of agent behavior patterns. Inspired by [swaptube's Klotski puzzle analysis](https://www.youtube.com/@twoswap), it uses force-directed graph layouts to show how agents navigate through inventory strategy spaces over time.

### Key Features

- **Force-Directed Layouts**: Physics-based graph positioning
- **Temporal State Transitions**: Edges represent actual agent inventory changes over time
- **Multi-Agent Support**: Visualize behavior patterns across multiple AI agents simultaneously  
- **Reward Visualization**: Color-coded nodes show successful vs unsuccessful strategies
- **Real-Time 3D Graphics**: SFML-powered interactive rendering with smooth camera controls
- **Universal Format Support**: Handles both `.grid_objects` and `.objects` replay formats
- **Compressed File Support**: Native zlib decompression for `.json.z` replay files

## Background: Klotski State Spaces

The Klotski puzzle demonstrates how complex state spaces emerge from simple rules:

> "The Klotski puzzle defines graphs modeling convoluted topologies with their local substructure as well as their global superstructure, and its state space can be represented as a node, with each move connecting to a different node"

Each puzzle configuration is a **node**, with **edges** representing valid moves between configurations. This creates intricate 3D graph structures - some puzzles generate over 25,000 interconnected states forming maze-like topologies with:

- **Branching pathways** through the solution space
- **Clustering** of similar configurations  
- **Isolated islands** of disconnected states
- **Dimensional complexity** that varies by puzzle structure

Graphew applies this same approach to **AI agent behavior analysis**, where:

- **Nodes** = Unique agent inventory combinations (`ore_red: 5, hearts: 3, batteries: 1`)
- **Edges** = Temporal transitions when agents change inventory states
- **Topology** = Natural clustering reveals successful vs unsuccessful strategies

## Installation

### Prerequisites

**macOS:**
```bash
brew install sfml cjson zlib glm pkg-config
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libsfml-dev libcjson-dev zlib1g-dev libglm-dev pkg-config
```

**Fedora:**
```bash
sudo dnf install SFML-devel libcjson-devel zlib-devel glm-devel pkgconfig
```

### Building

```bash
git clone <repository-url>
cd graphew
make install-deps  # Platform-specific dependency installation
make all           # Build the project
```

### Build Troubleshooting

```bash
make check-deps    # Verify all dependencies
make debug         # Show build configuration
./build.sh         # Alternative build script with better error messages
```

## Usage

### Basic Usage

```bash
# Visualize Metta AI agent replay data
./bin/graphew -f replay.json

# Support for compressed replays (recommended)
./bin/graphew -f replay.json.z  

# Help and version info
./bin/graphew --help
./bin/graphew --version
```

### Command Line Options

- `-f, --file FILE`: Load replay from JSON file (supports .json.z compression)
- `-h, --help`: Show help message with usage examples
- `-v, --version`: Display version information

### Interactive Controls

- **Mouse Drag**: Rotate camera around graph
- **Mouse Wheel**: Zoom in/out
- **SPACE**: Toggle auto-rotation on/off
- **R**: Reset camera to default position  
- **P**: Toggle physics simulation
- **O**: Toggle info overlay
- **ESC**: Exit application

## Data Format Support

Graphew supports multiple Metta AI replay formats:

### Format 1: grid_objects (Legacy)
```json
{
  "inventory_items": ["ore_red", "battery_red", "heart"],
  "grid_objects": [
    {
      "type": 0,
      "agent_id": 1,
      "inv:ore_red": [[0, 0], [100, 5], [200, 3]],
      "reward": [[100, 1.0], [200, 0.0]],
      "total_reward": [[0, 0.0], [100, 1.0]]
    }
  ]
}
```

### Format 2: objects (Current)
```json
{
  "item_names": ["ore_red", "battery_red", "heart"],
  "objects": [
    {
      "type_id": 0,
      "agent_id": 1,
      "inventory": [
        [0, []],
        [100, [[0, 5], [2, 1]]],
        [200, [[0, 3], [2, 2]]]
      ],
      "current_reward": [[100, 1.0]],
      "total_reward": [[100, 1.0], [200, 1.0]]
    }
  ]
}
```

## Visualization Features

### Node Properties
- **Position**: Determined by force-directed physics simulation
- **Color**: Reward level (Red → Yellow → Green for increasing success)
- **Size**: Proportional to total accumulated reward
- **Label**: State description with reward information

### Edge Properties  
- **Green Edges**: Agent gained reward during transition (successful actions)
- **Red Edges**: Agent lost reward during transition (unsuccessful actions)
- **Gray Edges**: No reward change (neutral transitions)
- **Thickness**: Frequency of that specific transition across all agents

### Graph Topology

The force-directed layout reveals natural clustering:

- **Successful strategies** (high-reward states) form connected clusters
- **Transition pathways** show how agents reach successful states
- **Isolated nodes** represent unique or rare inventory combinations
- **Dense regions** indicate common agent behavior patterns

## Technical Architecture

### Core Components

- **C++17**: Modern C++ with performance optimizations for large datasets
- **SFML 3.0**: Cross-platform graphics and window management
- **Force Layout Engine**: Physics simulation based on swaptube's approach
- **Dual Format Parser**: Universal support for Metta AI replay formats
- **Swaptube Integration**: Pixel manipulation utilities for overlays

### Dependencies

| Library | Purpose | Required |
|---------|---------|----------|
| SFML | 3D graphics and window management | Yes |
| cJSON | JSON parsing for replay files | Yes |
| zlib | Compressed file decompression | Yes |
| GLM | Mathematical operations (swaptube integration) | Yes |
| pkg-config | Build system dependency management | Yes |

### Performance

- **120 FPS target** for ProMotion displays
- **Handles 500+ nodes** with real-time physics
- **1000+ edges** with smooth rendering
- **200 physics iterations** for proper graph organization

## Integration with Metta AI

Graphew is designed specifically for Metta AI's multi-agent reinforcement learning research:

### Research Applications

1. **Strategy Analysis**: Identify which inventory management strategies lead to high rewards
2. **Agent Comparison**: Compare behavior patterns across different trained policies
3. **Temporal Dynamics**: Understand how agent strategies evolve during episodes
4. **Emergent Behavior**: Discover unexpected clustering patterns in agent state spaces

### Workflow Integration

```bash
# 1. Train agents in Metta
./tools/run.py experiments.recipes.arena.train --args run=my_experiment

# 2. Generate replay data
./tools/run.py experiments.recipes.arena.replay --overrides policy_uri=wandb://run/my_experiment

# 3. Visualize with Graphew
./bin/graphew -f replay.json.z
```

### Data Insights

The visualization reveals:

- **Cooperative agents** tend to cluster in high-reward regions
- **Competitive agents** show more scattered, exploratory behavior  
- **Successful policies** create clear pathways through inventory space
- **Failed strategies** result in isolated nodes or dead-end clusters

## Development

### Project Structure

```
graphew/
├── lib/           # Core libraries
│   ├── graph.hpp/cpp         # Graph data structures
│   ├── renderer.hpp/cpp      # SFML-based 3D rendering
│   ├── replay_parser.hpp/cpp # Multi-format replay parsing
│   ├── force_layout.hpp/cpp  # Physics simulation engine
│   └── swaptube_pixels.hpp   # Pixel manipulation utilities
├── src/
│   └── graphew.cpp           # Main application
├── vendor/
│   └── swaptube/            # Swaptube integration for graphics algorithms
├── Makefile                  # Cross-platform build system
└── build.sh                 # Build script with error handling
```

### Adding New Features

1. **New Replay Formats**: Extend `ReplayParser::parse_json_data()`
2. **Visualization Modes**: Add methods to `AgentGraphBuilder`  
3. **Physics Parameters**: Modify `ForceLayoutEngine::PhysicsParams`
4. **Rendering Effects**: Enhance `GraphRenderer` with new visual features

## Troubleshooting

### Common Issues

**Blank Screen**: File loaded but no data rendered
- Solution: Check that replay file contains agents with inventory changes

**Missing Edges**: Nodes visible but no connections  
- Solution: Verify agents actually changed inventory states during replay

**Build Errors**: Missing dependencies
- Solution: Run `make check-deps` and install missing packages

**Compressed Files Not Working**: zlib issues
- Solution: Install zlib development headers (`zlib1g-dev`, `zlib-devel`)

### Debug Commands

```bash
make debug         # Show build configuration
make check-deps    # Verify all dependencies  
./bin/graphew --version  # Check if binary works
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make changes following the existing code style
4. Test with sample replay files
5. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Acknowledgments

- **Swaptube**: Force-directed graph algorithms and visualization techniques
- **Metta AI**: Multi-agent RL framework and replay data format
- **SFML**: Cross-platform graphics library
- **Klotski Puzzle Research**: Inspiration for state space analysis methods