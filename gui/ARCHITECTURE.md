# Zappy Godot Architecture

## Overview
This document describes the cleaned up architecture of the Zappy Godot client after refactoring.

## Server Connection Architecture

### Main Entry Point
**Main.gd** - The main scene script with export variables for easy configuration:
- `use_mock_server: bool = true` - Toggle between mock and real server
- `server_ip: String = "127.0.0.1"` - Real server IP address
- `server_port: int = 8674` - Real server port

**Note**: The GUI connects as an **observer**, not as a player with a team. This allows it to receive game state updates and observe all teams without participating in the game.

### Core Components

#### 1. IntegrationManager.gd
**Purpose**: Central hub that manages switching between mock and real server modes
**Responsibilities**:
- Manages MockServer and ServerConnectionManager instances
- Routes commands to the appropriate processor based on current mode
- Provides unified API for sending commands regardless of mode

#### 2. ServerConnectionManager.gd
**Purpose**: Handles all real server communication via WebSockets as an observer using Godot's native WebSocketPeer
**Responsibilities**:
- WebSocket connection management with native Godot API
- Observer authentication with proper JSON login flow
- Receives game state updates from server
- Response parsing and signal emission
- Connection state management and error handling

**Note**: This connects as an observer role, not as a player, allowing the GUI to watch the game without interfering.

#### 3. CommandProcessor.gd (Mock Mode)
**Purpose**: Simulates server behavior for testing and development
**Responsibilities**:
- Processes commands locally against GameData
- Simulates game logic for movement, item collection, etc.
- Provides immediate feedback for testing

#### 4. MockServer.gd
**Purpose**: Automated command sending from pre-recorded JSON files
**Responsibilities**:
- Loads command files from res://data/commands/
- Sends commands at timed intervals
- Simulates realistic game scenarios

## Usage

### Switching Between Modes

#### Option 1: Inspector (Recommended)
1. Open Main scene in Godot
2. Select the Main node
3. In Inspector, toggle "Use Mock Server" checkbox
4. Set server connection details if using real server
5. Run the scene

#### Option 2: Code
```gdscript
# In Main.gd _ready() function
integration_manager.use_mock_server()
# OR
integration_manager.connect_to_real_server("127.0.0.1", 8674)
```

### Sending Commands
```gdscript
# Commands are only available in Mock mode for testing
# Real server mode is Observer-only
integration_manager.send_avance(player_id)    # Mock only
integration_manager.send_voir(player_id)      # Mock only
integration_manager.send_prend(player_id, "linemate")  # Mock only
```

### Handling Game State Updates
```gdscript
# Connect to IntegrationManager signals to receive game updates
integration_manager.connect("server_message_received", _on_server_message_received)

func _on_server_message_received(message_type: String, data: Dictionary):
    match message_type:
        "game_state":
            GameData.update_game_state(data)  # Update local game state
        "status":
            print("Server status: ", data.message)
```

## Signal Flow

```
Game State Updates from Server
     ↓
ServerConnectionManager (Observer)
     ↓
IntegrationManager
     ↓
GameData Update → Visual Update

[Mock Mode Only]
Command Request → CommandProcessor → GameData → Visual Update
```

## Removed Components

The following legacy files were removed during cleanup:
- `NetworkManager.gd` - Old TCP-based connection (replaced by WebSocket)
- `WebSocketManager.gd` - Redundant with ServerConnectionManager
- `WebSocketDemo.gd` - Test file no longer needed
- `RealServerCommandProcessor.gd` - Renamed to ServerConnectionManager
- `ServerSelectionUI.gd` - Replaced by inspector variables
- `WebSocketTest.gd` - Test file no longer needed

## Dependencies

### External
- **None**: Uses Godot's native WebSocketPeer for all WebSocket communication

### Internal Singletons
- **GameData**: Global game state management
- **ServerConfig**: Configuration storage

## Development Workflow

### Mock Server Mode (`use_mock_server = true`)
1. **Load Test Data**: Loads `res://data/initial_data/game_3x3.json`
2. **Initialize MockServer**: Starts sending commands from `res://data/commands/`
3. **Local Simulation**: Commands processed locally against GameData
4. **Perfect for**: Feature development, testing, and iteration

### Real Server Mode (`use_mock_server = false`)
1. **Connect as Observer**: Connects to real Zappy server
2. **Authenticate**: Sends observer login credentials
3. **Wait for Game State**: Receives initial map and game data from server
4. **Build World**: Constructs game world from server data
5. **Receive Updates**: Continuously receives game state updates
6. **Perfect for**: Integration testing and final deployment

## Flow Comparison

### Mock Mode Flow
```
Start → Load Test Data → Initialize MockServer → Send Commands → Update GameData → Visual Updates
```

### Real Server Flow  
```
Start → Connect to Server → Authenticate → Receive Game State → Build World → Receive Updates → Visual Updates
```

## Configuration Files

### Mock Commands
Located in: `res://data/commands/`
- JSON files containing command sequences
- Automatically loaded by MockServer
- Useful for testing specific scenarios

### Server Settings
- Set via inspector variables in Main scene
- Can be overridden in code if needed
- Support for IP, port, and team name configuration
