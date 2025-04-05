<script setup>
import { Box, OrbitControls, Sky } from '@tresjs/cientos'
import { TresCanvas } from '@tresjs/core'
import { Board } from '@/components/Board.vue'
import { useGameStore } from '@/stores/gameStore'


const gameStore = useGameStore()

const board = ref(null)

onMounted(() => {
  board.value = gameStore.loadFromJSON('./game.json')
  console.log('board', board.value)
})



</script>

<template>
  <main>
    <TresCanvas clear-color="#82DBC5" window-size>
      <TresPerspectiveCamera :position="[3, 3, 3]" />
      <OrbitControls />
      
      <Board v-if="board"/>
      
      <TresAmbientLight />
      <TresDirectionalLight
        :position="[0, 2, 4]"
      />
      <Sky/>
    </TresCanvas>
  </main>
</template>
