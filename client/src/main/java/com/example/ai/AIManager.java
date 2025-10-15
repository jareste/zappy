package com.example.ai;

import com.example.ai.state.*;
import com.example.model.GameState;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

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
}
