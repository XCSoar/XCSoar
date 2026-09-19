<script setup>
// The documented version next to the header links: the release version,
// the commit of a development build, or HEAD when the build contains
// uncommitted changes below docs/.
const config = useRuntimeConfig();

const version = computed(() => {
    const { xcsoarVersion, xcsoarCommit, xcsoarDirty } = config.public;
    if (xcsoarDirty) return 'HEAD';
    return xcsoarVersion || xcsoarCommit.slice(0, 7);
});
</script>

<template>
    <UBadge v-if="version" color="neutral" variant="subtle" size="sm" class="font-mono">
        {{ version }}
    </UBadge>
    <UButton
        to="https://xcsoar.org"
        target="_blank"
        color="neutral"
        variant="ghost"
        icon="i-lucide-globe"
        label="xcsoar.org"
        class="hidden sm:inline-flex"
    />
</template>
