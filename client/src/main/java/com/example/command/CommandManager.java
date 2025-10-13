package com.example.command;

import com.example.model.*;
import com.example.network.*;
import com.example.ai.*;

import java.util.function.Consumer;
import java.util.ArrayList;
import java.util.List;
import java.util.LinkedList;
import java.util.Queue;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;
import javax.websocket.Session;
import java.io.IOException;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicBoolean;

public class CommandManager {
    private final CommandResponseHandler cmdResponseHandler;
    private final MessageSender msgSender;
    private AIManager aiManager;
    private Session session;
    private int id;
    private final CommandQueue queue;
    // private final Queue<Command> commandQueue = new ConcurrentLinkedQueue<>();
    private final Player player;
    private final AtomicInteger pendingResponses = new AtomicInteger(0);

    public CommandManager(Player player, MessageSender msgSender, Session session) {
        this.player = player;
        this.session = session;
        this.id = player.getId();
        this.cmdResponseHandler = new CommandResponseHandler(player, this);
        this.msgSender = msgSender;
        this.queue = new CommandQueue();
    }

    public void onBienvenue() {
        msgSender.sendLoginMessage(player);
    }

    public void onWelcome(int x, int y) {
        player.setGameState(x, y);
        this.aiManager = new AIManager(player);
        Command firstCommand = new Command(CommandType.VOIR);
        addToQueue(firstCommand);
    }

    public void onCommandResponse(JsonObject jsonMessage) {
        decrementPendingResponses();
        cmdResponseHandler.handleResponse(jsonMessage);

        if (getPendingResponses() == 0) {
            System.out.println("[CLIENT " + this.id + "] " + "Deciding next moves ...");
            List<Command> nextMoves = aiManager.decideNextMoves();
            addToQueue(nextMoves);
        }
        
    }

    public void onBroadcastMessage(String rawMsg, int dir) {
        player.handleBroadcastMessage(rawMsg, dir);
    }

    public void onLevelUp() {
        player.incrementLevel();
    }

    /********** COMMAND FUNCTIONS **********/

    public void addToQueue(Command command) {
        queue.add(command);
    }

    private void addToQueue(List<Command> commands) {
        for (Command cmd : commands) {
            queue.add(cmd);
        }
    }

    private void sendCommand(Command command) {
        if (player.isDead()) {
            System.out.println("[CLIENT " + this.id + "] " + "Client is dead, cannot send command: " + command);
            return;
        }
        System.out.println("[CLIENT " + this.id + "] " + "Sending command: " + command);
        incrementPendingResponses();
        // System.out.println("[CLIENT " + this.id + "] " + "Pending responses: " + pendingResponses.get());
        msgSender.sendCommand(command);
    }

    public void sendCommandsFromQueue() {
        while (!queue.isEmpty() && getPendingResponses() < 10) {
            Command nextCommand = queue.poll();
            sendCommand(nextCommand);
        }
    }
    /********** GETTERS **********/

    public int getId() {
        return id;
    }

    public Player getPlayer() {
        return player;
    }

    public Session getSession() {
        return session;
    }

    public int getPendingResponses() {
        return pendingResponses.get();
    }

    // public Queue<Command> getCommandQueue() {
    //     return commandQueue;
    // }

    /********** SETTERS **********/

    public void incrementPendingResponses() {
        pendingResponses.incrementAndGet();
    }

    public void decrementPendingResponses() {
        pendingResponses.decrementAndGet();
    }

    /********** UTILS **********/

    public void closeSession() {
        if (this.session != null && this.session.isOpen()) {
            try {
                this.session.close();
            } catch (IOException e) {
                System.err.println("[CLIENT " + this.id + "] " + "Failed to close session: " + e.getMessage());
            }
        }
    }
}
