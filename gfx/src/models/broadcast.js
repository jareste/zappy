export class Broadcast {
  constructor(data = {}) {
    this.message = data.message || '';
    this.direction = data.direction || 0;
    this.targetId = data.target_id || 0;
  }

  static fromJSON(data) {
    return new Broadcast(data);
  }
  
  getDirectionLabel() {
    const directions = [
      'Current tile', 'North', 'North-East', 'East',
      'South-East', 'South', 'South-West', 'West', 'North-West'
    ];
    
    if (this.direction >= 0 && this.direction < directions.length) {
      return directions[this.direction];
    }
    return 'Unknown';
  }
}

export default Broadcast;
