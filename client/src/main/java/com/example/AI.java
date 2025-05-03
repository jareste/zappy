package com.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class AI {
    // private String teamName;
    private final Player player;
    private World world;

    // public AI(String teamName) {
    //     this.teamName = teamName;
    // }

    public AI(Player player) {
        this.player = player;
    }

    /********** DECISION MAKING **********/

    public List<Command> decideNextMoves() {
        List<Command> commands = new ArrayList<>();
        if (player.getLife() < 300) {
            // moveToAndTake("nourriture");
        }

        // set priority ( nourriture, sibur, phiras .. ?)

        // if (needsMoreOf(target)) -> moveToAndTake(target);
        return commands;
    }

    public List<Command> decideNextMovesViewBased(List<List<String>> viewData) {
        List<Command> commands = new ArrayList<>();
        int tileIdx = findItemInView(viewData, "nourriture");
        if (tileIdx != -1) {
            List<CommandType> moves = getMovesToTile(tileIdx);
            System.out.println("MOVES to nourriture: " + moves);
            for (CommandType move : moves) {
                commands.add(new Command(move));
            }
            commands.add(new Command(CommandType.PREND, Resource.NOURRITURE.getName()));
        } else {
            commands.add(new Command(CommandType.AVANCE));
        }
        return commands;
    }

    public List<Command> decideNextMovesRandom() {
        List<Command> commands = new ArrayList<>();
        Random random = new Random();
    
        CommandType[] possibleCommands = {CommandType.AVANCE, CommandType.GAUCHE, CommandType.DROITE, CommandType.VOIR, CommandType.INVENTAIRE, CommandType.PREND, CommandType.POSE};
        // CommandType[] possibleCommands = CommandType.values();
    
        CommandType randomCommand = possibleCommands[random.nextInt(possibleCommands.length)];
        commands.add(new Command(randomCommand));
    
        return commands;
    }

    /********** FIND **********/

    public int findItemInView(List<List<String>> viewData, String item) {
        for (int i = 0; i < viewData.size(); i++) {
            List<String> tileContents = viewData.get(i);
            if (tileContents.contains(item)) {
                return i;
            }
        }
        return -1; // item not found
    }

    public int findClosestItem(String item, Position pos) {
        return -1;
    }

    /********** MOVES **********/

    // tileIdx in the terms of current view from findItemInView()
    public List<CommandType> getMovesToTile(int tileIdx) {
        List<CommandType> moves = new ArrayList<>();

        if (tileIdx <= 0)
            return moves;
        
        int level = 1;
        int leftIdx = 1;
        while (2 * level + 1 < tileIdx) {
            leftIdx += 2 * level + 1;
            level++;
        }

        int center = leftIdx + level;
        int offset = tileIdx - center; // <0 => left, >0 => right
        for (int i = 0; i < level; i++) {
            moves.add(CommandType.AVANCE);
        }
        if (offset < 0) {
            moves.add(CommandType.GAUCHE);
        } else if (offset > 0) {
            moves.add(CommandType.DROITE);
        }
        for (int i = 0; i < Math.abs(offset); i++) {
            moves.add(CommandType.AVANCE);
        }

        return moves;
    }

    /********** UTILS **********/

    // for level=1 this will be:
    // [-1, -1], [0, -1], [1, -1],
    //           [0, 0]
    public List<int[]> generateRelativeOffsets() {
        int level = player.getLevel();
        List<int[]> offsets = new ArrayList<>();
        for (int l = 0; l <= level; l++) {
            for (int i = -l; i <= l; i++) {
                offsets.add(new int[]{i, -l}); // assuming player faces NORTH, adjust later
            }
        }
        return offsets;
    }

    // TODO: implement this directly in prev function
    // for not calling for each tile
    public int[] rotate(int dx, int dy, int direction) {
        switch (direction) {
            case 0: return new int[]{dx, dy};         // ^ NORTH 
            case 1: return new int[]{-dy, dx};        // > EAST
            case 2: return new int[]{-dx, -dy};       // v SOUTH
            case 3: return new int[]{dy, -dx};        // < WEST
            default: throw new IllegalArgumentException("Invalid direction");
        }
    }

    /********** SETTERS **********/

    public void setWorld(World world) {
        this.world = world;
    }
}