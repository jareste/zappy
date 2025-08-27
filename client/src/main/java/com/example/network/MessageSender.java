package com.example.network;

import com.example.command.*;
import com.example.model.*;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;

public class MessageSender {
    private final WebSocketClient client;

    public MessageSender(WebSocketClient client) {
        this.client = client;
    }

    private void sendToServer(String msg) {
        client.send(msg);
    }

    public void sendLoginMessage(Player player) {
        String loginMessage = createLoginJsonMessage(player);
        sendToServer(loginMessage);
    }

    public void sendCommand(Command command) {
        String cmdStr = createCommandJsonMessage(command);
        sendToServer(cmdStr);
    }

    private String createLoginJsonMessage(Player player) {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("type", "login");
        jsonMessage.addProperty("key", "SOME_KEY");
        jsonMessage.addProperty("role", "player");
        jsonMessage.addProperty("team-name", player.getTeamName());

        return jsonMessage.toString();
    }

    public String createCommandJsonMessage(Command cmd) {
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
}