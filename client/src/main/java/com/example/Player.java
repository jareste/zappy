package com.example;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
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
    // private Position position;
    // private List<Resource> resources;

    public Player(String teamName) {
        this.team = teamName;
        this.ai = new AI(teamName);
        this.level = 1;
        // this.world = new World(x, y); 

        // this.name = name;
        // this.position = position;
        // this.resources = new ArrayList<>();
    }

    public void handleResponse(JsonObject msg) {
        String cmd = msg.has("cmd") ? msg.get("cmd").getAsString() : null;
        System.out.println("Handling response of Command: " + cmd);
        // if (cmd == null) {
        //     System.out.println("Command not found in response message.");
        //     return;
        // }
        // String status = msg.has("status") ? msg.get("status").getAsString() : null;

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
                System.out.println("See response: " + msg);
                break;
            case "inventaire":
                System.out.println("Inventory response: " + msg);
                break;
            case "prend":
                System.out.println("Take an object response: " + msg);
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
        // Handle the response for the "avance" command
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            System.out.println("Move successful!");
            // Update player position or state
        } else {
            System.out.println("Move failed :(");
        }
    }

    private void handleDroiteResponse(JsonObject msg) {
        // Handle the response for the "droite" command
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            System.out.println("Turn right successful!");
            // Update player state
        } else {
            System.out.println("Turn right failed :(");
        }
    }

    private void handleGaucheResponse(JsonObject msg) {
        // Handle the response for the "gauche" command
        String status = msg.get("status").getAsString();
        if (status.equals("ok")) {
            System.out.println("Turn left successful!");
            // Update player state
        } else {
            System.out.println("Turn left failed :(");
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

    public void setWorldSize(int x, int y) {
        // Set the world size
        // this.world.setSize(x, y);
        // or this.world = new World(x, y);

        // start the AI decision-making process
    }

    /********** SOMETHING ELSE... **********/

    public void move(int x, int y) {
        // Move the player to a new position
        // this.position = new Position(x, y);
    }

}
