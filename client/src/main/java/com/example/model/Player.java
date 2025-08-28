package com.example.model;

import com.example.ai.*;
import com.example.command.*;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import java.util.List;
import java.util.ArrayList;
import java.util.Queue;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicBoolean;

public class Player {
    private String team;
    private int id;
    private CommandManager cmdManager;
    private AI ai;
    private final AtomicInteger level;
    private final AtomicInteger life;
    private final AtomicInteger nour;
    private World world;
    private Position position;
    private final Map<Resource, Integer> inventory;
    private final AtomicBoolean dead = new AtomicBoolean(false);

    public Player(String teamName, int id) {
        this.team = teamName;
        // this.ai = new AI(teamName);
        this.level = new AtomicInteger(1);
        this.id = id;
        this.inventory = new ConcurrentHashMap<>();
        this.life = new AtomicInteger(1260); // time units
        this.nour = new AtomicInteger(0);
    }

    public void incrementLevel() {
        level.incrementAndGet();
    }

    public void addLife(int life) {
        this.life.addAndGet(life);
    }

    public boolean isDead() {
        return dead.get();
    }

    public void moveForward() {
        position.moveForward();
    }

    public void turnRight() {
        position.turnRight();
    }

    public void turnLeft() {
        position.turnLeft();
    }

    public void updateInventory(Resource item, int count) {
        this.inventory.put(item, count);
    }

    public void addResource(Resource item) {
        this.inventory.compute(item, (k, v) -> (v == null) ? 1 : v + 1);
    }

    public void removeResource(Resource item) {
        this.inventory.computeIfPresent(item, (k, v) -> (v > 1) ? v - 1 : null);
    }

    /********** BROADCAST **********/
 
    public Command broadcastCmd(String event, String status, int level, int playersNeeded) { // level - cur level
        JsonObject broadcastMessage = new JsonObject();
        broadcastMessage.addProperty("event", event);
        broadcastMessage.addProperty("status", status);
        if (level > 0) {
            broadcastMessage.addProperty("level", level);
        }
        if (playersNeeded > 0) {
            broadcastMessage.addProperty("players_needed", playersNeeded);
        }
        return new Command(CommandType.BROADCAST, broadcastMessage.toString());
    }

    public void handleBroadcastMessage(String rawMsg, int dir) {
        try {
            JsonObject msg = JsonParser.parseString(rawMsg).getAsJsonObject();
            String event = msg.has("event") ? msg.get("event").getAsString() : "unknown";
            String status = msg.has("status") ? msg.get("status").getAsString() : "unknown";

            if ("elevation".equals(event) && "call".equals(status)) {
                int level = msg.get("level").getAsInt();
                int playersNeeded = msg.get("players_needed").getAsInt();
                System.out.println("[CLIENT " + this.id + "] Received elevation call for level " + level + ", from dir " + dir);
                if (level == this.level.get()) {
                    System.out.println("[CLIENT " + this.id + "] Elevation call matches my level, preparing incantation ...");
                    List<Command> cmds = ai.goToElevationCall(dir, level, playersNeeded);
                } else {
                    System.out.println("[CLIENT " + this.id + "] Elevation call does not match my level, ignoring.");
                }
            } else if ("elevation".equals(event) && "ko".equals(status)) {
                // System.out.println("[CLIENT " + this.id + "] Received elevation error (ko) for level " + level + ", from dir " + dir);
            } else {
                System.out.println("[CLIENT " + this.id + "] Unhandled broadcast message: " + rawMsg);
            }

        } catch (Exception e) {
            System.err.println("[CLIENT " + this.id + "] Failed to parse broadcast message: " + rawMsg);
            e.printStackTrace();
        }
    }

    /********** GETTERS **********/

    public String getTeamName() {
        return this.team;
    }

    public int getLevel() {
        return this.level.get();
    }

    public int getId() {
        return this.id;
    }

    public int getLife() {
        return this.life.get();
    }

    public int getNour() {
        return this.nour.get();
    }

    public Position getPosition() {
        return this.position;
    }

    public Map<Resource, Integer> getInventory() {
        return new ConcurrentHashMap<>(this.inventory); // returns a copy (to be safe)
    }

    public int getInventoryCount(Resource item) {
        return this.inventory.getOrDefault(item, 0);
    }

    public int getNourriture() {
        return getInventoryCount(Resource.NOURRITURE);
    }

    /********** SETTERS **********/

    public void setCommandManager(CommandManager commandManager) {
        this.cmdManager = commandManager;
    }

    public void setLevel(int level) {
        this.level.set(level);
    }

    public void setLife(int life) {
        this.life.set(life);
    }

    public void setDead(boolean value) {
        dead.set(value);
    }

    public void setGameState(int w, int h, AI ai) {
        this.world = new World(w, h);
        this.position = new Position(w, h);
        this.ai = ai;
        this.ai.setWorld(this.world);
    }
}
