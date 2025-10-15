package com.example.model;

import java.util.List;
import java.util.ArrayList;

import java.util.List;

public class View {
    private volatile List<List<Resource>> data = new ArrayList<>();

    public View() {}

    public void update(List<List<Resource>> newView) {
        if (newView == null) {
            this.data = new ArrayList<>();
        } else {
            List<List<Resource>> copy = new ArrayList<>(newView.size());
            for (List<Resource> tile : newView) {
                copy.add(new ArrayList<>(tile));
            }
            this.data = copy;
        }
    }

    public List<List<Resource>> getData() {
        return data; // always latest snapshot
    }

    public List<Resource> getTile(int idx) {
        if (data == null || idx < 0 || idx >= data.size()) {
            return java.util.Collections.emptyList();
        }
        return data.get(idx);
    }

    public int getCurrentPlayers() {
        if (data == null || data.isEmpty()) {
            return 0;
        }
        List<Resource> firstTile = data.get(0);
        int count = 0;

        for (Resource res : firstTile) {
            if (res == Resource.PLAYER) {
                count++;
            }
        }
         
        return count;
    }
}
