package com.example.ai.service;

import com.example.command.*;
import com.example.model.*;

import java.util.Random;
import java.util.List;
import java.util.ArrayList;

public class MovementService {

    public static int findItemInView(Resource item, View view, int level) {
        List<Integer> sortedIndices = getViewIndicesSortedByDistance(level);

        for (int i : sortedIndices) {
            List<Resource> tileContents = view.getTile(i);
            if (tileContents.contains(item)) {
                return i;
            }
        }
        return -1; // item not found
    }

    public static CommandType getRandomMove() {
        Random random = new Random();
        CommandType[] possibleMoves = {CommandType.AVANCE, CommandType.GAUCHE, CommandType.DROITE};
        return possibleMoves[random.nextInt(possibleMoves.length)];
    }

    // tileIdx in the terms of current view from findItemInView()
    public static List<CommandType> getMovesToTile(int tileIdx) {
        List<CommandType> moves = new ArrayList<>();

        if (tileIdx <= 0)
            return moves;
        
        int level = 1;
        int leftIdx = 1;
        while (leftIdx + 2 * level + 1 <= tileIdx) {
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

    public static void addRandomMove(List<Command> commands) {
        commands.add(new Command(getRandomMove()));
        commands.add(new Command(CommandType.VOIR));
    }

    public static void addMovesToTileAndPrend(int tileIdx, Resource target, List<Command> commands) {
        List<CommandType> moves = getMovesToTile(tileIdx);
        System.out.println("MOVES to " + target.getName() + "(idx: " + tileIdx + ") : " + moves);
        for (CommandType move : moves) {
            commands.add(new Command(move));
        }
        commands.add(new Command(CommandType.PREND, target.getName()));
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
}
