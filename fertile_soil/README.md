# Fertile Soil - Graph & State Space Generation

This directory contains all the files from the swaptube project related to generating and visualizing graphs for state spaces.

## Directory Structure

### `/core`
Core graph infrastructure:
- `Graph.cpp` - Main graph data structure and algorithms (BFS/DFS traversal, expansion)
- `DataObject.cpp` - Base class for all state space objects

### `/boards`
State space representations for various puzzles and games:
- `KlotskiBoard.*` - Klotski sliding block puzzle states
- `GenericBoard.cpp` - Generic board interface
- `HashableString.cpp` - String-based state representation
- `LambdaCalculus.cpp` - Lambda calculus expression states
- `PacmanPackage.cpp` - Pacman game states
- `/Connect4/` - Complete Connect4 game implementation with steady-state analysis
- `/Rubiks/` - Rubik's cube puzzle solver and state space

### `/scenes`
Visualization components for rendering graphs:
- `GraphScene.cpp` - General graph visualization
- `Connect4GraphScene.cpp` - Connect4-specific graph rendering
- `KlotskiScene.cpp` - Klotski puzzle visualization
- `LambdaGraphScene.cpp` - Lambda calculus expression tree visualization

### `/projects`
Complete examples and demonstrations:
- `BuildGraph.cpp` - Graph construction utilities
- `Klotski.cpp` - Full Klotski puzzle implementation with graph generation
- `KlotskiShort.cpp` - Simplified Klotski demo
- `C4SteadyStateGraphDemo.cpp` - Connect4 steady-state analysis demo

### `/support`
Supporting utilities:
- `StateManager.cpp` - State management and interpolation
- `GraphSpread.cu` - CUDA GPU acceleration for graph layout algorithms
- `Scene.cpp` - Base scene class for visualizations
- `ThreeDimensionScene.cpp` - 3D rendering support

## Key Concepts

These files implement a general framework for:
1. **State Space Exploration**: Representing game/puzzle states as nodes in a graph
2. **Graph Traversal**: BFS/DFS algorithms to explore all reachable states
3. **Visualization**: Rendering state space graphs in 2D/3D
4. **GPU Acceleration**: Using CUDA for physics-based graph layout

The system can generate and visualize the complete state spaces for puzzles like Klotski, Connect4, Rubik's cube, and even abstract systems like lambda calculus expressions.

