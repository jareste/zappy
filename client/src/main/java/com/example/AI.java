package com.example;

import java.util.ArrayList;
import java.util.List;

public class AI {
    private String teamName;

    public AI(String teamName) {
        this.teamName = teamName;
    }

    public List<Command> decideNextMoves() {
        List<Command> commands = new ArrayList<>();
        commands.add(new Command("avance")); // always move forward
        return commands;
    }
}