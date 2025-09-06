package com.example.ai.state;

import com.example.model.*;
import com.example.command.*;

import java.util.ArrayList;
import java.util.List;

public class CheckStatus implements AIState {

    @Override
    public List<Command> getActions(Player player, View view) {
        // No direct action: just deciding next step
        return null;
    }

    @Override
    public AIState next(Player player, View view) {

        return new SearchFood();

        // if (player.getLife() < 20) {
        //     return new SearchFood();
        // }
        // if (!player.hasStonesForNextLevel()) {
        //     return new CollectResources();
        // }
        // if (player.readyToElevate()) {
        //     return new Broadcast();
        // }
        // return new Idle(); // default fallback
    }
}
