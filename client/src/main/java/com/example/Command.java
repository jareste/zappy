package com.example;

import java.util.Random;

public class Command {
    public CommandType type;
    public String argument;
    private static final Random random = new Random(); // Define the Random instance

    public Command(CommandType type) {
        this.type = type;
        this.argument = "";

        if (type == CommandType.PREND || type == CommandType.POSE) {
            Resource[] possibleArgs = Resource.values(); // reuse your enum!
            Resource randomResource = possibleArgs[random.nextInt(possibleArgs.length)];
            this.argument = randomResource.toString();
            System.out.println("Random argument for " + type + ": " + this.argument);
        }
    }

    public Command(CommandType type, String argument) {
        this.type = type;
        this.argument = argument;
    }

    public int timeUnits() {
        return type.getTimeUnits();
    }

    public String getName() {
        return type.getName();
    }

    public String getArgument() {
        return argument;
    }

    @Override
    public String toString() {
        return argument.isEmpty() ? type.getName() : type.getName() + " " + argument;
    }
}
