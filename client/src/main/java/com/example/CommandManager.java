package com.example;

import java.util.function.Consumer;
import java.util.LinkedList;
import java.util.Queue;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import javax.websocket.Session;
import java.io.IOException;

public class CommandManager {
    // private String teamName;
    private final Consumer<String> sendToServerFunction;
    private Session session;
    private int id;
    private final Queue<Command> commandQueue = new LinkedList<>();
    private final Player player;
    private int pendingResponses = 0;

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

        if (!commandQueue.isEmpty()) {
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
            this.player.setWorld(x, y);
        }
        // send voir command instead:
        sendCommand(new Command("voir"));
        // sendCommand(new Command("avance"));
    }

    private void handleResponseMsg(JsonObject jsonResponse) {
        // System.out.println("Response received");
        pendingResponses--;
        this.player.handleResponse(jsonResponse);
    }

    private void handleBroadcastMsg(JsonObject jsonResponse) {
        int dir = jsonResponse.get("source_direction").getAsInt();
        System.out.println("[CLIENT " + this.id + "] " + "Message received: \"" + jsonResponse.get("arg").getAsString() + "\" from direction: " + dir);
        // handle properly
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
        //     sendCommand(command.name + " " + command.argument);
        //     pendingResponses++;
        // }
    }

    private void sendCommand(Command command) {
        String cmdStr = createCommandJsonMessage(command);
        sendMsg(cmdStr);
        pendingResponses++;
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
