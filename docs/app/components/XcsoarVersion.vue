<script setup>
// The version of the documentation, as in the badge of the header: the
// release, the commit of a development build, or HEAD when the build
// contains uncommitted changes below docs/.
const config = useRuntimeConfig();

const version = computed(() => {
    const { xcsoarVersion, xcsoarCommit, xcsoarDirty } = config.public;
    if (xcsoarDirty) return 'HEAD';
    return xcsoarVersion || xcsoarCommit.slice(0, 7);
});
</script>

<template>{{ version }}</template>
