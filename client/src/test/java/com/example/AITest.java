package com.example;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import java.lang.reflect.Field;
import java.util.*;
import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

public class AITest {
    private Player mockPlayer;
    private AI ai;

    @BeforeEach
    public void setUp() {
        mockPlayer = mock(Player.class);
        when(mockPlayer.getLevel()).thenReturn(1); // default level
        ai = new AI(mockPlayer);
    }


    @Test
    public void testGetViewIndicesSortedByDistance_Level1() {
        List<Integer> expected = Arrays.asList(0, 2, 1, 3);
        assertEquals(expected, AI.getViewIndicesSortedByDistance(1));
    }

    @Test
    public void testGetViewIndicesSortedByDistance_Level2() {
        List<Integer> expected = Arrays.asList(0, 2, 1, 3, 6, 5, 7, 4, 8);
        assertEquals(expected, AI.getViewIndicesSortedByDistance(2));
    }

    @Test
    public void testFindItemInView_ReturnsCorrectIndex() throws Exception {
        List<List<String>> view = Arrays.asList(
                Collections.emptyList(),                       // 0 (current tile)
                List.of(Resource.NOURRITURE.getName()),        // 1 (left)
                Collections.emptyList(),                       // 2 (centre of row)
                Collections.emptyList()                        // 3 (right)
        );

        // Inject curView via reflection (keep method under test public)
        Field f = AI.class.getDeclaredField("curView");
        f.setAccessible(true);
        f.set(ai, view);

        int idx = ai.findItemInView(Resource.NOURRITURE);
        assertEquals(1, idx);
    }

    @Test
    public void testDecideNextMovesViewBased_LowLife_SearchesFood() {
        when(mockPlayer.getLife()).thenReturn(1000); // critical life

        List<List<String>> view = Arrays.asList(
                Collections.emptyList(),
                List.of(Resource.NOURRITURE.getName()),
                Collections.emptyList(),
                Collections.emptyList()
        );

        List<Command> cmds = ai.decideNextMovesViewBased(view);
        assertFalse(cmds.isEmpty());

        Command last = cmds.get(cmds.size() - 1);
        assertEquals(CommandType.PREND.getName(), last.getName());
        assertEquals(Resource.NOURRITURE.getName(), last.getArgument());
    }

    /*
    9  10 11 12 13 14 15
       4  5  6  7  8
          1  2  3
             0
    */

    @Test
    public void testGetMovesToTile_3() {
        // For level 1, tile index 3 is the right‑most tile in the first visible row.
        List<CommandType> moves = ai.getMovesToTile(3);
        assertTrue(moves.size() >= 2);
        assertEquals(CommandType.AVANCE, moves.get(0)); // move to next row
        assertEquals(CommandType.DROITE, moves.get(1)); // turn right
        assertEquals(CommandType.AVANCE, moves.get(2)); // move forward
    }

    @Test
    public void testGetMovesToTile_4() {
        List<CommandType> moves = ai.getMovesToTile(4);
        assertTrue(moves.size() >= 2);
        assertEquals(CommandType.AVANCE, moves.get(0));
        assertEquals(CommandType.AVANCE, moves.get(1));
        assertEquals(CommandType.GAUCHE, moves.get(2));
        assertEquals(CommandType.AVANCE, moves.get(3));
        assertEquals(CommandType.AVANCE, moves.get(4));
    }

    @Test
    public void testGetMovesToTile_7() {
        List<CommandType> moves = ai.getMovesToTile(7);
        assertTrue(moves.size() >= 2);
        assertEquals(CommandType.AVANCE, moves.get(0));
        assertEquals(CommandType.AVANCE, moves.get(1));
        assertEquals(CommandType.DROITE, moves.get(2));
        assertEquals(CommandType.AVANCE, moves.get(3));
    }

    @Test
    public void testGetMovesToTile_12() {
        List<CommandType> moves = ai.getMovesToTile(12);
        assertTrue(moves.size() == 3);
        assertEquals(CommandType.AVANCE, moves.get(0));
        assertEquals(CommandType.AVANCE, moves.get(1));
        assertEquals(CommandType.AVANCE, moves.get(2));
    }

    @Test
    public void testGetMovesToTile_15() {
        List<CommandType> moves = ai.getMovesToTile(15);
        assertTrue(moves.size() >= 6);
        assertEquals(CommandType.AVANCE, moves.get(0));
        assertEquals(CommandType.AVANCE, moves.get(1));
        assertEquals(CommandType.AVANCE, moves.get(2));
        assertEquals(CommandType.DROITE, moves.get(3));
        assertEquals(CommandType.AVANCE, moves.get(4));
        assertEquals(CommandType.AVANCE, moves.get(5));
        assertEquals(CommandType.AVANCE, moves.get(6));
    }

    @Test
    void testDoElevation_Level1() {
        when(mockPlayer.getId()).thenReturn(1);
        // ai.setDebugLevel(1); // you can add a setter if needed

        List<Command> cmds = ai.doElevation();

        assertEquals(2, cmds.size());
        assertEquals(CommandType.POSE, cmds.get(0).getType());
        assertEquals("linemate", cmds.get(0).getArgument());

        assertEquals(CommandType.INCANTATION, cmds.get(1).getType());
    }
}
