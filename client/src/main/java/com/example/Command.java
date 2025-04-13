package com.example;

public class Command {
    public String name;
    public String argument;

    public Command(String name) {
        this.name = name;
        this.argument = "";
    }

    public Command(String name, String argument) {
        this.name = name;
        this.argument = argument;
    }

    public String getName() {
        return name;
    }

    public String getArgument() {
        return argument;
    }
}
