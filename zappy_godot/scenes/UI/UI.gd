# UI.gd - Main UI Controller
extends Control

@onready var game_info_panel = $BottomBanner/HBox/GameInfoPanel
@onready var player_info_panel = $BottomBanner/HBox/PlayerInfoPanel
@onready var team_stats_panel = $BottomBanner/HBox/TeamStatsPanel
@onready var resource_stats_panel = $BottomBanner/HBox/ResourceStatsPanel

# UI Labels - Game Info
@onready var tick_label = $BottomBanner/HBox/GameInfoPanel/VBox/TickLabel
@onready var time_unit_label = $BottomBanner/HBox/GameInfoPanel/VBox/TimeUnitLabel

# UI Labels - Player Info (selected player)
@onready var player_id_label = $BottomBanner/HBox/PlayerInfoPanel/VBox/PlayerIDLabel
@onready var player_pos_label = $BottomBanner/HBox/PlayerInfoPanel/VBox/PositionLabel
@onready var player_level_label = $BottomBanner/HBox/PlayerInfoPanel/VBox/LevelLabel
@onready var player_team_label = $BottomBanner/HBox/PlayerInfoPanel/VBox/TeamLabel
@onready var player_status_label = $BottomBanner/HBox/PlayerInfoPanel/VBox/StatusLabel
@onready var player_inventory_container = $BottomBanner/HBox/PlayerInfoPanel/VBox/InventoryContainer

# UI Container - Team Stats
@onready var teams_container = $BottomBanner/HBox/TeamStatsPanel/VBox/TeamsContainer

# UI Container - Resource Stats
@onready var resources_container = $BottomBanner/HBox/ResourceStatsPanel/VBox/ResourcesContainer

var selected_player_id = -1

func _ready():
	# Connect to GameData signals
	GameData.connect("game_state_updated", _on_game_state_updated)
	GameData.connect("player_updated", _on_player_updated)
	GameData.connect("team_updated", _on_team_updated)
	
	_setup_ui()

func _setup_ui():
	"""Initialize UI elements"""
	# Set up resource display
	_setup_resource_display()
	
	# Set initial values
	_update_game_info()
	_update_team_stats()
	_update_resource_stats()

func _setup_resource_display():
	"""Create UI elements for resource inventory display"""
	# Clear existing children
	for child in player_inventory_container.get_children():
		child.queue_free()
	
	for child in resources_container.get_children():
		child.queue_free()
	
	# Create labels for each resource type (compact horizontal layout for inventory)
	for resource in GameData.RESOURCES:
		# Player inventory (compact)
		var inv_label = Label.new()
		inv_label.name = resource + "_inv_label"
		inv_label.text = resource.capitalize().substr(0, 4) + ": 0"
		inv_label.add_theme_font_size_override("font_size", 10)
		player_inventory_container.add_child(inv_label)
		
		# Global resource stats (vertical)
		var res_label = Label.new()
		res_label.name = resource + "_res_label"
		res_label.text = resource.capitalize() + ": 0"
		res_label.add_theme_font_size_override("font_size", 11)
		resources_container.add_child(res_label)

func _on_game_state_updated():
	"""Called when the entire game state is updated"""
	_update_game_info()
	_update_team_stats()
	_update_resource_stats()
	if selected_player_id != -1:
		_update_player_display(selected_player_id)

func _on_player_updated(player_id):
	"""Called when a specific player is updated"""
	if selected_player_id == player_id:
		_update_player_display(player_id)

func _on_team_updated(team_name):
	"""Called when a team is updated"""
	_update_team_stats()

func _update_game_info():
	"""Update game information display"""
	if GameData.game_info.has("tick"):
		tick_label.text = "Tick: " + str(GameData.game_info.tick)
	if GameData.game_info.has("time_unit"):
		time_unit_label.text = "Time Unit: " + str(GameData.game_info.time_unit)

func _update_player_display(player_id: int):
	"""Update the player information panel for the selected player"""
	var player_data = GameData.get_player_data(player_id)
	if not player_data:
		return
	
	player_id_label.text = "Player ID: " + str(player_id)
	player_pos_label.text = "Position: (" + str(player_data.position.x) + ", " + str(player_data.position.y) + ")"
	player_level_label.text = "Level: " + str(player_data.level)
	player_team_label.text = "Team: " + str(player_data.team)
	player_status_label.text = "Status: " + str(player_data.status)
	
	# Update inventory (compact display)
	for resource in GameData.RESOURCES:
		var label = player_inventory_container.get_node(resource + "_inv_label")
		if label and player_data.inventory.has(resource):
			label.text = resource.capitalize().substr(0, 4) + ": " + str(player_data.inventory[resource])

func _update_team_stats():
	"""Update team statistics display"""
	# Clear existing team labels
	for child in teams_container.get_children():
		child.queue_free()
	
	# Create new team labels (compact)
	for team_name in GameData.teams:
		var team_data = GameData.teams[team_name]
		var team_label = Label.new()
		team_label.text = team_name + ": " + str(team_data.player_count) + "p (" + str(team_data.remaining_connections) + "s)"
		team_label.add_theme_font_size_override("font_size", 11)
		teams_container.add_child(team_label)

func _update_resource_stats():
	"""Update global resource statistics"""
	for resource in GameData.RESOURCES:
		var label = resources_container.get_node(resource + "_res_label")
		if label:
			var total = GameData.get_resource_total(resource)
			label.text = resource.capitalize() + ": " + str(total)

func select_player(player_id: int):
	"""Select a player to display their information"""
	selected_player_id = player_id
	_update_player_display(player_id)

func _unhandled_input(event):
	"""Handle input for player selection (example with number keys)"""
	if event is InputEventKey and event.pressed:
		# Select players 1-9 with number keys
		if event.keycode >= KEY_1 and event.keycode <= KEY_9:
			var player_id = event.keycode - KEY_1 + 1
			if GameData.players.has(player_id):
				select_player(player_id)
