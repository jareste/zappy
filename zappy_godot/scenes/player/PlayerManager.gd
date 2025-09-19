# PlayerManager.gd - Manages the visual representation of players
extends Node3D

@onready var player_root: Node3D
@onready var world_manager: Node3D

var players = {}

# Configuration constants
const PLAYER_HEIGHT = 0.3
const MOVEMENT_TWEEN_DURATION = 0.3

# Team colors configuration
var team_colors = {
	"Alpha": Color.RED,
	"Beta": Color.BLUE,
	"Gamma": Color.GREEN,
	"Delta": Color.YELLOW
}

func _ready():
	# Connect to GameData signals
	GameData.connect("game_state_updated", _on_game_state_updated)
	GameData.connect("player_updated", _on_player_updated)

func initialize(player_root_node: Node3D, world_manager_ref: Node3D):
	"""Initialize the player manager with required node references"""
	player_root = player_root_node
	world_manager = world_manager_ref

func _on_game_state_updated():
	"""Update all players when game state changes"""
	_update_all_players()

func _on_player_updated(player_id: int):
	"""Update a specific player when their data changes"""
	_update_player_visual(player_id)

func _update_all_players():
	"""Update all player visuals"""	
	if not player_root:
		return
		
	# Clear existing players
	for child in player_root.get_children():
		child.queue_free()
	players.clear()
	
	# Create new player visuals
	for player_id in GameData.players:
		_create_player_visual(player_id)

func _create_player_visual(player_id: int):
	"""Create visual representation for a player"""
	var player_data = GameData.get_player_data(player_id)
	if not player_data or not player_root or not world_manager:
		return
	
	var player_scene = preload("res://scenes/player/Player.tscn").instantiate()
	var pos = player_data.position
	var world_pos = world_manager.get_world_position(pos.x, pos.y)
	world_pos.y = PLAYER_HEIGHT
	
	player_scene.position = world_pos
	player_root.add_child(player_scene)
	players[player_id] = player_scene
	
	# Set initial player appearance
	_update_player_visual(player_id)

func _update_player_visual(player_id: int):
	"""Update a specific player's visual representation"""
	if not players.has(player_id):
		_create_player_visual(player_id)
		return
	
	var player_data = GameData.get_player_data(player_id)
	if not player_data or not world_manager:
		return
	
	var player_scene = players[player_id]
	var pos = player_data.position
	var target_pos = world_manager.get_world_position(pos.x, pos.y)
	target_pos.y = PLAYER_HEIGHT
	
	# Smooth movement
	var tween = create_tween()
	tween.tween_property(player_scene, "position", target_pos, MOVEMENT_TWEEN_DURATION)
	
	# Update player label
	var player_label = player_scene.get_node("Label3D") as Label3D
	if player_label:
		player_label.text = "Player_" + player_data.team + "_" + str(player_id)
	
	# Update player color based on team and status
	_update_player_color(player_scene, player_data)

func _update_player_color(player_scene: Node3D, player_data: Dictionary):
	"""Update player color based on team and status"""
	var mesh_instance := player_scene.get_node("MeshInstance3D") as MeshInstance3D
	if not mesh_instance or not mesh_instance.material_override:
		return
		
	var new_material := mesh_instance.material_override.duplicate() as StandardMaterial3D
	var base_color = team_colors.get(player_data.team, Color.WHITE)
	
	# Modify color based on status
	match player_data.status:
		"incantation":
			base_color = base_color.lerp(Color.WHITE, 0.5)  # Lighter for incantation
		"broadcasting":
			base_color = base_color.lerp(Color.YELLOW, 0.3)  # Yellowish for broadcasting
		_:
			pass  # Normal color
	
	new_material.albedo_color = base_color
	mesh_instance.material_override = new_material

func get_player_count() -> int:
	"""Get the current number of players"""
	return players.size()

func get_player_scene(player_id: int) -> Node3D:
	"""Get the scene node for a specific player"""
	return players.get(player_id, null)
