package com.example.ai;

import com.example.ai.state.*;
import com.example.ai.service.*;
import com.example.model.GameState;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class AIManager {
    private final GameState gameState;
    private AIState state;

    public AIManager(GameState gameState) {
        this.gameState = gameState;
        this.state = new CheckStatus(); // initial state
    }

    public List<Command> decideNextMoves() {
        state = state.next(gameState);
        List<Command> commands = state.getActions(gameState);

        return commands;
    }

    public void handleBroadcastMessage(String rawMsg, int dir) {
        Map<String, String> msgData = BroadcastService.parseBroadcastMessage(rawMsg);
        String event = msgData.getOrDefault("event", "unknown");
        String status = msgData.getOrDefault("status", "unknown");
        int level = Integer.parseInt(msgData.getOrDefault("level", "-1"));
        int playersNeeded = Integer.parseInt(msgData.getOrDefault("players_needed", "-1"));

        if ("elevation".equals(event) && "call".equals(status)) {
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Received elevation call for level " + level + ", from dir " + dir);
            if (level != gameState.getPlayer().getLevel()) {
                System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Elevation call does not match my level, ignoring ...");
                return;
            } else if (state instanceof WaitForOthers) {
                System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Already waiting for others, ignoring new call ...");
                return;
            } else if (dir == 0 && state instanceof GoOnCall) {
                System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Direction is 0, I'm at place ...");
                return;
            } else if (playersNeeded <= 0) {
                System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Players needed is " + playersNeeded + ", no need to go to elevation call.");
                this.state = new CheckStatus();
                return;
            }
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Elevation call matches my level, preparing incantation ...");
            this.state = new GoOnCall(dir);
        } else {
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Received unknown broadcast message: " + rawMsg);
        }
    }
}
