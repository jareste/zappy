package com.example;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Tile {
    private int nourriture = 0;
    private Map<String, Integer> stones = new HashMap<>();
    private int playerCount = 0;

    public Tile() {
        // init all stones
        for (String stone : List.of("linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame")) {
            stones.put(stone, 0);
        }
    }

    public void update(List<String> contents) {
        // resetting tile
        nourriture = 0;
        playerCount = 0;
        for (String stone : stones.keySet()) {
            stones.put(stone, 0);
        }

        for (String item : contents) {
            switch (item) {
                case "nourriture":
                    nourriture++;
                    break;
                case "player":
                    playerCount++;
                    break;
                default:
                    if (stones.containsKey(item)) {
                        stones.put(item, stones.get(item) + 1);
                    }
                    break;
            }
        }
    }

    public int getResourceCount(String resource) {
        if (resource.equals("nourriture"))
            return nourriture;
        return stones.getOrDefault(resource, 0);
    }

    public int getPlayerCount() {
        return playerCount;
    }
}
