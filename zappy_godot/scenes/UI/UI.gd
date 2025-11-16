# UI.gd - Main UI Controller
extends Control

# Tile Info Panel labels
@onready var tile_info_panel: Panel = $TileInfoPanel
@onready var title_label: Label = $TileInfoPanel/VBox/TitleLabel
@onready var tile_id_label: Label = $TileInfoPanel/VBox/TileIDLabel
@onready var tile_position_label: Label = $TileInfoPanel/VBox/TilePositionLabel
@onready var tile_players_label: Label = $TileInfoPanel/VBox/TilePlayersLabel
@onready var tile_eggs_label: Label = $TileInfoPanel/VBox/TileEggsLabel
@onready var tile_nourriture_label: Label = $TileInfoPanel/VBox/TileNourritureLabel
@onready var tile_linemate_label: Label = $TileInfoPanel/VBox/TileLinemateLabel
@onready var tile_deraumere_label: Label = $TileInfoPanel/VBox/TileDeraumereLabel
@onready var tile_sibur_label: Label = $TileInfoPanel/VBox/TileSiburLabel
@onready var tile_mendiane_label: Label = $TileInfoPanel/VBox/TileMendianeLabel
@onready var tile_phiras_label: Label = $TileInfoPanel/VBox/TilePhirasLabel
@onready var tile_thystame_label: Label = $TileInfoPanel/VBox/TileThystameLabel

# Player Info Panel labels
@onready var player_info_panel: Panel = $PlayerInfoPanel
@onready var player_title_label: Label = $PlayerInfoPanel/VBox/PlayerTitleLabel
@onready var player_id_label: Label = $PlayerInfoPanel/VBox/PlayerIDLabel
@onready var player_position_label: Label = $PlayerInfoPanel/VBox/PlayerPositionLabel
@onready var player_level_label: Label = $PlayerInfoPanel/VBox/PlayerLevelLabel
@onready var player_orientation_label: Label = $PlayerInfoPanel/VBox/PlayerOrientationLabel
@onready var player_team_label: Label = $PlayerInfoPanel/VBox/PlayerTeamLabel
@onready var player_status_label: Label = $PlayerInfoPanel/VBox/PlayerStatusLabel
@onready var player_nourriture_label: Label = $PlayerInfoPanel/VBox/PlayerNourritureLabel
@onready var player_linemate_label: Label = $PlayerInfoPanel/VBox/PlayerLinemateLabel
@onready var player_deraumere_label: Label = $PlayerInfoPanel/VBox/PlayerDeraumereLabel
@onready var player_sibur_label: Label = $PlayerInfoPanel/VBox/PlayerSiburLabel
@onready var player_mendiane_label: Label = $PlayerInfoPanel/VBox/PlayerMendianeLabel
@onready var player_phiras_label: Label = $PlayerInfoPanel/VBox/PlayerPhirasLabel
@onready var player_thystame_label: Label = $PlayerInfoPanel/VBox/PlayerThystameLabel

# Resource label
@onready var resource_type_panel: Panel = $ResourceTypePanel
@onready var resource_label: Label = $ResourceTypePanel/VBox/ResourceLabel

# Egg label
@onready var egg_info_panel: Panel = $EggInfoPanel
@onready var egg_id_label: Label = $EggInfoPanel/VBox/EggIDLabel
@onready var egg_position_label: Label = $EggInfoPanel/VBox/EggPositionLabel
@onready var egg_team_label: Label = $EggInfoPanel/VBox/EggTeamLabel
@onready var egg_status_label: Label = $EggInfoPanel/VBox/EggStatusLabel

# Panel - cursor tracking variables
var is_hovering_tile:= false
var is_hovering_player:= false
var is_hovering_egg:= false
var is_hovering_resource:= false
var is_following_cursor:= false
var follow_cursor_timer:= 0.0

func _ready():
	_setup_ui()
	
	# Connect to GameData updates
	if GameData.has_signal("game_state_updated"):
		GameData.connect("game_state_updated", _on_game_state_updated)

func _process(delta: float) -> void:
	if is_following_cursor and tile_info_panel.visible and is_hovering_tile:
		_position_panel_near_cursor(tile_info_panel)
	elif is_following_cursor and player_info_panel.visible and is_hovering_player:
		_position_panel_near_cursor(player_info_panel)
	elif is_following_cursor and resource_type_panel.visible and is_hovering_resource:
		_position_panel_near_cursor(resource_type_panel)
	elif is_following_cursor and egg_info_panel.visible and is_hovering_egg:
		_position_panel_near_cursor(egg_info_panel)

func _setup_ui():
	"""Initialize UI elements"""
	pass

func _on_game_state_updated():
	"""Called when the entire game state is updated"""
	pass

