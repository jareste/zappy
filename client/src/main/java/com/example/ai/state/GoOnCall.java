package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class GoOnCall implements AIState {
    private int direction;
    private boolean hasMovedToCallLocation = false;

    public GoOnCall(int direction) {
        this.direction = direction;
    }

    @Override
    public AIState next(GameState gameState) {
        if (!hasMovedToCallLocation) {
            // Still need to move to call location
            return this;
        }
        // Once at location, transition to waiting for others
        return new SearchFood();
    }

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        if (!hasMovedToCallLocation) {
            // Logic to move towards the call location
            // For simplicity, we assume the player is already at the location
            hasMovedToCallLocation = true;
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Moving to elevation call location");
            // In a real implementation, you would add movement commands here
        } else {
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Arrived at elevation call location, waiting for others ...");
        }
        return commands;
    }
}
