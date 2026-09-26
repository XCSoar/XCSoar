<script setup>
// The Docus docs layout, without the sidebar on the landing page: the
// landing page uses this layout only for the row of sections in the header
// (see nuxt.config.ts).
const route = useRoute();
const landing = computed(() => route.path === '/');
</script>

<template>
  <UMain>
    <slot v-if="landing" />
    <UContainer v-else>
      <UPage>
        <template #left>
          <UPageAside>
            <DocsAsideLeftTop />
            <DocsAsideLeftBody />
          </UPageAside>
        </template>
        <slot />
      </UPage>
    </UContainer>
  </UMain>
</template>
