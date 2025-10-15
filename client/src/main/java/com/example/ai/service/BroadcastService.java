package com.example.ai.service;

import com.example.command.*;
import com.example.model.*;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import java.util.Map;
import java.util.HashMap;

public class BroadcastService {

    public static Command createBroadcastCmd(String event, String status, int level, int playersNeeded) { // level - cur level
        JsonObject broadcastMessage = new JsonObject();
        broadcastMessage.addProperty("event", event);
        broadcastMessage.addProperty("status", status);
        if (level > 0) {
            broadcastMessage.addProperty("level", level);
        }
        if (playersNeeded >= 0) {
            broadcastMessage.addProperty("players_needed", playersNeeded);
        }
        return new Command(CommandType.BROADCAST, broadcastMessage.toString());
    }

    public static Map<String, String> parseBroadcastMessage(String rawMsg) {
        Map<String, String> result = new HashMap<>();
        try {
            JsonObject msg = JsonParser.parseString(rawMsg).getAsJsonObject();
            String event = msg.has("event") ? msg.get("event").getAsString() : "unknown";
            String status = msg.has("status") ? msg.get("status").getAsString() : "unknown";
            result.put("event", event);
            result.put("status", status);

            String level = msg.has("level") ? String.valueOf(msg.get("level").getAsInt()) : "-1";
            String playersNeeded = msg.has("players_needed") ? String.valueOf(msg.get("players_needed").getAsInt()) : "-1";
            result.put("level", level);
            result.put("players_needed", playersNeeded);
        } catch (JsonSyntaxException e) {
            System.err.println("Failed to parse broadcast message: " + rawMsg);
            e.printStackTrace();
        }
        return result;
    }

    // public static void handleBroadcastMessage(String rawMsg, int dir) {
    //     try {
    //         JsonObject msg = JsonParser.parseString(rawMsg).getAsJsonObject();
    //         String event = msg.has("event") ? msg.get("event").getAsString() : "unknown";
    //         String status = msg.has("status") ? msg.get("status").getAsString() : "unknown";

    //         if ("elevation".equals(event) && "call".equals(status)) {
    //             int level = msg.get("level").getAsInt();
    //             int playersNeeded = msg.get("players_needed").getAsInt();
    //             System.out.println("[CLIENT " + this.id + "] Received elevation call for level " + level + ", from dir " + dir);
    //             if (level == this.level.get()) {
    //                 System.out.println("[CLIENT " + this.id + "] Elevation call matches my level, preparing incantation ...");
    //                 List<Command> cmds = ai.goToElevationCall(dir, level, playersNeeded);
    //             } else {
    //                 System.out.println("[CLIENT " + this.id + "] Elevation call does not match my level, ignoring.");
    //             }
    //         } else if ("elevation".equals(event) && "ko".equals(status)) {
    //             // System.out.println("[CLIENT " + this.id + "] Received elevation error (ko) for level " + level + ", from dir " + dir);
    //         } else {
    //             System.out.println("[CLIENT " + this.id + "] Unhandled broadcast message: " + rawMsg);
    //         }

    //     } catch (Exception e) {
    //         System.err.println("[CLIENT " + this.id + "] Failed to parse broadcast message: " + rawMsg);
    //         e.printStackTrace();
    //     }
    // }
}
