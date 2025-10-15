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
    public List<Command> getActions(Player player, View view) {
        List<Command> commands = new ArrayList<>();
        int level = player.getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        Map<Resource, Integer> resourcesNeeded = rule.getResources();

        // do pose of each target
        for (Map.Entry<Resource, Integer> entry : resourcesNeeded.entrySet()) {
            Resource resource = entry.getKey();
            int requiredAmount = entry.getValue();
            for (int i = 0; i < requiredAmount; i++) {
                // add pose command for this resource
                commands.add(new Command(CommandType.POSE, resource.getName()));
                System.out.println("[Client " + player.getId() + "] Posing " + resource.getName());
            }
        }
        return commands;
    }

    @Override
    public AIState next(Player player, View view) {
        if (player.getLevel() == 1) {
            return new StartElevation();
        } else {
            return new WaitForOthers(); // TODO: change to WaitForOthers after testing
        }
    }
}
