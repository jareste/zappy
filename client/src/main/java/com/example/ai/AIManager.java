package com.example.ai;

import com.example.ai.state.*;
import com.example.model.Player;
import com.example.model.View;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class AIManager {
    private final Player player;
    private final View view;
    private AIState state;

    public AIManager(Player player) {
        this.player = player;
        this.view = player.getView();
        this.state = new CheckStatus(); // initial state
    }

    public List<Command> decideNextMoves() {
        state = state.next(player, view);
        List<Command> commands = state.getActions(player, view);

        return commands;
    }
}
