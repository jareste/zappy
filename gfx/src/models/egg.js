import { EggStatus } from './status';

export class Egg {
  constructor(data = {}) {
    this.id = data.id || 0;
    this.position = { 
      x: data.position?.x || 0, 
      y: data.position?.y || 0 
    };
    this.status = data.status || EggStatus.LAID;
    this.parentId = data.parent_id || null;
    this.team = data.team || '';
  }

  static fromJSON(data) {
    return new Egg(data);
  }

  isLaid() {
    return this.status === EggStatus.LAID;
  }

  isHatching() {
    return this.status === EggStatus.HATCHING;
  }

  isHatched() {
    return this.status === EggStatus.HATCHED;
  }
}

export default Egg;
