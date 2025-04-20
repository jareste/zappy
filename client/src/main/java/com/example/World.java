package com.example;

import java.util.List;
import java.util.ArrayList;

public class World {
    private final int width;
    private final int height;
    private final Tile[][] grid;

    public World(int width, int height) {
        this.width = width;
        this.height = height;
        this.grid = new Tile[height][width]; // y rows, each with x columns
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x] = new Tile();
            }
        }
    }

    public Tile getTile(int x, int y) {
        int wrappedX = (x + width) % width;
        int wrappedY = (y + height) % height;
        return grid[wrappedY][wrappedX];
    }

    public void updateTile(int x, int y, List<String> contents) {
        getTile(x, y).update(contents);
    }

    // will use this after 'voir' to update the known map
    public void updateVisibleTiles(int px, int py, int direction, int level, List<List<String>> viewData) {
        List<int[]> relativeOffsets = generateRelativeOffsets(level);
        int n = viewData.size();

        for (int i = 0; i < n; i++) {
            int[] offset = relativeOffsets.get(i);
            int dx = offset[0];
            int dy = offset[1];

            // Rotate the offsets based on the player's direction
            int[] rotatedOffset = rotate(dx, dy, direction);
            dx = rotatedOffset[0];
            dy = rotatedOffset[1];
            int tx = (px + dx + width) % width;
            int ty = (py + dy + height) % height;

            // Update the tile with the new data
            updateTile(tx, ty, viewData.get(i));
        }
    }

    // for level=1 this will be:
    // [-1, -1], [0, -1], [1, -1],
    //           [0, 0]
    public List<int[]> generateRelativeOffsets(int level) {
        List<int[]> offsets = new ArrayList<>();
        for (int l = 0; l <= level; l++) {
            for (int i = -l; i <= l; i++) {
                offsets.add(new int[]{i, -l}); // assuming player faces NORTH, adjust later
            }
        }
        return offsets;
    }

    // TODO: implement this directly in prev function
    // for not calling for each tile
    public int[] rotate(int dx, int dy, int direction) {
        switch (direction) {
            case 0: return new int[]{dx, dy};         // ^ NORTH 
            case 1: return new int[]{-dy, dx};        // > EAST
            case 2: return new int[]{-dx, -dy};       // v SOUTH
            case 3: return new int[]{dy, -dx};        // < WEST
            default: throw new IllegalArgumentException("Invalid direction");
        }
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }
}
