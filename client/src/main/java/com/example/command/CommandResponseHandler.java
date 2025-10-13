package com.example.command;

import com.example.model.*;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;

import java.util.List;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.Map;

public class CommandResponseHandler {
    private final CommandManager cmdManager;
    private final Player player;
    private int id;

    public CommandResponseHandler(Player player, CommandManager cmdManager) {
        this.cmdManager = cmdManager;
        this.player = player;
        this.id = player.getId();
    }

    public void handleResponse(JsonObject msg) {
        // msg == jsonResponse from server (from MessageHandler)
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
                handleIncantationResponse(msg);
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

        player.addLife(-CommandType.fromName(cmd).getTimeUnits());
    }

    /********** RESPONSE HANDLERS **********/

    private void handleAvanceResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            player.moveForward();
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + player.getPosition());
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Move failed :(");
        }
    }

    private void handleDroiteResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            player.turnRight();
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + player.getPosition());
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Turn right failed :(");
        }
    }

    private void handleGaucheResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("ok")) {
            player.turnLeft();
            System.out.println("[CLIENT " + this.id + "] " + "New position: " + player.getPosition());
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Turn left failed :(");
        }
    }

    private void handleVoirResponse(JsonObject msg) {
        List<List<Resource>> data = new ArrayList<>();
        JsonArray arr = msg.getAsJsonArray("vision");

        for (JsonElement tile : arr) {
            JsonArray tileArr = tile.getAsJsonArray();
            List<Resource> contents = new ArrayList<>();
            for (JsonElement item : tileArr) {
                String itemStr = item.getAsString();
                Resource resource = Resource.fromString(itemStr);
                if (resource != null) {
                    contents.add(resource);
                } else {
                    System.out.println("[CLIENT " + this.id + "] Unknown resource: " + itemStr);
                }
            }
            data.add(contents);
        }
        for (int i = 0; i < data.size(); i++) {
            System.out.println("[CLIENT " + this.id + "] " + "Tile " + i + ": " + data.get(i));
        }

        player.updateView(data);
    }

    private void handleInventaireResponse(JsonObject msg) {
        JsonObject inv = msg.getAsJsonObject("inventaire");
        System.out.println("[CLIENT " + this.id + "] " + "Inventory response: " + inv);
        for (Map.Entry<String, JsonElement> entry : inv.entrySet()) {
            String item = entry.getKey();
            Resource resource = Resource.fromString(item);
            int count = entry.getValue().getAsInt();
            player.updateInventory(resource, count);
            // this.ai.setInventaireChecked(true);
        }

        // TODO: update nour??

        // if (inv.has(Resource.NOURRITURE.getName())) {
        //     int nourCount = inv.get(Resource.NOURRITURE.getName()).getAsInt();
        //     this.nour.set(nourCount);
        // }
    }

    private void handlePrendResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        String item = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        System.out.println("[CLIENT " + this.id + "] " + "Take an object (" + item + ") response: " + status);
        if (status.equals("ok")) {
            Resource resource = Resource.fromString(item);
            player.addResource(resource);
            if (resource == Resource.NOURRITURE) {
                player.addLife(126);
                // this.nour.addAndGet(1);
                // System.out.println("[CLIENT " + this.id + "] " + "Nourritures taken: " + this.nour.get());
            }
            System.out.println("[CLIENT " + this.id + "] " + "New inventory: " + player.getInventory() + " (life: " + player.getLife() + ")");
        }
    }

    private void handlePoseResponse(JsonObject msg) {
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        String item = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        System.out.println("[CLIENT " + this.id + "] " + "Drop an object (" + item + ") response: " + status);
        if (status.equals("ok")) {
            Resource resource = Resource.fromString(item);
            player.removeResource(resource);
            System.out.println("[CLIENT " + this.id + "] " + "New inventory: " + player.getInventory());
        }
    }

    private void handleIncantationResponse(JsonObject msg) {
        System.out.println("[CLIENT " + this.id + "] " + "INCANTATION!!! response: " + msg);
        String status = msg.has("status") ? msg.get("status").getAsString() : "ko";
        if (status.equals("in_progress")) {
            // cmdManager.incrementPendingResponses();
        } else if (status.equals("Level up!")) {
            // cmdManager.incrementPendingResponses();
            player.incrementLevel();
        } else if (status.equals("ok")) {
            System.out.println("[CLIENT " + this.id + "] " + "Incantation successful! and FINISHED :)");
            // this.ai.setInventaireChecked(false);
        } else {
            System.out.println("[CLIENT " + this.id + "] " + "Incantation failed :(");
        }
    }

    private void handleDieResponse(JsonObject msg) {
        String arg = msg.has("arg") ? msg.get("arg").getAsString() : "null";
        if (arg.equals("die")) {
            System.out.println("[CLIENT " + this.id + "] " + "I AM DEAD :(");
            // cmdManager.closeSession();
            player.setDead(true);
        }
    }
}