package com.example;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;

public class Main {
    private static void showUsage() {
        // System.out.println("Usage: mvn exec:java -Dexec.args=\"-n <team> -p <port> [-h <hostname>]\"");
        System.out.println("Usage: ./client -n <team> -p <port> [-h <hostname>]");
        System.out.println("-n <team_name>  : Name of the team (required)");
        System.out.println("-p <port>       : Port number (required)");
        System.out.println("-h <hostname>   : Hostname (default: localhost)");
        System.out.println("-c <client_cnt> : Number of clients to run (default: 1)");
    }

    private static Map<String, String> parseArguments(String[] args) {
        Map<String, String> arguments = new HashMap<>();
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-n":
                case "-p":
                case "-h":
                case "-c":
                    if (i + 1 < args.length) {
                        arguments.put(args[i], args[i + 1]);
                        i++;
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
        int clientCount = 1; // Default client count

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
        if (arguments.containsKey("-c")) {
            try {
                clientCount = Integer.parseInt(arguments.get("-c"));
            } catch (NumberFormatException e) {
                System.out.println("Invalid client count.");
                showUsage();
                return;
            }
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

        // CountDownLatch latch = new CountDownLatch(1);
        // WebSocketClient webSocketClient = new WebSocketClient(teamName, port, hostname, latch);

        // // Registering shutdown hook
        // Runtime.getRuntime().addShutdownHook(new Thread(() -> {
        //     System.out.println("Shutdown signal received. Closing connection...");
        //     webSocketClient.close();
        // }));

        // webSocketClient.startConnection();

        // try {
        //     latch.await(); // Blocks until latch is counted down
        //     System.out.println("Shutting down gracefully.");
        // } catch (InterruptedException e) {
        //     e.printStackTrace();
        // }

        final String finalTeamName = teamName;
        final int finalPort = port;
        final String finalHostname = hostname;

        for (int i = 0; i < clientCount; i++) {
            int clientId = i + 1; // for clarity in logs
            new Thread(() -> {
                CountDownLatch latch = new CountDownLatch(1);
                WebSocketClient webSocketClient = new WebSocketClient(finalTeamName, finalPort, finalHostname, latch, clientId);

                Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                    System.out.println("[CLIENT " + clientId + "] " + "Shutdown signal received. Closing connection...");
                    webSocketClient.close();
                }));

                // System.out.println("Client " + clientId + " connecting...");
                webSocketClient.startConnection();

                try {
                    latch.await();
                    // System.out.println("Client " + clientId + " finished.");
                } catch (InterruptedException e) {
                    System.out.println("[CLIENT " + clientId + "] " + "Interrupted while waiting for latch.");
                    e.printStackTrace();
                }
            }).start();
        }
    }
}