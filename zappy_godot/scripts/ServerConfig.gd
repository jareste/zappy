extends Node

var server_config = {
	"ip": "127.0.0.1",
	"port": 8674,
	"use_ssl": true,
	"team_name": "default_team"
}

func set_server(ip: String, port: int, use_ssl: bool = true):
	server_config.ip = ip
	server_config.port = port
	server_config.use_ssl = use_ssl
	
	OS.set_environment("ZAPPY_SERVER_IP", ip)
	OS.set_environment("ZAPPY_SERVER_PORT", str(port))
	
	print("Server configured: ", ip, ":", port, " (SSL: ", use_ssl, ")")

func get_server_url() -> String:
	var protocol = "wss" if server_config.use_ssl else "ws"
	return protocol + "://" + server_config.ip + ":" + str(server_config.port)

func set_team_name(team: String):
	server_config.team_name = team

func get_team_name() -> String:
	return server_config.team_name
