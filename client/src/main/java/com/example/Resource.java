package com.example;

public enum Resource {
    NOURRITURE("nourriture"),
    LINEMATE("linemate"),
    DERAUMERE("deraumere"),
    SIBUR("sibur"),
    MENDIANE("mendiane"),
    PHIRAS("phiras"),
    THYSTAME("thystame"),
    PLAYER("player");

    private final String name;

    Resource(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public static Resource fromString(String s) {
        for (Resource type : values()) {
            if (type.name.equalsIgnoreCase(s)) {
                return type;
            }
        }
        throw new IllegalArgumentException("Unknown resource: " + s);
    }
}
