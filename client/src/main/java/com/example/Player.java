package com.example;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import java.util.List;
import java.util.ArrayList;
import java.util.Queue;
import java.util.LinkedList;

public class Player {
    private String name;
    private String team;
    private CommandManager cmdManager;
    private AI ai;
    private int level;
    private World world;
    private Position position;
    // private List<Resource> resources;

    public Player(String teamName) {
        this.team = teamName;
        this.ai = new AI(teamName);
        this.level = 1;
        // this.resources = new ArrayList<>();
    }

    public void handleResponse(JsonObject msg) {
        // msg == jsonResponse from server (from CommandManager)
        String cmd = msg.has("cmd") ? msg.get("cmd").getAsString() : "null";
        System.out.println("Handling response of Command: " + cmd);

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
                System.out.println("Inventory response: " + msg);
                break;
            case "prend":
                String status = msg.has("status") ? msg.get("status").getAsString() : "ko"
                System.out.println("Take an object response: " + status);
                break;
            case "pose":
                String status = msg.has("status") ? msg.get("status").getAsString() : "ko"
                System.out.println("Drop an object response: " + status);
                break;
            case "expulse":
                String status = msg.has("status") ? msg.get("status").getAsString() : "ko"
                System.out.println("Expulse response: " + status);
                break;
            case "broadcast":
                String status = msg.has("status") ? msg.get("status").getAsString() : "ko"
                System.out.println("Broadcast response: " + status);
                break;
            case "incantation":
                System.out.println("Incantation response: " + msg);
                break;
            case "fork":
                String status = msg.has("status") ? msg.get("status").getAsString() : "ko"
                System.out.println("Fork response: " + status);
                break;
            case "connect_nbr":
                int value = msg.has("value") ? msg.get("value").getAsInt() : 0;
                System.out.println("Connect number response: " + value);
                break;
            case "-":
                handleDieResponse(msg);
                break;
            default:
                System.out.println("Not handled (yet) command in response message.");
                break;
        }

        List<Command> nextMoves = ai.decideNextMoves();
        for (Command c : nextMoves) {
            cmdManager.addCommand(c);
        }
    }

    /********** RESPONSE HANDLERS **********/

    private void handleAvanceResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            // System.out.println("Move successful!");
            this.position.moveForward();
            System.out.println("New position: " + this.position);
        } else {
            System.out.println("Move failed :(");
        }
    }

    private void handleDroiteResponse(JsonObject msg) {
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            // System.out.println("Turn right successful!");
            this.position.turnRight();
            System.out.println("New position: " + this.position);
        } else {
            System.out.println("Turn right failed :(");
        }
    }

    private void handleGaucheResponse(JsonObject msg) {
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            // System.out.println("Turn left successful!");
            this.position.turnLeft();
            System.out.println("New position: " + this.position);
        } else {
            System.out.println("Turn left failed :(");
        }
    }

    private void handleVoirResponse(JsonObject msg) {
        System.out.println("See response: " + msg);
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
            System.out.println("Tile " + i + ": " + data.get(i));
        }
        world.updateVisibleTiles(position.getX(), position.getY(), position.getDirection(), level, data);
    }

    private void handleDieResponse(JsonObject msg) {
        String arg = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        if (arg.equals("die")) {
            System.out.println("I AM DEAD :(");
            cmdManager.closeSession();
        }
    }

    /********** GETTERS **********/

    public String getTeamName() {
        return this.team;
    }

    public int getLevel() {
        return this.level;
    }

    /********** SETTERS **********/

    public void setCommandManager(CommandManager commandManager) {
        this.cmdManager = commandManager;
    }

    private void setLevel(int level) {
        this.level = level;
    }

    public void setWorld(int w, int h) {
        this.world = new World(w, h);
        this.position = new Position(w, h);
    }

    /********** UTILS **********/
}
