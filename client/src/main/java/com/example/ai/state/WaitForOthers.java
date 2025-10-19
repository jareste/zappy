package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.EnumSet;
import java.util.Map;

public class WaitForOthers implements AIState {

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        int level = gameState.getPlayer().getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);

        System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] WAITING ON POSITION " + gameState.getPlayer().getPosition());
        Command broadcastCmd = BroadcastService.createBroadcastCmd("elevation", "call", level, rule.getPlayers());
        commands.add(broadcastCmd);
        commands.add(new Command(CommandType.VOIR));
        return commands; 
    }

    @Override
    public AIState next(GameState gameState) {
        int level = gameState.getPlayer().getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        int requiredPlayers = rule.getPlayers();
        int currentPlayers = gameState.getView().getCurrentPlayers() + 1;

        if (currentPlayers >= requiredPlayers) {
            return new StartElevation();
        }
        return this; // keep waiting
    }
}
