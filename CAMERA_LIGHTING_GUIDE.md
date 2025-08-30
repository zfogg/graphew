# Enhanced Camera & Lighting Controls for Graphew

## Overview
Your graphew visualization now includes advanced camera controls, lighting system, and visual effects for a more immersive 3D experience.

## New Features

### 🎥 Camera Movement Controls
- **WASD Keys**: Move the camera in 3D space
  - `W/S` - Move forward/backward
  - `A/D` - Strafe left/right  
  - `Q/E` - Move up/down vertically
- **Arrow Keys**: Rotate camera view
- **Mouse Controls**:
  - **Left Drag** - Pan the view
  - **Right Drag** - Rotate camera around the scene
  - **Scroll Wheel** - Zoom in/out

### 🎬 Advanced Camera Features
- **Field of View (FOV)**: `Shift + Scroll` - Adjust perspective (20°-120°)
- **Camera Speed**: `Ctrl + Scroll` - Adjust movement speed (1-50 units/sec)
- **Camera Presets**:
  - Press `0-9` to load saved camera positions
  - Press `Ctrl + 0-9` to save current camera to a preset
- **Reset Camera**: Press `R` to return to default view
- **Auto-Rotation**: Press `Space` to toggle automatic rotation

### 💡 Lighting System
- **Ambient Light**: 
  - `I` - Increase ambient light intensity
  - `K` - Decrease ambient light intensity
- **Directional Light**:
  - `L` - Increase directional light intensity  
  - `J` - Decrease directional light intensity
- **Light Direction** (Numpad):
  - `Numpad 4/6` - Rotate light horizontally
  - `Numpad 8/2` - Rotate light vertically
- **Shadows**: `Shift + S` - Toggle shadow rendering (when available)

### 🎨 Visual Effects
- **Fog Effect**: Press `F` - Adds depth-based atmospheric fog
- **Grid Display**: Press `G` - Toggle coordinate grid
- **Axis Indicators**: Press `X` - Show/hide XYZ axis lines (Red=X, Green=Y, Blue=Z)
- **Help Overlay**: Press `H` - Display on-screen control guide
- **Info Overlay**: Press `O` - Toggle information display
- **Physics**: Press `P` - Toggle physics simulation

## Technical Improvements

### Rendering Enhancements
- **Depth-based shading**: Objects further away appear darker for better depth perception
- **Fog rendering**: Quadratic fog falloff for realistic atmospheric effects
- **Per-node lighting**: Each graph node receives proper 3D lighting calculations
- **Perspective-correct rendering**: Improved FOV calculations for more natural 3D projection

### Performance Features
- **Smooth camera movement**: Velocity-based camera with optional smoothing
- **Configurable render distance**: Fog start/end distances for performance tuning
- **Depth culling**: Objects are shaded based on distance from camera

### Default Settings
- Auto-rotation: OFF (more control by default)
- Field of View: 60°
- Camera Speed: 10 units/second
- Ambient Light: 30%
- Directional Light: 80%
- Fog: Disabled by default

## Usage Example
```bash
./bin/graphew -f your-data-file.json.z
```

When the visualization loads:
1. Use WASD to explore the 3D graph
2. Press `G` to see the grid for orientation
3. Press `X` to display axis indicators
4. Adjust lighting with I/K/L/J for optimal visibility
5. Save interesting viewpoints with Ctrl+[0-9]
6. Press `H` anytime to see the help overlay

## Tips for Best Experience
- Start with auto-rotation (Space) to get an overview
- Use camera presets to save interesting angles
- Adjust FOV (Shift+Scroll) for wide or narrow perspectives
- Enable fog (F) for large graphs to improve depth perception
- Fine-tune lighting based on your graph's color scheme

Enjoy exploring your data in 3D with full control over camera and lighting!
