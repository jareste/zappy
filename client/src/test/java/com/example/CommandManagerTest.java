package com.example;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeEach;
import org.mockito.ArgumentCaptor;

import javax.websocket.Session;
import java.util.function.Consumer;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.Mockito.*;

public class CommandManagerTest {
    private CommandManager manager;
    private Consumer<String> mockSendFunction;
    private Player mockPlayer;
    private Session mockSession;

    @BeforeEach
    public void setUp() {
        mockSendFunction = mock(Consumer.class);
        mockPlayer = mock(Player.class);
        mockSession = mock(Session.class);
        manager = new CommandManager(mockSendFunction, mockPlayer, mockSession, 1);
    }

    @Test
    public void testConstructor() {
        assertNotNull(manager);
        assertEquals(mockPlayer, manager.getPlayer());
        assertEquals(mockSession, manager.getSession());
        assertEquals(1, manager.getId());
        assertEquals(0, manager.getCommandQueue().size());
        assertFalse(manager.isDead());
    }

    @Test
    public void testAddCommand() {
        Command cmd = new Command(CommandType.AVANCE);
        manager.addCommand(cmd);
        assertEquals(1, manager.getCommandQueue().size());
        assertEquals(cmd, manager.getCommandQueue().peek());
    }

    @Test
    public void testSendMsgCallsSendFunction() {
        manager.sendMsg("test");
        verify(mockSendFunction, times(1)).accept("test");
    }

    @Test
    public void testSendCommandWhenAlive() {
        Command cmd = new Command(CommandType.AVANCE);
        manager.sendCommand(cmd);
        verify(mockSendFunction, times(1)).accept(contains("\"cmd\""));
        assertEquals(1, manager.getPendingResponses());
    }

    @Test
    public void testSendCommandWhenDead() {
        manager.setDead(true); 
        Command cmd = new Command(CommandType.AVANCE);
        manager.sendCommand(cmd);
        verify(mockSendFunction, never()).accept(anyString());
    }

    @Test
    public void testCreateCommandJsonMessageWithArg() {
        Command cmd = new Command(CommandType.AVANCE, "arg1");
        String jsonMessage = manager.createCommandJsonMessage(cmd);
        assertTrue(jsonMessage.contains("\"cmd\":\"avance\""));
        assertTrue(jsonMessage.contains("\"arg\":\"arg1\""));
    }

    @Test
    public void testCreateCommandJsonMessageWithoutArg() {
        Command cmd = new Command(CommandType.AVANCE);
        String jsonMessage = manager.createCommandJsonMessage(cmd);
        assertTrue(jsonMessage.contains("\"cmd\":\"avance\""));
        assertFalse(jsonMessage.contains("\"arg\""));
    }

    @Test
    public void testGetters() {
        assertEquals(1, manager.getId());
        assertEquals(mockPlayer, manager.getPlayer());
        assertEquals(mockSession, manager.getSession());
        // assertEquals(0, manager.getPendingResponses());
        // assertFalse(manager.isDead());
        // assertTrue(manager.getCommandQueue().isEmpty());
    }

    @Test
    public void testCloseSessionWhenOpen() throws Exception {
        when(mockSession.isOpen()).thenReturn(true);
        manager.closeSession();
        verify(mockSession).close();
    }

    @Test
    public void testCloseSessionWhenNotOpen() throws Exception {
        when(mockSession.isOpen()).thenReturn(false);
        manager.closeSession();
        verify(mockSession, never()).close();
    }

    @Test
    public void testCloseSessionHandlesException() throws Exception {
        when(mockSession.isOpen()).thenReturn(true);
        doThrow(new RuntimeException("Close failed")).when(mockSession).close();
        
        try {
            manager.closeSession();
        } catch (Exception e) {
            // Exception is expected, but we just want to ensure it doesn't crash
        }
        verify(mockSession).close();
    }
}
