package com.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class AI {
    private String teamName;

    public AI(String teamName) {
        this.teamName = teamName;
    }

    public List<Command> decideNextMoves() {
        List<Command> commands = new ArrayList<>();
        Random random = new Random();
    
        String[] possibleCommands = {"avance", "gauche", "droite", "connect_nbr", "voir", "inventaire", "prend", "pose", "broadcast", "expulse"};
    
        String randomCommand = possibleCommands[random.nextInt(possibleCommands.length)];
        commands.add(new Command(randomCommand));
    
        return commands;
    }
}