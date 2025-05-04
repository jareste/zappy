package com.example;

import java.util.function.Consumer;
import java.util.LinkedList;
import java.util.Queue;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import javax.websocket.Session;
import java.io.IOException;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicBoolean;

public class CommandManager {
    // private String teamName;
    private final Consumer<String> sendToServerFunction;
    private Session session;
    private int id;
    private final Queue<Command> commandQueue = new ConcurrentLinkedQueue<>();
    private final Player player;
    private final AtomicInteger pendingResponses = new AtomicInteger(0);
    private final AtomicBoolean dead = new AtomicBoolean(false);

    public CommandManager(Consumer<String> sendFunction, Player player, Session session, int id) {
        this.sendToServerFunction = sendFunction;
        this.player = player;
        this.session = session;
        this.id = id;
    }

    public void sendMsg(String msg) {
        sendToServerFunction.accept(msg);
    }

    public void handleResponse(String response) {
        JsonObject jsonResponse = parseJson(response);
        String type = jsonResponse.has("type") ? jsonResponse.get("type").getAsString() : "response"; // only for debug

        switch (type) {
            case "bienvenue":
                handleBienvenueMsg(jsonResponse);
                break;
            case "welcome":
                handleWelcomeMsg(jsonResponse);
                break;
            case "response":
                handleResponseMsg(jsonResponse);
                break;
            case "broadcast":
                handleBroadcastMsg(jsonResponse);
                break;
            case "kick":
                handleKickMsg(jsonResponse);
                break;
            case "event":
                handleEventMsg(jsonResponse);
                break;
            case "error":
                handleErrorMsg(jsonResponse);
                break;
            case "cmd": // for debug
                this.player.handleResponse(jsonResponse);
                break;
            default:
                System.out.println("[CLIENT " + this.id + "] " + "Unknown message type: " + type);
        }        

        // System.out.println("[CLIENT " + this.id + "] " + "BEFORE SENDING Pending responses: " + pendingResponses.get() + " command queue size: " + commandQueue.size());
        while (!commandQueue.isEmpty() && pendingResponses.get() < 10) {
            Command nextCommand = commandQueue.poll();
            sendCommand(nextCommand);
        }

        // // If there are no pending responses, process the next command
        // if (pendingResponses == 0 && !commandQueue.isEmpty()) {
        //     String nextCommand = commandQueue.poll();
        //     sendMsg(nextCommand);
        //     pendingResponses++;
        // }
    }

    /********** MESSAGE HANDLERS **********/

    private void handleBienvenueMsg(JsonObject jsonResponse) {
        System.out.println("[CLIENT " + this.id + "] " + "BIENVENUE message received: " + jsonResponse.get("msg").getAsString());
        String loginMessage = createLoginJsonMessage();
        sendMsg(loginMessage);
    }

    private void handleWelcomeMsg(JsonObject jsonResponse) {
        System.out.println("[CLIENT " + this.id + "] " + "Welcome message received");
        int remaining_clients = jsonResponse.get("remaining_clients").getAsInt();
        System.out.println("[CLIENT " + this.id + "] " + "Remaining clients: " + remaining_clients); // Integer.toString(remaining_clients)
        if (jsonResponse.has("map_size") && jsonResponse.get("map_size").isJsonObject()) {
            JsonObject mapSize = jsonResponse.getAsJsonObject("map_size");
            int x = mapSize.get("x").getAsInt();
            int y = mapSize.get("y").getAsInt();
            // System.out.println("Map size: " + x + "x" + y);
            AI ai = new AI(this.player);
            this.player.setGameState(x, y, ai);
        }
        // send voir command instead:
        sendCommand(new Command(CommandType.VOIR));
        // sendCommand(new Command(CommandType.AVANCE));
    }

    private void handleResponseMsg(JsonObject jsonResponse) {
        // System.out.println("Response received");
        pendingResponses.decrementAndGet();
        this.player.handleResponse(jsonResponse);
    }

