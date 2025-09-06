package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;
import com.example.ai.service.*;

import java.util.ArrayList;
import java.util.List;

public class SearchFood implements AIState {

    @Override
    public List<Command> getActions(Player player, View view) {
        List<Command> commands = new ArrayList<>();
        
        int tileIdx = MovementService.findItemInView(Resource.NOURRITURE, view, player.getLevel());
        if (tileIdx != -1) {
            List<CommandType> moves = MovementService.getMovesToTile(tileIdx);
            for (CommandType move : moves) {
                commands.add(new Command(move));
            }
            commands.add(new Command(CommandType.PREND, Resource.NOURRITURE.getName()));
        } else {
            commands.add(new Command(MovementService.getRandomMove()));
        }
        
        commands.add(new Command(CommandType.VOIR));
        return commands; 
    }

    @Override
    public AIState next(Player player, View view) {
        if (player.getNourriture() > 20) {
            return new CheckStatus(); // safe now
        }
        return this; // keep searching
    }

    // private List<Command> searchForFood(Player player, View view) {
    //     List<Command> commands = new ArrayList<>();

    //     int tileIdx = MovementService.findItemInView(Resource.NOURRITURE, view, player.getLevel());
    //     if (tileIdx != -1) {
    //         MovementService.addMovesToTileAndPrend(tileIdx, Resource.NOURRITURE, commands);
    //     } else {
    //         MovementService.addRandomMove(commands);
    //     }
    //     return commands;
    // }
}
