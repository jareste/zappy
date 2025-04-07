package com.example;

import javax.websocket.*;
import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.*;
import com.google.gson.JsonObject;

@ClientEndpoint
public class WebSocketClient {

    private static final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);

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
        jsonMessage.addProperty("team", "lala");
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

    private static void showUsage() {
        // System.out.println("Usage: mvn exec:java -Dexec.args=\"-n <team> -p <port> [-h <hostname>]\"");
        System.out.println("Usage: ./client -n <team> -p <port> [-h <hostname>]");
        System.out.println("-n <team_name>  : Name of the team (required)");
        System.out.println("-p <port>       : Port number (required)");
        System.out.println("-h <hostname>   : Hostname (default: localhost)");
    }

    private static Map<String, String> parseArguments(String[] args) {
        Map<String, String> arguments = new HashMap<>();
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-n":
                case "-p":
                case "-h":
                    if (i + 1 < args.length) {
                        arguments.put(args[i], args[i + 1]);
                        i++; // Skip next value because it's already processed
                    } else {
                        System.out.println("Error: Missing value for " + args[i]);
                        showUsage();
                        System.exit(1);
                    }
                    break;
                default:
                    System.out.println("Error: Unrecognized argument " + args[i]);
                    showUsage();
                    System.exit(1);
                    break;
            }
        }
        return arguments;
    }

    public static void main(String[] args) {
        String teamName = null;
        int port = -1;
        String hostname = "localhost"; // Default hostname

        // Parse command line arguments
        // System.out.println(args[0]);
        // System.out.println(args.length);
        Map<String, String> arguments = parseArguments(args);

        if (arguments.containsKey("-n")) {
            teamName = arguments.get("-n");
        }
        if (arguments.containsKey("-p")) {
            try {
                port = Integer.parseInt(arguments.get("-p"));
            } catch (NumberFormatException e) {
                System.out.println("Invalid port number.");
                showUsage();
                return;
            }
        }
        if (arguments.containsKey("-h")) {
            hostname = arguments.get("-h");
        }

        // Validate arguments
        if (teamName == null || port == -1) {
            System.out.println("Error: Missing required arguments.");
            showUsage();
            return;
        }

        // Print the parsed arguments
        System.out.println("Team Name: " + teamName);
        System.out.println("Port: " + port);
        System.out.println("Hostname: " + hostname);
        System.out.println("Connecting to server...");

        // Build the server URI
        String serverUri = "wss://" + hostname + ":" + port;
        // String serverUri = "wss://echo.websocket.events";
        System.out.println("Connecting to server: " + serverUri);

        try {
            // Connect to the WebSocket server
            WebSocketContainer container = ContainerProvider.getWebSocketContainer();
            container.connectToServer(WebSocketClient.class, new URI(serverUri));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
