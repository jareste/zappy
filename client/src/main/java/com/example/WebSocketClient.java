package com.example;

import javax.websocket.*;
import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.*;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;

@ClientEndpoint
public class WebSocketClient {
    private String teamName;
    private int port;
    private String hostname;
    private static final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
    private Session session;
    private CommandManager cmdManager;

    public WebSocketClient(String teamName, int port, String hostname) {
        this.teamName = teamName;
        this.port = port;
        this.hostname = hostname;
    }

    @OnOpen
    public void onOpen(Session session) {
        this.session = session;
        System.out.println("Connected to server");

        Player player = new Player(this.teamName);
        CommandManager cmdManager = new CommandManager(this::send, player);
        player.setCommandManager(cmdManager);
        this.cmdManager = cmdManager;

        // // FOR DEBUG
        // String msg1 = createJsonMessage1();
        // String msg2 = createJsonMessage2();
        // this.cmdManager.handleResponse(msg1);
        // this.cmdManager.handleResponse(msg2);

        // scheduler.scheduleAtFixedRate(() -> {
        //     try {
        //         if (session.isOpen()) {
        //             session.getAsyncRemote().sendText("ping"); 
        //         }
        //     } catch (Exception e) {
        //         e.printStackTrace();
        //     }
        // }, 0, 5, TimeUnit.SECONDS);
    }

    private String createJsonMessage() {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("team", this.teamName);
        jsonMessage.addProperty("message", "Hello from Java client!");

        return jsonMessage.toString();
    }

    private String createJsonMessage1() {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("type", "bienvenue");
        jsonMessage.addProperty("msg", "Whoa! Knock knock, whos there?");

        return jsonMessage.toString();
    }

    private String createJsonMessage2() {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("type", "welcome");
        jsonMessage.addProperty("remaining_clients", 3);
        JsonObject mapSize = new JsonObject();
        mapSize.addProperty("x", 10);
        mapSize.addProperty("y", 10);
        jsonMessage.add("map_size", mapSize);

        return jsonMessage.toString();
    }

    @OnMessage
    public void onMessage(String message) {
        System.out.println("RECEIVED message: " + message);
        try {
            this.cmdManager.handleResponse(message);
        } catch (Exception e) {
            e.printStackTrace();  // You’ll see if it’s crashing quietly
        }
    }

    @OnClose
    public void onClose() {
        System.out.println("Connection closed");
        scheduler.shutdown();  // Clean up the scheduler when the connection is closed
    }

    private void send(String msg) {
        try {
            this.session.getBasicRemote().sendText(msg);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void startConnection() {
        // Build the server URI
        String serverUri = "wss://" + this.hostname + ":" + this.port;
        // String serverUri = "wss://echo.websocket.events";
        System.out.println("Connecting to server: " + serverUri);

        try {
            // Connect to the WebSocket server
            WebSocketContainer container = ContainerProvider.getWebSocketContainer();
            container.connectToServer(this, new URI(serverUri));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
