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
    public List<Command> getActions(Player player, View view) {
        List<Command> commands = new ArrayList<>();
        int level = player.getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);

        Command broadcastCmd = BroadcastService.createBroadcastCmd("elevation", "call", level, rule.getPlayers());
        commands.add(broadcastCmd);
        return commands; 
    }

    @Override
    public AIState next(Player player, View view) {
        int level = player.getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        int requiredPlayers = rule.getPlayers();
        int currentPlayers = view.getCurrentPlayers();

        if (currentPlayers >= requiredPlayers) {
            return new StartElevation();
        }
        return this; // keep waiting
    }
}
