import ResourceCollection from './resource';

export class Tile {
  constructor(data = {}) {
    this.x = data.x || 0;
    this.y = data.y || 0;
    this.resources = new ResourceCollection(data.resources);
    this.players = data.players ? [...data.players] : [];
    this.eggs = data.eggs ? [...data.eggs] : [];
  }

  static fromJSON(data) {
    return new Tile(data);
  }
}

export class GameMap {
  constructor(data = {}) {
    this.width = data.width || 0;
    this.height = data.height || 0;
    this.tiles = [];

    if (data.tiles && Array.isArray(data.tiles)) {
      this.tiles = data.tiles.map(tileData => new Tile(tileData));
    }
  }

  static fromJSON(data) {
    return new GameMap(data);
  }

  getTileAt(x, y) {
    return this.tiles.find(tile => tile.x === x && tile.y === y) || null;
  }

  getResourceAt(x, y) {
    const tile = this.getTileAt(x, y);
    return tile ? tile.resources : new ResourceCollection();
  }

  getPlayersAt(x, y) {
    const tile = this.getTileAt(x, y);
    return tile ? tile.players : [];
  }
}

export default GameMap;
