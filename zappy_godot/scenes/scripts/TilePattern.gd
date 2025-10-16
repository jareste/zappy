extends Resource
class_name TilePatterqn

@export var pattern_name: String = ""
@export var min_size: Vector2i = Vector2i(1, 1)
@export var max_size: Vector2i = Vector2i(100, 100)
@export var rules: Array[TileRule] = []
@export var weight: float = 1.0
@export var description: String = ""
