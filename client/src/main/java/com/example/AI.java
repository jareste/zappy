package com.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class AI {
    // private String teamName;
    private Player player;
    private World world;

    // public AI(String teamName) {
    //     this.teamName = teamName;
    // }

    public AI(Player player) {
        this.player = player;
    }

    /********** DECISION MAKING **********/

    public List<Command> decideNextMovesNew() {
        List<Command> commands = new ArrayList<>();
        if (player.getLife() < 300) {
            // moveToAndTake("nourriture");
            // return;
        }

        // set priority ( nourriture, sibur, phiras .. ?)

        // if (needsMoreOf(target)) -> moveToAndTake(target);
        return commands;
    }

    public List<Command> decideNextMoves() {
        List<Command> commands = new ArrayList<>();
        Random random = new Random();
    
        String[] possibleCommands = {"avance", "gauche", "droite", "connect_nbr", "voir", "inventaire", "prend", "pose", "broadcast", "expulse"};
    
        String randomCommand = possibleCommands[random.nextInt(possibleCommands.length)];
        commands.add(new Command(randomCommand));
    
        return commands;
    }

    /********** SETTERS **********/

    public void setWorld(World world) {
        this.world = world;
    }
}