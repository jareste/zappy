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
            String[] possibleArgs = {"norriture", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
            String randomCommand = possibleArgs[random.nextInt(possibleArgs.length)];
            this.argument = randomCommand;
            System.out.println("Random argument for " + name + ": " + this.argument);
        }
    }

    public Command(String name, String argument) {
        this.name = name;

        if (name.equals("prend") || name.equals("pose")) {
            String[] possibleArgs = {"norriture", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"};
            String randomCommand = possibleArgs[random.nextInt(possibleArgs.length)];
            this.argument = randomCommand;
            System.out.println("Random argument for " + name + ": " + this.argument);
        } else {
            this.argument = argument;
        }
    }

    public String getName() {
        return name;
    }

    public String getArgument() {
        return argument;
    }
}
