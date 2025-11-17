extends Control

@onready var mode_button = $VBoxContainer/ModeButton
@onready var server_input = $VBoxContainer/HBoxContainer/ServerInput
@onready var port_input = $VBoxContainer/HBoxContainer/PortInput
@onready var team_input = $VBoxContainer/HBoxContainer/TeamInput
@onready var connect_button = $VBoxContainer/ConnectButton
@onready var status_label = $VBoxContainer/StatusLabel

var integration_manager: Node

func _ready():
	integration_manager = get_node("../IntegrationManager")
	
	mode_button.text = "Mode: MOCK"
	server_input.text = "127.0.0.1"
	port_input.text = "8674"
	team_input.text = "team1"
	status_label.text = "Status: Using Mock Server"
	
	mode_button.pressed.connect(_on_mode_toggle)
	connect_button.pressed.connect(_on_connect_pressed)

func _on_mode_toggle():
	if mode_button.text.contains("MOCK"):
		mode_button.text = "Mode: REAL"
		connect_button.disabled = false
		status_label.text = "Status: Ready to connect to real server"
	else:
		mode_button.text = "Mode: MOCK"  
		connect_button.disabled = true
		integration_manager.use_mock_server()
		status_label.text = "Status: Using Mock Server"

func _on_connect_pressed():
	var ip = server_input.text
	var port = int(port_input.text)
	var team = team_input.text
	
	status_label.text = "Status: Connecting to " + ip + ":" + str(port) + "..."
	integration_manager.connect_to_real_server(ip, port, team)
	
	await get_tree().create_timer(2.0).timeout
	status_label.text = "Status: Connected to real server (check console for details)"
