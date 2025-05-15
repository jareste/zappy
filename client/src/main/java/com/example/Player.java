package com.example;

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

    public Player(String teamName, int id) {
        this.team = teamName;
        // this.ai = new AI(teamName);
        this.level = new AtomicInteger(1);
        this.id = id;
        this.inventory = new ConcurrentHashMap<>();
        this.life = new AtomicInteger(1260); // time units
        this.nour = new AtomicInteger(0);
    }

    public void handleResponse(JsonObject msg) {
        // msg == jsonResponse from server (from CommandManager)
        String cmd = msg.has("cmd") ? msg.get("cmd").getAsString() : msg.get("command").getAsString();
        System.out.println("[CLIENT " + this.id + "] " + "Handling response of COMMAND: " + cmd);
        String status = "";

        switch (cmd) {
            case "avance":
                handleAvanceResponse(msg);
                break;
            case "droite":
                handleDroiteResponse(msg);
                break;
            case "gauche":
                handleGaucheResponse(msg);
                break;
            case "voir":
                handleVoirResponse(msg);
                break;
            case "inventaire":
                handleInventaireResponse(msg);
                break;
            case "prend":
                handlePrendResponse(msg);
                break;
            case "pose":
                handlePoseResponse(msg);
                break;
            case "expulse":
                status = msg.has("status") ? msg.get("status").getAsString() : "ko";
                System.out.println("[CLIENT " + this.id + "] " + "Expulse response: " + status);
                break;
            case "broadcast":
                status = msg.has("status") ? msg.get("status").getAsString() : "ko";
                System.out.println("[CLIENT " + this.id + "] " + "Broadcast response: " + status);
                break;
            case "incantation":
                System.out.println("[CLIENT " + this.id + "] " + "Incantation response: " + msg);
                break;
            case "fork":
                status = msg.has("status") ? msg.get("status").getAsString() : "ko";
                System.out.println("[CLIENT " + this.id + "] " + "Fork response: " + status);
                break;
            case "connect_nbr":
                int value = msg.has("arg") ? msg.get("arg").getAsInt() : 0;
                System.out.println("[CLIENT " + this.id + "] " + "Connect number response: " + value);
                break;
            case "-":
                handleDieResponse(msg);
                return;
                // break;
            default:
                System.out.println("[CLIENT " + this.id + "] " + "Not handled (yet) command in response message.");
                break;
        }

        // addLife(-Command.timeUnits(cmd));
        addLife(-CommandType.fromName(cmd).getTimeUnits());
        
        System.out.println("[CLIENT " + this.id + "] " + "IN PLAYER Pending responses: " + cmdManager.getPendingResponses());
        if (!cmd.equals("voir") && cmdManager.getPendingResponses() == 0) {
            System.out.println("[CLIENT " + this.id + "] " + "Deciding next moves ...");
            List<Command> nextMoves = ai.decideNextMoves();
            for (Command c : nextMoves) {
                cmdManager.addCommand(c);
            }
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Not deciding next moves, waiting responses ...");
        }
    }

    /********** RESPONSE HANDLERS **********/

    private void handleAvanceResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            // System.out.println("Move successful!");
            this.position.moveForward();
            // this.life -= Command.timeUnits("avance");
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + this.position);
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Move failed :(");
        }
    }

    private void handleDroiteResponse(JsonObject msg) {
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            // System.out.println("Turn right successful!");
            this.position.turnRight();
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + this.position);
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Turn right failed :(");
        }
    }

    private void handleGaucheResponse(JsonObject msg) {
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            // System.out.println("Turn left successful!");
            this.position.turnLeft();
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + this.position);
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Turn left failed :(");
        }
    }

    private void handleVoirResponse(JsonObject msg) {
        List<List<String>> data = new ArrayList<>();
        JsonArray arr = msg.getAsJsonArray("vision");

        for (JsonElement tile : arr) {
            JsonArray tileArr = tile.getAsJsonArray();
            List<String> contents = new ArrayList<>();
            for (JsonElement item : tileArr) {
                contents.add(item.getAsString());
            }
            data.add(contents);
        }
        for (int i = 0; i < data.size(); i++) {
            System.out.println("[CLIENT " + this.id + "] " + "Tile " + i + ": " + data.get(i));
        }
        world.updateVisibleTiles(position.getX(), position.getY(), position.getDirection(), getLevel(), data);
        List<Command> nextMoves = ai.decideNextMovesViewBased(data);
        for (Command c : nextMoves) {
            cmdManager.addCommand(c);
        }
    }

    private void handleInventaireResponse(JsonObject msg) {
        JsonObject inv = msg.getAsJsonObject("inventaire");
        for (Map.Entry<String, JsonElement> entry : inv.entrySet()) {
            String item = entry.getKey();
            Resource resource = Resource.fromString(item);
            int count = entry.getValue().getAsInt();
            updateInventory(resource, count);
            this.ai.setInventaireChecked(true);
        }
        // for (Map.Entry<String, Integer> entry : this.inventory.entrySet()) {
        //     System.out.println("[CLIENT " + this.id + "] " + entry.getKey() + ": " + entry.getValue());
        // }
    }

    private void handlePrendResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        String item = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        System.out.println("[CLIENT " + this.id + "] " + "Take an object (" + item + ") response: " + status);
        if (status.equals("ok")) {
            Resource resource = Resource.fromString(item);
            addResource(resource);
            if (resource == Resource.NOURRITURE) {
                addLife(126);
                this.nour.addAndGet(1);
                System.out.println("[CLIENT " + this.id + "] " + "Nourritures taken: " + this.nour.get());
            }
            System.out.println("[CLIENT " + this.id + "] " + "New inventory: " + this.inventory + " (life: " + this.life + ")");
        }
    }

    private void handlePoseResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        String item = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        System.out.println("[CLIENT " + this.id + "] " + "Drop an object (" + item + ") response: " + status);
        if (status.equals("ok")) {
            Resource resource = Resource.fromString(item);
            removeResource(resource);
            System.out.println("[CLIENT " + this.id + "] " + "New inventory: " + this.inventory);
        }
    }

    private void handleDieResponse(JsonObject msg) {
        String arg = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        if (arg.equals("die")) {
            System.out.println("[CLIENT " + this.id + "] " + "I AM DEAD :(");
            cmdManager.closeSession();
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

    public Position getPosition() {
        return this.position;
    }

    public Map<Resource, Integer> getInventory() {
        return new ConcurrentHashMap<>(this.inventory); // returns a copy (to be safe)
    }

    public int getInventoryCount(Resource item) {
        return this.inventory.getOrDefault(item, 0);
    }

    /********** SETTERS **********/

    public void setCommandManager(CommandManager commandManager) {
        this.cmdManager = commandManager;
    }

    public void setLevel(int level) {
        this.level.set(level);
    }

    public void incrementLevel() {
        this.level.incrementAndGet();
    }

    public void setLife(int life) {
        this.life.set(life);
    }

    public void addLife(int life) {
        this.life.addAndGet(life);
    }

    public void setGameState(int w, int h, AI ai) {
        this.world = new World(w, h);
        this.position = new Position(w, h);
        this.ai = ai;
        this.ai.setWorld(this.world);
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

    /********** UTILS **********/
}
