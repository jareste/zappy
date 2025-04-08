package com.example;

import javax.websocket.*;
import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.*;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

@ClientEndpoint
public class WebSocketClient {
    private String teamName;
    private int port;
    private String hostname;
    private static final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

    public WebSocketClient(String teamName, int port, String hostname) {
        this.teamName = teamName;
        this.port = port;
        this.hostname = hostname;
    }

    @OnOpen
    public void onOpen(Session session) {
        System.out.println("Connected to server");
        // Send the first message to the server immediately
        try {
            session.getBasicRemote().sendText("Hello from Java client!");
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Schedule messages to be sent every 1000ms
        scheduler.scheduleAtFixedRate(() -> {
            try {
                String message = createJsonMessage();
                session.getBasicRemote().sendText(message);
                System.out.println("Sent: " + message);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }, 0, 1000, TimeUnit.MILLISECONDS); // Starts immediately and sends every 1000ms
    }

    private String createJsonMessage() {
        JsonObject jsonMessage = new JsonObject();
        jsonMessage.addProperty("team", this.teamName);
        jsonMessage.addProperty("message", "Hello from Java client!");

        return jsonMessage.toString();
    }

    @OnMessage
    public void onMessage(String message) {
        System.out.println("Received message: " + message);
    }

    @OnClose
    public void onClose() {
        System.out.println("Connection closed");
        scheduler.shutdown();  // Clean up the scheduler when the connection is closed
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
