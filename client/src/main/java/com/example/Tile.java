package com.example;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class Tile {
    private final Map<Resource, AtomicInteger> resources = new ConcurrentHashMap<>();

    public Tile() {
        for (Resource resource : Resource.values()) {
            resources.put(resource, new AtomicInteger(0));
        }
    }
    // private final AtomicInteger nourriture = new AtomicInteger(0);
    // private Map<String, Integer> stones = new ConcurrentHashMap<>();
    // private final AtomicInteger playerCount = new AtomicInteger(0);

    // public Tile() {
    //     // init all stones
    //     for (String stone : List.of("linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame")) {
    //         this.stones.put(stone, 0);
    //     }
    // }

    public synchronized void update(List<String> contents) {
        for (AtomicInteger count : resources.values()) {
            count.set(0);
        }

        for (String item : contents) {
            Resource resource = Resource.fromString(item);
            if (resource != null) {
                resources.get(resource).incrementAndGet();
            // } else if (item.equals("player")) {
            //     resources.get(ResourceType.PLAYER).incrementAndGet();
            }
        }
    }

    // public synchronized void update(List<String> contents) {
    //     // resetting tile
    //     setNourriture(0);
    //     setPlayerCount(0);
    //     for (String stone : stones.keySet()) {
    //         this.stones.put(stone, 0);
    //     }

    //     for (String item : contents) {
    //         switch (item) {
    //             case "nourriture":
    //                 this.nourriture.incrementAndGet();
    //                 break;
    //             case "player":
    //                 this.playerCount.incrementAndGet();
    //                 break;
    //             default:
    //                 if (this.stones.containsKey(item)) {
    //                     this.stones.put(item, stones.get(item) + 1);
    //                 }
    //                 break;
    //         }
    //     }
    // }

    public int getResourceCount(Resource resource) {
        AtomicInteger count = resources.get(resource);
        return count != null ? count.get() : 0;
    }

    public int getPlayerCount() {
        return getResourceCount(Resource.PLAYER);
    }

    public void setPlayerCount(int count) {
        AtomicInteger playerCounter = resources.get(Resource.PLAYER);
        if (playerCounter != null) {
            playerCounter.set(count);
        }
    }

    public void setNourriture(int count) {
        AtomicInteger nourritureCounter = resources.get(Resource.NOURRITURE);
        if (nourritureCounter != null) {
            nourritureCounter.set(count);
        }
    }

    // public int getResourceCount(String resource) {
    //     if (resource.equals("nourriture"))
    //         return this.nourriture.get();
    //     return this.stones.getOrDefault(resource, 0);
    // }

    // public int getPlayerCount() {
    //     return this.playerCount.get();
    // }

    // public void setPlayerCount(int count) {
    //     this.playerCount.set(count);
    // }

    // public void setNourriture(int count) {
    //     this.nourriture.set(count);
    // }

    public void printTile() {
        System.out.println("---- Tile Contents ----");
        for (Resource resource : Resource.values()) {
            int count = getResourceCount(resource);
            if (count > 0) {
                System.out.printf("%s: %d\n", resource.getName(), count);
            }
        }
        System.out.println("------------------------");
    }
}
