# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Duel 6 Reloaded is a cross-platform open-source 2D arcade deathmatch game written in C++ that supports up to 15 players in 26 arenas. The game features local and online multiplayer with 12 different weapons, customizable player profiles, and a level editor.

## Build System

The project uses **CMake** as the build system with multiple renderer backends:

### Common Build Commands

```bash
# Create build directory
mkdir build
cd build

# Configure build (default: OpenGL ES 3)
cmake ..

# Configure with specific renderer
cmake .. -DD6R_RENDERER=gl1    # OpenGL 1
cmake .. -DD6R_RENDERER=gl4    # OpenGL 4
cmake .. -DD6R_RENDERER=es2    # OpenGL ES 2
cmake .. -DD6R_RENDERER=es3    # OpenGL ES 3 (default)
cmake .. -DD6R_RENDERER=headless  # Headless (for servers)

# Build main game
make -j8 duel6r

# Build fast unity build (faster compilation)
make -j8 duel6unity

# Build master server
make -j8 duel6r-masterserver

# Build all targets
make -j8
```

### macOS Specific Build

```bash
mkdir build && cd build
brew install cmake gcc sdl2 sdl2_image sdl2_mixer sdl2_ttf glew mesa enet curl opus lua

cmake .. -D CMAKE_C_COMPILER=gcc-11 -D CMAKE_CXX_COMPILER=g++-11 -D D6R_RENDERER=gl1
make -j8 duel6r

# Run on macOS (required for High DPI)
SDL_VIDEO_HIGHDPI_DISABLED=1 ./duel6r
```

## Architecture

### Core Components

- **Application**: Main application loop and initialization (`source/Application.cpp`)
- **Game**: Core game logic and state management (`source/Game.cpp`)  
- **Menu**: Main menu and UI handling (`source/Menu.cpp`)
- **World**: Game world representation and physics (`source/World.cpp`)
- **WorldRenderer**: Rendering of the game world (`source/WorldRenderer.cpp`)

### Key Subsystems

1. **Rendering System** (`source/renderer/`):
   - Multiple backends: OpenGL 1, OpenGL 4, OpenGL ES 2/3, Headless, Vulkan (WIP)
   - Base classes: `RendererBase`, `RendererTarget`, `RendererBuffer`
   - Renderer-specific implementations in subdirectories

2. **Networking** (`source/net/`):
   - Client-server architecture with `NetClient` and `NetHost`
   - Master server protocol for server discovery
   - Game proxies for handling network game state

3. **Weapons System** (`source/weapon/`):
   - Base classes: `WeaponBase`, `ShotBase`
   - Specific implementations in `impl/` directory (Pistol, Bazooka, Laser, etc.)

4. **Bonus System** (`source/bonus/`):
   - Various power-ups and bonuses (Invulnerability, FastReload, etc.)

5. **GUI System** (`source/gui/`):
   - Basic UI controls: Button, CheckBox, ListBox, Slider, etc.

6. **Scripting** (`source/script/`):
   - Lua scripting support for game customization
   - Level and person scripts

### Namespace Structure

All code is under the `Duel6` namespace.

## Key File Locations

- **Main entry point**: `source/Main.cpp`
- **Game resources**: `resources/` (textures, sounds, levels, shaders)
- **Level files**: `resources/levels/` (JSON format)
- **Player profiles**: `resources/profiles/`
- **Shaders**: `resources/shaders/` (by renderer type)
- **Configuration**: `resources/data/` (JSON configs)

## Development Workflow

### Testing Network Features

The repository includes network testing utilities in `TESTING.txt`:

```bash
# Test packet reordering (Linux)
sudo tc qdisc add dev lo root netem gap 10 delay 100ms reorder 100
sudo tc qdisc change dev lo root netem gap 50 delay 100ms reorder 100
sudo tc qdisc del dev lo root
```

### Command Line Arguments

Key runtime arguments for testing:

```bash
# Windowed mode
./duel6r -window 1920x1080

# Dedicated server mode
./duel6r -dedicated

# Game state integration (for external tools)
./duel6r -publish -url http://localhost/api/duel6r-gsi -port 3000
```

## Dependencies

- **SDL2** and extensions (SDL2_image, SDL2_mixer, SDL2_ttf)
- **OpenGL/OpenGL ES** (depending on renderer)
- **GLEW** (for OpenGL contexts)
- **ENet** (networking)
- **Lua** (scripting, optional)
- **cURL** (HTTP requests)
- **Opus** (voice chat)

## Unity Build System

The project supports a "unity build" system for faster compilation times. Unity source files are located in various `unity.cpp` files throughout the codebase and compile multiple related source files together as a single translation unit.

## Custom Features

- **Quake-style console**: Built-in developer console accessible in-game
- **Level editor**: External HTML5 level editor available
- **Voice chat**: Built-in voice communication support
- **Game state integration**: JSON API for external tools
- **Multiple render backends**: Supports various OpenGL versions and headless mode
- **Cross-platform**: Windows, Linux, macOS, and Raspberry Pi support