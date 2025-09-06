package com.example.model;

import java.util.List;
import java.util.ArrayList;

import java.util.List;

public class View {
    private volatile List<List<Resource>> data = new ArrayList<>();

    public View() {}

    public void update(List<List<Resource>> newView) {
        // Build a completely new structure from the response
        this.data = newView;
    }

    public List<List<Resource>> getData() {
        return data; // always latest snapshot
    }

    public List<Resource> getTile(int idx) {
        return data.get(idx);
    }
}
