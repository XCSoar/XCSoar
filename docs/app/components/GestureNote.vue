<script setup>
// Paragraph with gesture drawings in a column on its left, like the margin
// notes of the LaTeX manual. Several gestures are separated by spaces:
//
//   ::gesture-note{id="L R"}
//   To go through the various screen pages ...
//   ::
const props = defineProps({
  id: {
    type: String,
    required: true,
  },
})

const ids = computed(() => props.id.trim().split(/\s+/))
</script>

<template>
  <div class="my-5 flex items-start gap-4">
    <div class="flex shrink-0 flex-col gap-2 mt-1">
      <Gesture v-for="gesture in ids" :key="gesture" :id="gesture" />
    </div>
    <div class="min-w-0 flex-1 [&>*:first-child]:mt-0 [&>*:last-child]:mb-0">
      <slot />
    </div>
  </div>
</template>
