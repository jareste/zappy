package com.example;

import java.util.HashMap;
import java.util.Map;

public class Main {
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

        WebSocketClient webSocketClient = new WebSocketClient(teamName, port, hostname);
        webSocketClient.startConnection();

        try {
            Thread.sleep(Long.MAX_VALUE); // Keep alive forever
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}