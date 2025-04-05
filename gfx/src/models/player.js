import ResourceCollection from './resource';
import { PlayerStatus } from './status';

export class Player {
  constructor(data = {}) {
    this.id = data.id || 0;
    this.position = { 
      x: data.position?.x || 0, 
      y: data.position?.y || 0 
    };
    this.orientation = data.orientation || 1;
    this.level = data.level || 1;
    this.team = data.team || '';
    this.inventory = new ResourceCollection(data.inventory);
    this.status = data.status || PlayerStatus.NORMAL;
  }

  static fromJSON(data) {
    return new Player(data);
  }

  isIncantating() {
    return this.status === PlayerStatus.INCANTATION;
  }

  isBroadcasting() {
    return this.status === PlayerStatus.BROADCASTING;
  }
  
  getOrientationLabel() {
    const directions = ['North', 'East', 'South', 'West'];
    return directions[(this.orientation - 1) % 4];
  }
}

export default Player;
