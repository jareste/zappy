import { ref, computed } from 'vue'
import { defineStore } from 'pinia'
import { Game } from '../models/game'
import { readFile } from 'fs/promises'

export const useGameStore = defineStore('gameStore', () => {
  // State
  const game = ref(new Game())
  const loading = ref(false)
  const error = ref(null)

  // Getters
  const getMap = computed(() => game.value.map)
  const getMapSize = computed(() => ({
    width: game.value.map.width,
    height: game.value.map.height
  }))
  const getPlayers = computed(() => game.value.players)
  const getEggs = computed(() => game.value.eggs)
  const getBroadcasts = computed(() => game.value.broadcasts)
  const getEvents = computed(() => game.value.events)
  const getTeams = computed(() => game.value.gameInfo.teams)
  const getGameInfo = computed(() => game.value.gameInfo)
  
  const getTileAt = computed(() => (x, y) => game.value.map.getTileAt(x, y))
  const getPlayer = computed(() => (id) => game.value.getPlayer(id))
  const getEgg = computed(() => (id) => game.value.getEgg(id))
  const getTeam = computed(() => (name) => game.value.getTeam(name))
  const getPlayersInTeam = computed(() => (teamName) => game.value.getPlayersInTeam(teamName))

  // Actions
  async function loadFromJSON(path) {
    loading.value = true
    error.value = null
  
    try {
      const fileContent = await readFile(path, 'utf-8')
      const data = JSON.parse(fileContent)
  
      // Using the Game model to parse and store the data
      if (game.value.players.length === 0) {
        // First load - create new Game
        game.value = Game.fromJSON(data)
        console.log('Game loaded from JSON:', game.value)
      } else {
        // Subsequent loads - update existing Game
        game.value.update(data)
      }
  
      return true
    } catch (err) {
      error.value = err.message || 'Failed to load game data'
      return false
    } finally {
      loading.value = false
  }
  
  // Placeholder for future implementation
  function loadFromCSV(path) {
    // To be implemented when backend is ready
    console.log('CSV loading not yet implemented:', path)
    return false
  }
  
  // For development/testing purposes
  function loadMockData(data) {
    game.value = Game.fromJSON(data)
  }
  
  function reset() {
    game.value = new Game()
    error.value = null
  }

  return { 
    // State
    game,
    loading,
    error,
    
    // Getters
    getMap,
    getMapSize,
    getPlayers,
    getEggs,
    getBroadcasts,
    getEvents,
    getTeams,
    getGameInfo,
    getTileAt,
    getPlayer,
    getEgg,
    getTeam,
    getPlayersInTeam,
    
    // Actions
    loadFromJSON,
    loadFromCSV,
    loadMockData,
    reset
  }
})
