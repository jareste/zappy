extends Node

enum PatternType {
	# Small patterns (≤5x5)
	SMALL_FORTRESS,
	SMALL_ARENA,
	SMALL_TOWER,
	
	# Medium patterns (6-12)
	MEDIUM_COURTYARD,
	MEDIUM_WALLS,
	MEDIUM_CORNER_TOWERS,
	MEDIUM_CENTER_TOWER,
	
	# Large patterns (>12)
	LARGE_MAZE,
	LARGE_TOWERS,
	LARGE_STRONGHOLD,

	# DEBUG
	DEBUG
}

func _ready() -> void:
	print("WorldBuilder initialized with simple pattern system")

func select_pattern(debug_mode: bool, map_size: Vector2i) -> PatternType:
	"""Select a random pattern based on map size"""
	var max_dimension = max(map_size.x, map_size.y)
	
	if debug_mode:
		return PatternType.DEBUG

	if max_dimension <= 5:
		# Small patterns
		return [PatternType.SMALL_FORTRESS, PatternType.SMALL_ARENA, PatternType.SMALL_TOWER].pick_random()
	elif max_dimension <= 12:
		# Medium patterns  
		return [PatternType.MEDIUM_COURTYARD, PatternType.MEDIUM_WALLS, PatternType.MEDIUM_CORNER_TOWERS, PatternType.MEDIUM_CENTER_TOWER].pick_random()
	else:
		# Large patterns
		return [PatternType.LARGE_MAZE, PatternType.LARGE_TOWERS, PatternType.LARGE_STRONGHOLD].pick_random()

