import { IncantationStatus } from './status';

export class Event {
  constructor(data = {}) {
    this.type = data.type || '';
    this.position = data.position ? { ...data.position } : null;
    this.playerId = data.player_id;
    this.players = data.players ? [...data.players] : [];
    this.level = data.level;
    this.status = data.status;
    this.sound = data.sound;
    this.resource = data.resource;
  }

  static fromJSON(data) {
    return new Event(data);
  }

  isIncantation() {
    return this.type === 'incantation';
  }

  isPickup() {
    return this.type === 'pickup';
  }

  isDrop() {
    return this.type === 'drop';
  }

  getIncantationStatus() {
    if (!this.isIncantation()) return null;
    return this.status;
  }
}

export default Event;
