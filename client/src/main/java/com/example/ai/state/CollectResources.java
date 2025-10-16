package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.EnumSet;
import java.util.Map;

public class CollectResources implements AIState {

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        Set<Resource> targets = setTargets(gameState);

        List<Integer> sortedIndices = MovementService.getViewIndicesSortedByDistance(gameState.getPlayer().getLevel());
        for (int tileIdx : sortedIndices) {
            for (Resource target : targets) {
                if (gameState.getView().getTile(tileIdx).contains(target)) {
                    System.out.println("[Client "+ gameState.getPlayer().getId() + "] I AM GOING FOR TARGET STONE");
                    MovementService.addMovesToTileAndPrend(tileIdx, target, commands);
                    break;
                }
            }
        }

        if (commands.isEmpty()) {
            commands.add(new Command(MovementService.getRandomMove()));
        }

        commands.add(new Command(CommandType.VOIR));
        return commands;
    }

    @Override
    public AIState next(GameState gameState) {
        Set<Resource> targets = setTargets(gameState);

        if (gameState.getNourriture() < 35) {
            return new SearchFood();
        } else if (readyToElevate(targets)) {
            System.out.println("[Client "+ gameState.getPlayer().getId() + "] I AM READY TO ELEVATE! increasing level to " + (gameState.getPlayer().getLevel() + 1));
            return new PoseResources();
        }
        return this; // keep searching
    }

    private Set<Resource> setTargets(GameState gameState) {
        int level = gameState.getPlayer().getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        Map<Resource, Integer> resourcesNeeded = rule.getResources();
        Set<Resource> targets = EnumSet.noneOf(Resource.class);
        // targets.clear();

        for (Map.Entry<Resource, Integer> entry : resourcesNeeded.entrySet()) {
            Resource resource = entry.getKey();
            int requiredAmount = entry.getValue();
            int currentAmount = gameState.getInventoryCount(resource);
            if (currentAmount < requiredAmount) {
                targets.add(resource);
            }
        }
        return targets;
    }

    private boolean readyToElevate(Set<Resource> targets) {
        if (targets.isEmpty()) {
            return true;
        }
        return false;
    }
}