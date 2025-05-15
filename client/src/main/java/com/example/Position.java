package com.example;

import java.util.concurrent.atomic.AtomicInteger;

public class Position {
    private final AtomicInteger x;
    private final AtomicInteger y;
    private final AtomicInteger direction; // 0 = north, 1 = east, 2 = south, 3 = west
    private final int worldWidth;
    private final int worldHeight;

    public Position(int worldWidth, int worldHeight) {
        this.x = new AtomicInteger(0);
        this.y = new AtomicInteger(0);
        this.direction = new AtomicInteger(0);
        this.worldWidth = worldWidth;
        this.worldHeight = worldHeight;
    }

    public synchronized void moveForward() {
        switch (getDirection()) {
            case 0: // north
                y.set((y.get() - 1 + worldHeight) % worldHeight);
                break;
            case 1: // east
                x.set((x.get() + 1) % worldWidth);
                break;
            case 2: // south
                y.set((y.get() + 1) % worldHeight);
                break;
            case 3: // west
                x.set((x.get() - 1 + worldWidth) % worldWidth);
                break;
        }
    }

    public synchronized void turnLeft() {
        direction.set((getDirection() + 3) % 4);
    }

    public synchronized void turnRight() {
        direction.set((getDirection() + 1) % 4);
    }

    /********** GETTERS **********/

    public int getDirection() {
        return this.direction.get();
    }

    public int getX() {
        return this.x.get();
    }

    public int getY() {
        return this.y.get();
    }

    /********** SETTERS **********/

    public void setX(int x) {
        this.x.set(x);
    }

    public void setY(int y) {
        this.y.set(y);
    }

    public void setDirection(int direction) {
        this.direction.set(direction);
    }

    @Override
    public String toString() {
        String dirStr;
        switch (getDirection()) {
            case 0: dirStr = "North"; break;
            case 1: dirStr = "East"; break;
            case 2: dirStr = "South"; break;
            case 3: dirStr = "West"; break;
            default: dirStr = "Unknown"; break;
        }
        return String.format("Position(x=%d, y=%d, direction=%s)", getX(), getY(), dirStr);
    }
}