func update_ui_tile_stats(tile_data, tile_x: int, tile_y: int):
	if not tile_nourriture_label or not tile_info_panel or not tile_data:
		tile_info_panel.visible = false
		return
	
	is_hovering_tile = true
	tile_info_panel.visible = true
	is_following_cursor = true

	tile_id_label.text = "ID:" + str(tile_data.id)
	tile_position_label.text = "Position: (" + str(tile_x) + ", " + str(tile_y) + ")"

	tile_players_label.text = "Players: " + str(tile_data.get("players", []).size())
	tile_eggs_label.text = "Eggs: " + str(tile_data.get("eggs", []).size())

	var resources = tile_data.get("resources", {})
	tile_nourriture_label.text = "Nourriture: " + str(resources.get("nourriture", 0))
	tile_linemate_label.text = "Linemate: " + str(resources.get("linemate", 0))
	tile_deraumere_label.text = "Deraumere: " + str(resources.get("deraumere", 0))
	tile_sibur_label.text = "Sibur: " + str(resources.get("sibur", 0))
	tile_mendiane_label.text = "Mendiane: " + str(resources.get("mendiane", 0))
	tile_phiras_label.text = "Phiras: " + str(resources.get("phiras", 0))
	tile_thystame_label.text = "Thystame: " + str(resources.get("thystame", 0))

	#Info panel reposition
	_position_panel_near_cursor(tile_info_panel)

func update_ui_player_stats(player_id: int):
	var player_data = GameData.players[player_id]

	if not player_info_panel or not player_data:
		player_info_panel.visible = false
		return
	
	is_hovering_player = true
	player_info_panel.visible = true
	is_following_cursor = true

	player_id_label.text = "ID: " + str(player_data.id)
	player_position_label.text = "Position: " + str(player_data.position)
	player_level_label.text = "Level: " + str(player_data.level)
	player_orientation_label.text = "Orientation: " + str(player_data.orientation)
	player_team_label.text = "Team: " + str(player_data.team)
	player_status_label.text = "Status: " + str(player_data.status)

	var inventory = player_data.get("inventory", {})
	player_nourriture_label.text = "Nourriture: " + str(inventory.get("nourriture", 0))
	player_linemate_label.text = "Linemate: " + str(inventory.get("linemate", 0))
	player_deraumere_label.text = "Deraumere: " + str(inventory.get("deraumere", 0))
	player_sibur_label.text = "Sibur: " + str(inventory.get("sibur", 0))
	player_mendiane_label.text = "Mendiane: " + str(inventory.get("mendiane", 0))
	player_phiras_label.text = "Phiras: " + str(inventory.get("phiras", 0))
	player_thystame_label.text = "Thystame: " + str(inventory.get("thystame", 0))
	_position_panel_near_cursor(player_info_panel)


func _position_panel_near_cursor(panel: Panel):
	"""Position the info panel near the cursor with window boundary checks"""
	var mouse_pos = get_global_mouse_position()
	var panel_size = panel.size
	var screen_size = get_viewport().get_visible_rect().size

	var offset = Vector2(20, -10)
	var new_pos = mouse_pos + offset

	# Boundary check
	if new_pos.x + panel_size.x > screen_size.x:
		new_pos.x = mouse_pos.x - panel_size.x - 20
	
	if new_pos.y + panel_size.y > screen_size.y:
		new_pos.y = mouse_pos.y - panel_size.y +10
	
	if new_pos.y < 0:
		new_pos.y = mouse_pos.y + 20

	new_pos.x = max(5, min(new_pos.x, screen_size.x - panel_size.x - 5))
	new_pos.y = max(5, min(new_pos.y, screen_size.y - panel_size.y - 5))

	panel.global_position = new_pos

func hide_tile_info():
	"""Hide the tile info panel and stop following the cursor"""
	is_hovering_tile = false
	tile_info_panel.visible = false
	is_following_cursor = false

func hide_player_info():
	"""Hide the player info panel and stop following the cursor"""
	is_hovering_player = false
	player_info_panel.visible = false
	is_following_cursor = false

func update_resource_label_display(type: String, position: Vector2i):
	if not type:
		resource_type_panel.visible = false
		return

	is_hovering_resource = true
	resource_type_panel.visible = true
	is_following_cursor = true
	
	# Use bracket notation to access dictionary with variable key
	var quantity = GameData.tiles[position].resources[type.to_lower()]

	resource_label.text = type + " (" + str(quantity) + ")"
	
	_position_panel_near_cursor(resource_type_panel)

func hide_resource_label_display():
	is_hovering_resource = false
	resource_type_panel.visible = false
	is_following_cursor = false
	resource_label.text = "unknown"
	
func update_egg_panel(egg_id: int):
	var egg_data = GameData.get_egg_data(egg_id)

	if not egg_info_panel or not egg_data:
		egg_info_panel.visible = false

	is_hovering_egg = true
	egg_info_panel.visible = true
	is_following_cursor = true

	egg_id_label.text = "ID: " + str(int(egg_data.id))
	egg_position_label.text = "Position: " + str(egg_data.position)
	egg_team_label.text = "Team: " + str(egg_data.team)
	egg_status_label.text = "Status: " + str(egg_data.status)

	_position_panel_near_cursor(egg_info_panel)

	
func hide_egg_panel():
	is_hovering_egg = false
	egg_info_panel.visible = false
	is_following_cursor = false
