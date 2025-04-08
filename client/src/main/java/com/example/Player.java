package com.example;

// public class Player {
//     private String name;
//     private int score;

//     public Player(String name) {
//         this.name = name;
//         this.score = 0;
//     }

//     public String getName() {
//         return name;
//     }

//     public int getScore() {
//         return score;
//     }

//     // Method to update the score based on the received message
//     public void updateScore(int newScore) {
//         this.score = newScore;
//         System.out.println(name + "'s new score: " + score);
//     }
// }
 // OR
public class Player {
    private String name;
    private String team;
    // private Position position;
    // private List<Resource> resources;

    public Player(String name) {
        this.name = name;
        // this.position = position;
        // this.resources = new ArrayList<>();
    }

    public void move(int x, int y) {
        // Move the player to a new position
        // this.position = new Position(x, y);
    }

    // public void pickUpResource(Resource resource) {
    //     // Add resource to the player's inventory
    //     // this.resources.add(resource);
    // }

    // Getters and setters omitted for brevity
}
