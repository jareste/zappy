package com.example;

public enum CommandType {
    AVANCE("avance", 7),
    DROITE("droite", 7),
    GAUCHE("gauche", 7),
    VOIR("voir", 7),
    INVENTAIRE("inventaire", 1),
    PREND("prend", 7),
    POSE("pose", 7),
    EXPULSE("expulse", 7),
    BROADCAST("broadcast", 7),
    INCANTATION("incantation", 300),
    FORK("fork", 42),
    CONNECT_NBR("connect_nbr", 0);

    private final String name;
    private final int timeUnits;

    CommandType(String name, int timeUnits) {
        this.name = name;
        this.timeUnits = timeUnits;
    }

    public String getName() {
        return name;
    }

    public int getTimeUnits() {
        return timeUnits;
    }

    @Override
    public String toString() {
        return name;
    }

    public static CommandType fromName(String name) {
        for (CommandType cmd : values()) {
            if (cmd.name.equals(name)) {
                return cmd;
            }
        }
        throw new IllegalArgumentException("Unknown command: " + name);
    }
}
