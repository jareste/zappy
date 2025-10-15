package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.EnumSet;
import java.util.Map;

public class PoseResources implements AIState {

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        int level = gameState.getPlayer().getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        Map<Resource, Integer> resourcesNeeded = rule.getResources();

        // do pose of each target
        for (Map.Entry<Resource, Integer> entry : resourcesNeeded.entrySet()) {
            Resource resource = entry.getKey();
            int requiredAmount = entry.getValue();
            for (int i = 0; i < requiredAmount; i++) {
                // add pose command for this resource
                commands.add(new Command(CommandType.POSE, resource.getName()));
                System.out.println("[Client " + gameState.getPlayer().getId() + "] Posing " + resource.getName());
            }
        }
        return commands;
    }

    @Override
    public AIState next(GameState gameState) {
        if (gameState.getPlayer().getLevel() == 1) {
            return new StartElevation();
        } else {
            return new WaitForOthers(); // TODO: change to WaitForOthers after testing
        }
    }
}
