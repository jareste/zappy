extends Resource
class_name TileRule

enum TileType {
	BASIC,
	ARCH_1F,
	ARCH_2F,
	ARCH_3F
}

enum PositionType {
	BORDER,
	CORNER,
	CENTER,
	EDGE_NORTH,
	EDGE_SOUTH,
	EDGE_EAST,
	EDGE_WEST,
	INNER_BORDER,
	INNER_NORTH,
	INNER_SOUTH,
	INNER_EAST,
	INNER_WEST,
	INNER_CENTER
}

@export var position_type: PositionType
@export var tile_type: TileType
@export var priority: int = 0
@export var description: String = ""
