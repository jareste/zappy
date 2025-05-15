package com.example;

import java.util.HashMap;
import java.util.Map;
import java.util.EnumMap;

public class ElevationRules {

    public static class Rule {
        public int players;
        public Map<Resource, Integer> stones = new EnumMap<>(Resource.class);

        public Rule(int players, Map<Resource, Integer> stones) {
            this.players = players;
            this.stones = stones;
        }

        public int getPlayers() {
            return this.players;
        }

        public Map<Resource, Integer> getResources() {
            return this.stones;
        }
    }

    private static final Map<Integer, Rule> rules = new HashMap<>();

    static {
        // Level 1 -> 2
        rules.put(1, new Rule(1, Map.of(
                Resource.LINEMATE, 1
        )));
        // Level 2 -> 3
        rules.put(2, new Rule(2, Map.of(
                Resource.LINEMATE, 1,
                Resource.DERAUMERE, 1,
                Resource.SIBUR, 1
        )));
        rules.put(3, new Rule(2, Map.of(
                Resource.LINEMATE, 2,
                Resource.SIBUR, 1,
                Resource.PHIRAS, 2
        )));
        rules.put(4, new Rule(4, Map.of(
                Resource.LINEMATE, 1,
                Resource.DERAUMERE, 1,
                Resource.SIBUR, 2,
                Resource.PHIRAS, 1
        )));
        rules.put(5, new Rule(4, Map.of(
                Resource.LINEMATE, 1,
                Resource.DERAUMERE, 2,
                Resource.SIBUR, 1,
                Resource.MENDIANE, 3
        )));
        rules.put(6, new Rule(6, Map.of(
                Resource.LINEMATE, 1,
                Resource.DERAUMERE, 2,
                Resource.SIBUR, 3,
                Resource.PHIRAS, 1
        )));
        rules.put(7, new Rule(6, Map.of(
                Resource.LINEMATE, 2,
                Resource.DERAUMERE, 2,
                Resource.SIBUR, 2,
                Resource.PHIRAS, 2,
                Resource.MENDIANE, 2,
                Resource.THYSTAME, 1
        )));
    }

    public static Rule getRule(int level) {
        return rules.get(level);
    }
}

/*
How to use:
Rule rule = ElevationRules.getRule(currentLevel);
int playersNeeded = rule.getPlayers();
Map<Resource, Integer> resourcesNeeded = rule.getResources();
*/ 