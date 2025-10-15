package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class CheckStatus implements AIState {

    @Override
    public List<Command> getActions(GameState gameState) {
        // No direct action: just deciding next step
        return null;
    }

    @Override
    public AIState next(GameState gameState) {

        return new SearchFood();

        // if (gameState.getPlayer().getLife() < 20) {
        //     return new SearchFood();
        // }
        // if (!gameState.getPlayer().hasStonesForNextLevel()) {
        //     return new CollectResources();
        // }
        // if (gameState.getPlayer().readyToElevate()) {
        //     return new Broadcast();
        // }
        // return new Idle(); // default fallback
    }
}
