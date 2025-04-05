export class Team {
  constructor(data = {}) {
    this.name = data.name || '';
    this.playerCount = data.player_count || 0;
    this.remainingConnections = data.remaining_connections || 0;
  }

  static fromJSON(data) {
    return new Team(data);
  }

  getTotalCapacity() {
    return this.playerCount + this.remainingConnections;
  }
}

export default Team;
