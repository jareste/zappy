package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.EnumSet;
import java.util.Map;

public class CheckResources implements AIState {

    @Override
    public List<Command> getActions(Player player, View view) {
        List<Command> commands = new ArrayList<>();

        commands.add(new Command(CommandType.INVENTAIRE));

        return commands;
    }

    @Override
    public AIState next(Player player, View view) {
        if (player.getNourriture() < 12) {
            return new SearchFood();
        }
        return this; // TODO: change to elevate (or poseELementss..)
    }
}