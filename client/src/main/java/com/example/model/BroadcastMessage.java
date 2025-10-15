package com.example.model;

public class BroadcastMessage {
    private String event;
    private String status;
    private int level;
    private int playersNeeded;
    private int direction; // direction from which the message was received

    public BroadcastMessage(String event, String status, int level, int playersNeeded, int direction) {
        this.event = event;
        this.status = status;
        this.level = level;
        this.playersNeeded = playersNeeded;
        this.direction = direction;
    }

    public String getEvent() {
        return event;
    }

    public String getStatus() {
        return status;
    }

    public int getLevel() {
        return level;
    }

    public int getPlayersNeeded() {
        return playersNeeded;
    }

    public int getDirection() {
        return direction;
    }
}
