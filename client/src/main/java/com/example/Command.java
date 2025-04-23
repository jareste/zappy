package com.example;

import java.util.Random;

public class Command {
    public String name;
    public String argument;
    private static final Random random = new Random(); // Define the Random instance

    public Command(String name) {
        this.name = name;
        this.argument = "";
        if (name.equals("prend") || name.equals("pose")) {
            String[] possibleArgs = {"nourriture", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
            String randomCommand = possibleArgs[random.nextInt(possibleArgs.length)];
            this.argument = randomCommand;
            System.out.println("Random argument for " + name + ": " + this.argument);
        }
    }

    public Command(String name, String argument) {
        this.name = name;

        if (name.equals("prend") || name.equals("pose")) {
            String[] possibleArgs = {"nourriture", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
            String randomCommand = possibleArgs[random.nextInt(possibleArgs.length)];
            this.argument = randomCommand;
            System.out.println("Random argument for " + name + ": " + this.argument);
        } else {
            this.argument = argument;
        }
    }

    public static int timeUnits(String cmd) {
        switch (cmd) {
            case "avance":
            case "droite":
            case "gauche":
            case "voir":
                return 7;
            case "inventaire":
                return 1;
            case "prend":
            case "pose":
            case "expulse":
            case "broadcast":
                return 7;
            case "incantation":
                return 300;
            case "fork":
                return 42;
            case "connect_nbr":
                return 0;
            default:
                return 0; // Invalid command
        }
    }

    public String getName() {
        return name;
    }

    public String getArgument() {
        return argument;
    }
}
