package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;
import java.util.ArrayList;
import java.util.List;

public class StartElevation implements AIState {

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        commands.add(new Command(CommandType.INCANTATION));
        return commands;
    }

    @Override
    public AIState next(GameState gameState) {
        return new SearchFood(); // or checkStatus() 
    }
}
