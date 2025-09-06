package com.example.ai.state;

import com.example.model.Player;
import com.example.model.View;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public interface AIState {

    // The actual command to send to the server
    List<Command> getActions(Player player, View view);

    // Decide the next state after this action
    AIState next(Player player, View view);
}
