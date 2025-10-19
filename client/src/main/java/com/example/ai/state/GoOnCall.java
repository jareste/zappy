package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class GoOnCall implements AIState {
    private int direction;
    private boolean hasMovedToCallLocation = false;
    private boolean didActions = false;

    public GoOnCall(int direction) {
        this.direction = direction;
    }

    @Override
    public List<Command> getActions(GameState gameState) {
        List<Command> commands = new ArrayList<>();
        if (direction > 0) {
            didActions = true;
        }
        if (!hasMovedToCallLocation) {
            switch (direction) {
                case 1:
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 2:
                    commands.add(new Command(CommandType.AVANCE));
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 3:
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 4:
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.AVANCE));
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 5:
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.GAUCHE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 6:
                    commands.add(new Command(CommandType.DROITE));
                    commands.add(new Command(CommandType.AVANCE));
                    commands.add(new Command(CommandType.DROITE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 7:
                    commands.add(new Command(CommandType.DROITE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                case 8:
                    commands.add(new Command(CommandType.AVANCE));
                    commands.add(new Command(CommandType.DROITE));
                    commands.add(new Command(CommandType.AVANCE));
                    break;
                default:
                    break;
            }
        } else {
            System.out.println("[CLIENT " + gameState.getPlayer().getId() + "] Arrived at elevation call location, waiting for others ...");
        }
        return commands;
    }

    @Override
    public AIState next(GameState gameState) {
        if (!hasMovedToCallLocation) {
            // Still need to move to call location
            if (didActions) {
                setDirection(-1); // need to wait for new message
            }
            
            return this;
        }
        // Once at location, transition to waiting for others
        return new SearchFood();
    }

    private void setDirection(int dir) {
        this.direction = dir;
    }
}