func get_tile_type_for_position(pattern: PatternType, x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Get the tile type for a specific position based on the selected pattern"""
	match pattern:
		PatternType.SMALL_FORTRESS:
			return _build_small_fortress(x, y, map_size)
		PatternType.SMALL_ARENA:
			return _build_small_arena(x, y, map_size)
		PatternType.SMALL_TOWER:
			return _build_small_tower(x, y, map_size)
		PatternType.MEDIUM_COURTYARD:
			return _build_medium_courtyard(x, y, map_size)
		PatternType.MEDIUM_WALLS:
			return _build_medium_walls(x, y, map_size)
		PatternType.MEDIUM_CORNER_TOWERS:
			return _build_corner_towers(x, y, map_size)
		PatternType.MEDIUM_CENTER_TOWER:
			return _build_center_tower(x, y, map_size)
		PatternType.LARGE_MAZE:
			return _build_large_maze(x, y, map_size)
		PatternType.LARGE_TOWERS:
			return _build_large_towers(x, y, map_size)
		PatternType.LARGE_STRONGHOLD:
			return _build_large_stronghold(x, y, map_size)
		_:
			return TileRule.TileType.BASIC

func get_pattern_name(pattern: PatternType) -> String:
	"""Get a descriptive name for a pattern"""
	match pattern:
		PatternType.SMALL_FORTRESS:
			return "Small Fortress"
		PatternType.SMALL_ARENA:
			return "Small Arena"
		PatternType.SMALL_TOWER:
			return "Small Tower"
		PatternType.MEDIUM_COURTYARD:
			return "Medium Courtyard"
		PatternType.MEDIUM_WALLS:
			return "Medium Walls"
		PatternType.MEDIUM_CORNER_TOWERS:
			return "Corner Towers"
		PatternType.MEDIUM_CENTER_TOWER:
			return "Center Tower"
		PatternType.LARGE_MAZE:
			return "Large Maze"
		PatternType.LARGE_TOWERS:
			return "Large Towers"
		PatternType.LARGE_STRONGHOLD:
			return "Large Stronghold"
		_:
			return "Unknown Pattern"

# Pattern Building Functions

func _build_small_tower(x:int, y:int, map_size: Vector2i) -> TileRule.TileType:
	"""Small corner tower"""
	if _is_north_corner(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif _is_north_sub_corner(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	elif _is_north_sub_sub_corner(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_small_fortress(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Small fortress: 1F walls around border, basic interior"""
	if _is_border(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_small_arena(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Small arena: 2F towers at corners, basic elsewhere"""
	if _is_corner(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	return TileRule.TileType.BASIC

func _build_medium_courtyard(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Medium courtyard: 3F outer walls, 1F inner border, basic center"""
	if _is_border(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif _is_inner_border(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_medium_walls(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Medium walls: 2F corners, 1F borders, basic interior"""
	if _is_corner(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	elif _is_border(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_corner_towers(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Corner Walls: from corners, 3f -> 2f -> 1f"""
	if _is_corner(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif _is_sub_corner(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	elif _is_sub_sub_corner(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_center_tower(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Center tower: 3F center (4 tiles), 2F surrounding ring, 1F next ring, basic rest"""
	if _is_center_core(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif _is_center_ring_1(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	elif _is_center_ring_2(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_large_maze(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Large maze: Varying wall heights creating maze-like patterns"""
	if _is_border(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif y == 1 or y == map_size.y - 2:
		return TileRule.TileType.ARCH_2F
	elif x == 1 or x == map_size.x - 2:
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

func _build_large_towers(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Large towers: Only 3F corner towers, basic everywhere else"""
	if _is_corner(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	return TileRule.TileType.BASIC

func _build_large_stronghold(x: int, y: int, map_size: Vector2i) -> TileRule.TileType:
	"""Large stronghold: 3F corners, 2F borders, 1F inner border, basic center"""
	if _is_corner(x, y, map_size):
		return TileRule.TileType.ARCH_3F
	elif _is_border(x, y, map_size):
		return TileRule.TileType.ARCH_2F
	elif _is_inner_border(x, y, map_size):
		return TileRule.TileType.ARCH_1F
	return TileRule.TileType.BASIC

# Helper Functions for Position Detection

func _is_border(x: int, y: int, map_size: Vector2i) -> bool:
	return x == 0 or y == 0 or x == map_size.x - 1 or y == map_size.y - 1

func _is_corner(x: int, y: int, map_size: Vector2i) -> bool:
	return (x == 0 or x == map_size.x - 1) and (y == 0 or y == map_size.y - 1)
	
func _is_north_corner(x: int, y: int, map_size: Vector2i) -> bool:
	return (x == 0 and y == 0)
	
func _is_north_sub_corner(x: int, y: int, map_size: Vector2i) -> bool:
	if ((x == 0 and y == 1)
	or (x == 1 and (y == 0 or y == 1))):
		return true
	return false
	
func _is_north_sub_sub_corner(x: int, y: int, map_size: Vector2i) -> bool:
	if ((x == 0 and y == 2)
	or (x == 1 and y == 2)
	or (x == 2 and (y == 0 or y == 1 or y == 2))):
		return true
	return false

func _is_sub_corner(x: int, y: int, map_size: Vector2i) -> bool:
	if (x == 0 and (y == 1 or y == map_size.y - 2)):
		return true
	elif (x == 1 and (y == 0 or y == 1 or y == map_size.y - 1 or y == map_size.y - 2)):
		return true
	elif (x == map_size.x - 2 and (y == 0 or y == 1 or y == map_size.y - 1 or y == map_size.y - 2)):
		return true
	elif (x == map_size.x - 1 and (y == 1 or y == map_size.y - 2)):
		return true
	return false

func _is_sub_sub_corner(x: int, y: int, map_size: Vector2i) -> bool:
	if (x == 0 and (y == 2 or y == map_size.y - 3)):
		return true
	elif (x == 1 and (y == 2 or y == map_size.y - 3)):
		return true
	elif (x == 2 and (y == 0 or y == 1 or y == 2 or y == map_size.y - 3 or y == map_size.y - 2 or y == map_size.y - 1)):
		return true
	elif (x == map_size.x - 3 and (y == 0 or y == 1 or y == 2 or y == map_size.y - 3 or y == map_size.y - 2 or y == map_size.y - 1)):
		return true
	elif (x == map_size.x - 2 and (y == 2 or y == map_size.y - 3)):
		return true
	elif (x == map_size.x - 1 and (y == 2 or y == map_size.y - 3)):
		return true
	return false

func _is_center(x: int, y: int, map_size: Vector2i) -> bool:
	@warning_ignore("INTEGER_DIVISION")
	var center_x = map_size.x / 2
	@warning_ignore("INTEGER_DIVISION") 
	var center_y = map_size.y / 2
	return abs(x - center_x) <= 1 and abs(y - center_y) <= 1

func _is_inner_border(x: int, y: int, map_size: Vector2i) -> bool:
	return ((x == 1 or x == map_size.x - 2) or (y == 1 or y == map_size.y - 2)) and not _is_border(x, y, map_size)

func _is_inner_center(x: int, y: int, map_size: Vector2i) -> bool:
	return not _is_border(x, y, map_size) and not _is_inner_border(x, y, map_size)


func _is_center_core(x: int, y: int, map_size: Vector2i) -> bool:
	"""Check if position is in the 2x2 center core (3F tiles)"""
	@warning_ignore("INTEGER_DIVISION")
	var center_x = map_size.x / 2
	@warning_ignore("INTEGER_DIVISION") 
	var center_y = map_size.y / 2
	
	return (x >= center_x - 1 and x <= center_x) and (y >= center_y - 1 and y <= center_y)

func _is_center_ring_1(x: int, y: int, map_size: Vector2i) -> bool:
	"""Check if position is in the 4x4 hollowed ring around center (2F tiles)"""
	@warning_ignore("INTEGER_DIVISION")
	var center_x = map_size.x / 2
	@warning_ignore("INTEGER_DIVISION") 
	var center_y = map_size.y / 2
	
	var in_4x4_area = (x >= center_x - 2 and x <= center_x + 1) and (y >= center_y - 2 and y <= center_y + 1)
	return in_4x4_area and not _is_center_core(x, y, map_size)

func _is_center_ring_2(x: int, y: int, map_size: Vector2i) -> bool:
	"""Check if position is in the 6x6 hollowed ring around center (1F tiles)"""
	@warning_ignore("INTEGER_DIVISION")
	var center_x = map_size.x / 2
	@warning_ignore("INTEGER_DIVISION") 
	var center_y = map_size.y / 2

	var in_6x6_area = (x >= center_x - 3 and x <= center_x + 2) and (y >= center_y - 3 and y <= center_y + 2)
	var in_4x4_area = (x >= center_x - 2 and x <= center_x + 1) and (y >= center_y - 2 and y <= center_y + 1)
	return in_6x6_area and not in_4x4_area

func verify_center_tower_8x8():
	"""Manually verify the CENTER_TOWER pattern matches expected output"""
	var map_size = Vector2i(8, 8)
	print("Verifying CENTER_TOWER 8x8 pattern:")
	print("Expected: 2x2 center (3F), 4x4 ring (2F), 6x6 ring (1F), rest Basic")
	
	# Test key positions
	var test_cases = [
		[Vector2i(3,3), "3F", "center core"],
		[Vector2i(3,4), "3F", "center core"], 
		[Vector2i(4,3), "3F", "center core"],
		[Vector2i(4,4), "3F", "center core"],
		[Vector2i(2,2), "2F", "4x4 ring corner"],
		[Vector2i(5,5), "2F", "4x4 ring corner"],
		[Vector2i(3,2), "2F", "4x4 ring edge"],
		[Vector2i(1,1), "1F", "6x6 ring corner"],
		[Vector2i(6,6), "1F", "6x6 ring corner"],
		[Vector2i(0,0), "B", "outside all rings"],
		[Vector2i(7,7), "B", "outside all rings"]
	]
	
	for test_case in test_cases:
		var pos = test_case[0]
		var expected = test_case[1]
		var description = test_case[2]
		var actual_type = get_tile_type_for_position(PatternType.MEDIUM_CENTER_TOWER, pos.x, pos.y, map_size)
		var actual = ""
		match actual_type:
			TileRule.TileType.BASIC: actual = "B"
			TileRule.TileType.ARCH_1F: actual = "1F"
			TileRule.TileType.ARCH_2F: actual = "2F" 
			TileRule.TileType.ARCH_3F: actual = "3F"
		
		var status = "✓" if actual == expected else "✗"
		print("  %s (%d,%d) %s: expected %s, got %s %s" % [status, pos.x, pos.y, description, expected, actual, "✓" if actual == expected else "✗"])
