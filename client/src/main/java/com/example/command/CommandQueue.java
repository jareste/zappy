package com.example.command;

import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class CommandQueue {
    private final Queue<Command> queue = new ConcurrentLinkedQueue<>();

    public void add(Command cmd) {
        queue.add(cmd);
    }

    public Command poll() {
        return queue.poll();
    }

    public boolean isEmpty() {
        return queue.isEmpty();
    }

    public int size() {
        return queue.size();
    }

    public void clear() {
        queue.clear();
    }
}