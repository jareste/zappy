package com.example.model;

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class GameState {
    private final Player player;
    private final View view;
    private final Map<Resource, Integer> inventory;
    private BroadcastMessage lastBroadcast;

    public GameState(String teamName, int playerId) {
        this.player = new Player(teamName, playerId);
        this.view = new View();
        this.inventory = new ConcurrentHashMap<>();
    }

    public GameState(Player player) {
        this.player = player;
        this.view = new View();
        this.inventory = new ConcurrentHashMap<>();
    }

    /********** UPDATERS **********/

    public void updateInventory(Resource item, int count) {
        this.inventory.put(item, count);
    }

    public void updateView(List<List<Resource>> data) {
        view.update(data);
    }

    public void addResource(Resource item) {
        this.inventory.compute(item, (k, v) -> (v == null) ? 1 : v + 1);
    }

    public void removeResource(Resource item) {
        this.inventory.computeIfPresent(item, (k, v) -> (v > 1) ? v - 1 : null);
    }

    /********** GETTERS **********/

    public Player getPlayer() {
        return this.player;
    }

    public Map<Resource, Integer> getInventory() {
        return new ConcurrentHashMap<>(this.inventory); // returns a copy (to be safe)
    }

    public View getView() {
        return this.view;
    }

    public int getInventoryCount(Resource item) {
        return this.inventory.getOrDefault(item, 0);
    }

    public int getNourriture() {
        return getInventoryCount(Resource.NOURRITURE);
    }

    public BroadcastMessage getLastBroadcast() {
        return lastBroadcast;
    }

    /********** SETTERS **********/

    public void setLastBroadcast(BroadcastMessage msg) {
        this.lastBroadcast = msg;
    }
}
