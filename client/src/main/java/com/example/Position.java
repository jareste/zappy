package com.example;

public class Position {
    private int x;
    private int y;
    private int direction; // 0 = north, 1 = east, 2 = south, 3 = west
    private int worldWidth;
    private int worldHeight;

    public Position(int worldWidth, int worldHeight) {
        this.x = 0;
        this.y = 0;
        this.worldWidth = worldWidth;
        this.worldHeight = worldHeight;
        this.direction = 0;
    }

    public void moveForward() {
        switch (direction) {
            case 0: // north
                y = (y - 1 + worldHeight) % worldHeight;
                break;
            case 1: // east
                x = (x + 1) % worldWidth;
                break;
            case 2: // south
                y = (y + 1) % worldHeight;
                break;
            case 3: // west
                x = (x - 1 + worldWidth) % worldWidth;
                break;
        }
    }

    public void turnLeft() {
        direction = (direction + 3) % 4;
    }

    public void turnRight() {
        direction = (direction + 1) % 4;
    }

    public int getDirection() {
        return direction;
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public void setX(int x) {
        this.x = x;
    }

    public void setY(int y) {
        this.y = y;
    }

    @Override
    public String toString() {
        String dirStr;
        switch (direction) {
            case 0: dirStr = "North"; break;
            case 1: dirStr = "East"; break;
            case 2: dirStr = "South"; break;
            case 3: dirStr = "West"; break;
            default: dirStr = "Unknown"; break;
        }
        return String.format("Position(x=%d, y=%d, direction=%s)", x, y, dirStr);
    }
}