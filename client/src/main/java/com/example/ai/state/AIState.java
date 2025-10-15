package com.example.ai.state;

import com.example.model.GameState;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public interface AIState {

    // The actual command to send to the server
    List<Command> getActions(GameState gameState);

    // Decide the next state after this action
    AIState next(GameState gameState);
}