    private void handleBroadcastMsg(JsonObject jsonResponse) {
        int dir = jsonResponse.get("source_direction").getAsInt();
        String rawMsg = jsonResponse.get("arg").getAsString();
        System.out.println("[CLIENT " + this.id + "] " + "Message received: \"" + rawMsg + "\" from direction: " + dir);
        
        try {
            JsonObject msg = JsonParser.parseString(rawMsg).getAsJsonObject();
            String event = msg.get("event").getAsString();
            String status = msg.get("status").getAsString();

            if ("elevation".equals(event) && "call".equals(status)) {
                int level = msg.get("level").getAsInt();
                int playersNeeded = msg.get("players_needed").getAsInt();
                System.out.println("[CLIENT " + this.id + "] Received elevation call for level " + level + ", from dir " + dir);
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

    private void handleKickMsg(JsonObject jsonResponse) {
        int dir = jsonResponse.get("from_direction").getAsInt();
        System.out.println("[CLIENT " + this.id + "] " + "KICK message received from direction: " + dir);
        // handle properly
    }

    private void handleEventMsg(JsonObject jsonResponse) {
        String event = jsonResponse.get("event").getAsString();
        System.out.println("[CLIENT " + this.id + "] " + "Event received: " + event);
        // handle properly
    }

    private void handleErrorMsg(JsonObject jsonResponse) {
        String argument = jsonResponse.has("arg") ? jsonResponse.get("arg").getAsString() : "Unknown";
        System.out.println("[CLIENT " + this.id + "] " + "Error received: " + argument);
        closeSession();
    }

    /********** JSON FUNCTIONS **********/

    private JsonObject parseJson(String message) {
        JsonObject jsonMessage = JsonParser.parseString(message).getAsJsonObject();
        return jsonMessage;
    }

    private String createLoginJsonMessage() {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("type", "login");
        jsonMessage.addProperty("key", "SOME_KEY");
        jsonMessage.addProperty("role", "player");
        jsonMessage.addProperty("team-name", this.player.getTeamName());

        return jsonMessage.toString();
    }

    /********** COMMAND FUNCTIONS **********/

    public void addCommand(Command command) {
        commandQueue.add(command);
        // if (pendingResponses == 0) {
        //     sendCommand(command);
        //     pendingResponses++;
        // }
    }

    private void sendCommand(Command command) {
        if (isDead()) {
            System.out.println("[CLIENT " + this.id + "] " + "Client is dead, cannot send command: " + command);
            return;
        }
        System.out.println("[CLIENT " + this.id + "] " + "Sending command: " + command);
        String cmdStr = createCommandJsonMessage(command);
        pendingResponses.incrementAndGet();
        // System.out.println("[CLIENT " + this.id + "] " + "Pending responses: " + pendingResponses.get());
        sendMsg(cmdStr);
    }

    private String createCommandJsonMessage(Command cmd) {
        JsonObject jsonMessage = new JsonObject();
        String commandName = cmd.getName();
        String argument = cmd.getArgument();

        jsonMessage.addProperty("type", "cmd");
        jsonMessage.addProperty("cmd", commandName);
        if (argument != null && !argument.isEmpty()) {
            jsonMessage.addProperty("arg", argument);
        }

        return jsonMessage.toString();
    }

    /********** GETTERS **********/

    public int getPendingResponses() {
        return pendingResponses.get();
    }

    public boolean isDead() {
        return dead.get();
    }

    /********** SETTERS **********/

    public void setDead(boolean value) {
        dead.set(value);
    }

    /********** UTILS **********/

    public void closeSession() {
        if (this.session != null && this.session.isOpen()) {
            try {
                this.session.close();
            } catch (IOException e) {
                System.err.println("[CLIENT " + this.id + "] " + "Failed to close session: " + e.getMessage());
            }
        }
    }
}
