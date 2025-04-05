

<template>

</template>

<script setup>
import { useGameStore } from '@/stores/gameStore'


const gameStore = useGameStore()

const board = ref(gameStore.getMap())
console.log('board', board.value)
</script>