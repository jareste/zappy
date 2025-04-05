import { GameMap } from './map';
import Player from './player';
import Egg from './egg';
import Team from './team';
import Event from './event';
import Broadcast from './broadcast';

export class Game {
  constructor(data = {}) {
    this.map = new GameMap(data.map || {});
    this.players = [];
    this.eggs = [];
    this.broadcasts = [];
    this.events = [];
    this.gameInfo = {
      tick: data.game?.tick || 0,
      timeUnit: data.game?.time_unit || 100,
      teams: []
    };

    if (data.players && Array.isArray(data.players)) {
      this.players = data.players.map(playerData => Player.fromJSON(playerData));
    }

    if (data.eggs && Array.isArray(data.eggs)) {
      this.eggs = data.eggs.map(eggData => Egg.fromJSON(eggData));
    }

    if (data.broadcasts && Array.isArray(data.broadcasts)) {
      this.broadcasts = data.broadcasts.map(broadcastData => Broadcast.fromJSON(broadcastData));
    }

    if (data.events && Array.isArray(data.events)) {
      this.events = data.events.map(eventData => Event.fromJSON(eventData));
    }

    if (data.game?.teams && Array.isArray(data.game.teams)) {
      this.gameInfo.teams = data.game.teams.map(teamData => Team.fromJSON(teamData));
    }
  }

  static fromJSON(data) {
    return new Game(data);
  }

  getPlayer(id) {
    return this.players.find(player => player.id === id) || null;
  }

  getPlayersInTeam(teamName) {
    return this.players.filter(player => player.team === teamName);
  }

  getEgg(id) {
    return this.eggs.find(egg => egg.id === id) || null;
  }

  getTeam(name) {
    return this.gameInfo.teams.find(team => team.name === name) || null;
  }

  update(newGameData) {
    // Create a new game object from the new data
    const newGame = Game.fromJSON(newGameData);
    
    // Update current game with new data
    this.map = newGame.map;
    this.players = newGame.players;
    this.eggs = newGame.eggs;
    this.broadcasts = newGame.broadcasts;
    this.events = newGame.events;
    this.gameInfo = newGame.gameInfo;
    
    return this;
  }
}

export default Game;
