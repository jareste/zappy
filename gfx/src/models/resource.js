export class Resource {
  constructor(type = '', amount = 0) {
    this.type = type;
    this.amount = amount;
  }

  static fromJSON(type, amount) {
    return new Resource(type, amount);
  }

  isEmpty() {
    return this.amount === 0;
  }
}

export class ResourceCollection {
  constructor(data = {}) {
    this.nourriture = data.nourriture || 0;
    this.linemate = data.linemate || 0;
    this.deraumere = data.deraumere || 0;
    this.sibur = data.sibur || 0;
    this.mendiane = data.mendiane || 0;
    this.phiras = data.phiras || 0;
    this.thystame = data.thystame || 0;
  }

  static fromJSON(data) {
    return new ResourceCollection(data);
  }

  isEmpty() {
    return Object.values(this).every(value => value === 0);
  }

  getTotalCount() {
    return Object.values(this).reduce((sum, value) => sum + value, 0);
  }

  getAsArray() {
    return Object.entries(this).map(([type, amount]) => 
      new Resource(type, amount)
    ).filter(resource => !resource.isEmpty());
  }

  getResourceAmount(type) {
    return this[type] || 0;
  }
}

export default ResourceCollection;
