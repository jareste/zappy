package com.example.ai.state;

import com.example.model.Player;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class SearchFood implements AIState {

    @Override
    public List<Command> getActions(Player player) {
        List<Command> commands = new ArrayList<>();
        Command cmd = new Command(CommandType.AVANCE);
        commands.add(cmd);

        return commands; 
    }

    @Override
    public AIState next(Player player) {
        if (player.getNourriture() > 10) {
            return new CheckStatus(); // safe now
        }
        return this; // keep searching
    }
}
