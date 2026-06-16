<script setup>
const sizes = {
  sm: 'h-8',
  md: 'h-12',
  lg: 'h-16',
};

// Gesture id -> drawing in doc/manual/figures (served at /img/drawings)
const files = {
  u: 'gesture_up',
  d: 'gesture_down',
  r: 'gesture_right',
  l: 'gesture_left',
  ud: 'gesture_ud',
  du: 'gesture_du',
  dr: 'gesture_dr',
  dl: 'gesture_dl',
  rd: 'gesture_rd',
  rl: 'gesture_rl',
  lu: 'gesture_lu',
  urd: 'gesture_urd',
  ldr: 'gesture_ldr',
  urdl: 'gesture_urdl',
  ldrdl: 'gesture_ldrdl',
  uldr: 'gesture_uldr',
};

const props = defineProps({
  id: {
    type: String,
    required: true,
  },
  size: {
    type: String,
    default: 'md',
    validator: (v) => ['sm', 'md', 'lg'].includes(v),
  },
  // Float the drawing at the left edge of the paragraph so the text wraps
  // around it, similar to the margin icons of the LaTeX manual.
  float: {
    type: Boolean,
    default: false,
  },
})

const file = computed(() => files[props.id.toLowerCase()])
</script>

<template>
  <img
    v-if="file"
    :src="`/img/drawings/${file}.svg`"
    :alt="`Gesture ${id}`"
    class="w-auto"
    :class="[sizes[size], float ? 'float-left mr-3 mb-1' : 'inline-block align-middle']"
  />
  <span v-else class="inline-block align-middle font-mono text-xs text-red-500">
    {{ `Unknown gesture: ${id}` }}
  </span>
</template>
