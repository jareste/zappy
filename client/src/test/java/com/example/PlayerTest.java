package com.example;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

public class PlayerTest {

    @Test
    public void testBroadcastCommandCreation() {
        Player player = new Player("team", 1);
        Command command = player.broadcast("elevation", "call", "4", "3");

        // Assert command type
        assertEquals(CommandType.BROADCAST, command.getType());

        // Parse argument as JSON and validate contents
        JsonObject json = JsonParser.parseString(command.getArgument()).getAsJsonObject();
        assertEquals("elevation", json.get("event").getAsString());
        assertEquals("call", json.get("status").getAsString());
        assertEquals("4", json.get("level").getAsString());
        assertEquals("3", json.get("players_needed").getAsString());
    }

    @Test
    public void testBroadcastWithoutOptionalFields() {
        Player player = new Player("team", 1);
        Command command = player.broadcast("elevation", "ko", null, null);

        JsonObject json = JsonParser.parseString(command.getArgument()).getAsJsonObject();
        assertEquals("elevation", json.get("event").getAsString());
        assertEquals("ko", json.get("status").getAsString());

        // Check that optional fields are not present
        assertFalse(json.has("level"));
        assertFalse(json.has("players_needed"));
    }
}
