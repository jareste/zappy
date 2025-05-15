package com.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.EnumSet;
import java.util.Map;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;

public class AI {
    // private String teamName;
    private final Player player;
    private World world;
    private Set<Resource> targets = EnumSet.noneOf(Resource.class);
    private List<List<String>> curView = new ArrayList<>();
    private int debugLevel = 1;
    private boolean inventaireChecked = false;

    public AI(Player player) {
        this.player = player;
    }

    /********** DECISION MAKING **********/

    public List<Command> decideNextMoves() {
        List<Command> commands = new ArrayList<>();
        addRandomMove(commands);
        return commands;
    }

    public List<Command> decideNextMovesViewBased(List<List<String>> viewData) {
        this.curView = viewData;
        // List<Command> commands = new ArrayList<>();
        setTargets();

        // int tileIdx = findItemInView(Resource.NOURRITURE);
        // if (tileIdx != -1) {
        //     List<CommandType> moves = getMovesToTile(tileIdx);
        //     System.out.println("MOVES to nourriture: " + moves);
        //     for (CommandType move : moves) {
        //         commands.add(new Command(move));
        //     }
        //     commands.add(new Command(CommandType.PREND, Resource.NOURRITURE.getName()));
        // } else {
        //     commands.add(new Command(CommandType.AVANCE));
        // }

        // return decideNextMovesRandom();

        if (player.getLife() < 1500) {
            System.out.println("[Client "+ player.getId() + "] I AM GOING FOR FOOD");
            return searchForFood();
        }
        if (inventaireChecked && !readyToElevate()) {
            setInventaireChecked(false);
        }
        if (!readyToElevate()) {
            System.out.println("[Client "+ player.getId() + "] I AM GOING FOR TARGET STONE");
            return searchForTarget();
        } else if (!inventaireChecked) {
            System.out.println("[Client "+ player.getId() + "] I AM GOING TO ELEVATE, CHECKING INVENTAIRE");
            return checkInventaire();
        }
        // player.setLevel(player.getLevel() + 1);
        debugLevel++;
        System.out.println("[Client "+ player.getId() + "] I AM READY TO ELEVATE! increasing level to " + debugLevel);
        return checkInventaire();
        

        // return commands();
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

    /********** ELEVATION **********/

    private boolean readyToElevate() {
        if (targets.isEmpty()) {
            return true;
        }
        return false;
    }

    private List<Command> doElevation() {
        List<Command> commands = new ArrayList<>();
        // commands.add(new Command(CommandType.INCANTATION));
        return commands;
    }

    private List<Command> searchForTarget() {
        List<Command> commands = new ArrayList<>();

        // for (Resource target : targets) {
        //     int tileIdx = findItemInView(curView, target.getName());
        //     if (tileIdx != -1) {
        //         addMovesToTile(tileIdx, target, commands);
        //         return commands;
        //     }
        // }
        List<Integer> sortedIndices = getViewIndicesSortedByDistance(player.getLevel());
        for (int tileIdx : sortedIndices) {
            for (Resource target : targets) {
                if (curView.get(tileIdx).contains(target.getName())) {
                    addMovesToTile(tileIdx, target, commands);
                    return commands;
                }
            }
        }

        addRandomMove(commands);
        return commands;
    }

    private List<Command> checkInventaire() {
        List<Command> commands = new ArrayList<>();
        commands.add(new Command(CommandType.INVENTAIRE));
        return commands;
    }

    private void setTargets() {
        int level = debugLevel; //player.getLevel();
        ElevationRules.Rule rule = ElevationRules.getRule(level);
        Map<Resource, Integer> resourcesNeeded = rule.getResources();
        // Set<Resource> targets = new EnumSet<>();
        targets.clear();

        for (Map.Entry<Resource, Integer> entry : resourcesNeeded.entrySet()) {
            Resource resource = entry.getKey();
            int requiredAmount = entry.getValue();
            int currentAmount = player.getInventoryCount(resource);
            if (currentAmount < requiredAmount) {
                targets.add(resource); // add to targets
            }
        }
        // return targets;
    }

    /********** FIND **********/

    private List<Command> searchForFood() {
        List<Command> commands = new ArrayList<>();

        int tileIdx = findItemInView(Resource.NOURRITURE);
        if (tileIdx != -1) {
            addMovesToTile(tileIdx, Resource.NOURRITURE, commands);
        } else {
            addRandomMove(commands);
        }
        return commands;
    }

    public int findItemInView(Resource item) {
        List<Integer> sortedIndices = getViewIndicesSortedByDistance(player.getLevel());

        for (int i : sortedIndices) {
            List<String> tileContents = curView.get(i);
            if (tileContents.contains(item.getName())) {
                return i;
            }
        }
        return -1; // item not found
    }

    public int findClosestItem(String item, Position pos) {
        return -1;
    }

    /********** MOVES **********/

    private CommandType getRandomMove() {
        Random random = new Random();
        CommandType[] possibleMoves = {CommandType.AVANCE, CommandType.GAUCHE, CommandType.DROITE};
        return possibleMoves[random.nextInt(possibleMoves.length)];
    }

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

    private void addMovesToTile(int tileIdx, Resource target, List<Command> commands) {
        List<CommandType> moves = getMovesToTile(tileIdx);
        System.out.println("MOVES to " + target.getName() + ": " + moves);
        for (CommandType move : moves) {
            commands.add(new Command(move));
        }
        commands.add(new Command(CommandType.PREND, target.getName()));
    }

    private void addRandomMove(List<Command> commands) {
        commands.add(new Command(getRandomMove()));
        commands.add(new Command(CommandType.VOIR));
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

    public static List<Integer> getViewIndicesSortedByDistance(int level) {
        List<Integer> result = new ArrayList<>();
        int index = 0;
        result.add(index++); // always start with tile 0 (your current tile)

        for (int l = 1; l <= level; l++) {
            int rowSize = 2 * l + 1;
            int rowStart = index;
            int center = rowStart + rowSize / 2;

            result.add(center);

            for (int offset = 1; offset <= rowSize / 2; offset++) {
                result.add(center - offset); // left
                result.add(center + offset); // right
            }

            index += rowSize; // move to next row for next level
        }

        return result;
    }

    private String createBroadcastMessage(String event, String status, int level, int playersNeeded) {
        JsonObject msg = new JsonObject();
        msg.addProperty("event", event);
        msg.addProperty("status", status);
        msg.addProperty("level", level);
        msg.addProperty("players_needed", playersNeeded);

        return msg.toString();
    }

    /********** SETTERS **********/

    public void setWorld(World world) {
        this.world = world;
    }

    public void setInventaireChecked(boolean inventaireChecked) {
        this.inventaireChecked = inventaireChecked;
    }
}