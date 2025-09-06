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

    // public void tick(Player player, World world, CommandQueue queue) {
    //     String action = state.getAction(player, world);
    //     if (action != null) {
    //         queue.add(action);
    //     }
    //     state = state.next(player, world);
    // }

    public List<Command> decideNextMoves() {
        state = state.next(player, view);
        List<Command> commands = state.getActions(player, view);

        return commands;
    }

    // private List<Command> getCommandsFromActions(List<CommandType> actions) {
    //     List<Command> commands = new ArrayList<>();
    //     for (CommandType action : actions) {
    //         Command cmd = new Command(action);
    //         commands.add(cmd);
    //     }
    // }
}
