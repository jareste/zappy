package com.example.network;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.AI;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;

public class MessageHandler {
    private int id;
    private final Player player;
    private final CommandResponseHandler cmdResponseHandler;
    private final MessageSender msgSender;

    public MessageHandler(Player player, MessageSender msgSender) {
        this.player = player;
        this.id = player.getId();
        this.cmdResponseHandler = new CommandResponseHandler(player);
        this.msgSender = msgSender;
    }

    private JsonObject parseJson(String message) {
        JsonObject jsonMessage = JsonParser.parseString(message).getAsJsonObject();
        return jsonMessage;
    }

    public void handleMessage(String message) {
        JsonObject jsonMessage = parseJson(message);
        String type = jsonMessage.has("type") ? jsonMessage.get("type").getAsString() : "response"; // only for debug

        switch (type) {
            case "bienvenue":
                handleBienvenueMsg(jsonMessage);
                break;
            case "welcome":
                handleWelcomeMsg(jsonMessage);
                break;
            case "response":
                handleResponseMsg(jsonMessage);
                break;
            case "message":
                handleBroadcastMsg(jsonMessage);
                break;
            case "kick":
                handleKickMsg(jsonMessage);
                break;
            case "event":
                handleEventMsg(jsonMessage);
                break;
            case "error":
                handleErrorMsg(jsonMessage);
                break;
            case "cmd": // for debug
                handleResponseMsg(jsonMessage);
                break;
            default:
                System.out.println("[CLIENT " + this.id + "] " + "Unknown message type: " + type);
        }        

        // System.out.println("[CLIENT " + this.id + "] " + "BEFORE SENDING Pending responses: " + pendingResponses.get() + " command queue size: " + commandQueue.size());
        // while (!commandQueue.isEmpty() && pendingResponses.get() < 10) {
        //     Command nextCommand = commandQueue.poll();
        //     sendCommand(nextCommand);
        // }

        // // If there are no pending responses, process the next command
        // if (pendingResponses == 0 && !commandQueue.isEmpty()) {
        //     String nextCommand = commandQueue.poll();
        //     sendMsg(nextCommand);
        //     pendingResponses++;
        // }
    }

    /********** MESSAGE HANDLERS **********/

    private void handleBienvenueMsg(JsonObject jsonMessage) {
        System.out.println("[CLIENT " + this.id + "] " + "BIENVENUE message received: " + jsonMessage.get("msg").getAsString());
        msgSender.sendLoginMessage(player);
    }

    private void handleWelcomeMsg(JsonObject jsonMessage) {
        System.out.println("[CLIENT " + this.id + "] " + "Welcome message received");
        int remaining_clients = jsonMessage.get("remaining_clients").getAsInt();
        System.out.println("[CLIENT " + this.id + "] " + "Remaining clients: " + remaining_clients); // Integer.toString(remaining_clients)
        if (jsonMessage.has("map_size") && jsonMessage.get("map_size").isJsonObject()) {
            JsonObject mapSize = jsonMessage.getAsJsonObject("map_size");
            int x = mapSize.get("x").getAsInt();
            int y = mapSize.get("y").getAsInt();
            // System.out.println("Map size: " + x + "x" + y);

            // TODO: set dimensions and create ai manager ?

            AI ai = new AI(player);
            player.setGameState(x, y, ai);
        }
        // sendCommand(new Command(CommandType.VOIR));
    }

    private void handleResponseMsg(JsonObject jsonMessage) {
        // TODO: decrement pending responses

        // pendingResponses.decrementAndGet();
        cmdResponseHandler.handleResponse(jsonMessage);
    }

    private void handleBroadcastMsg(JsonObject jsonMessage) {
        System.out.println("[CLIENT " + this.id + "] " + "Broadcast message received: " + jsonMessage);
        int dir = jsonMessage.has("status") ? jsonMessage.get("status").getAsInt() : -1; // default to -1 if not present
        String rawMsg = jsonMessage.get("arg").getAsString();
        System.out.println("[CLIENT " + this.id + "] " + "Message received: \"" + rawMsg + "\" from direction: " + dir);
        
        player.handleBroadcastMessage(rawMsg, dir); // !!!???
    }

    private void handleKickMsg(JsonObject jsonMessage) {
        int dir = jsonMessage.get("from_direction").getAsInt();
        System.out.println("[CLIENT " + this.id + "] " + "KICK message received from direction: " + dir);
        // handle properly
    }

    private void handleEventMsg(JsonObject jsonMessage) {
        String status = jsonMessage.has("status") ? jsonMessage.get("status").getAsString() : "unknown";
        if (status.equals("Level up!")) {
            System.out.println("[CLIENT " + this.id + "] " + "Event: LEVEL UP!");
            player.incrementLevel();
        }
    }

    private void handleErrorMsg(JsonObject jsonMessage) {
        String argument = jsonMessage.has("arg") ? jsonMessage.get("arg").getAsString() : "Unknown";
        System.out.println("[CLIENT " + this.id + "] " + "Error received: " + argument);
        // closeSession();
    }
}