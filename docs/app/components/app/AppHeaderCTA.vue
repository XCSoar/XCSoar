<script setup>
// The documented version next to the header links, linked to its release
// on GitHub: the release version, the commit of a development build, or
// HEAD without a link when the build contains uncommitted changes below
// docs/.
const config = useRuntimeConfig();
const appConfig = useAppConfig();

const version = computed(() => {
    const { xcsoarVersion, xcsoarCommit, xcsoarDirty } = config.public;
    if (xcsoarDirty) return 'HEAD';
    return xcsoarVersion || xcsoarCommit.slice(0, 7);
});

const versionUrl = computed(() => {
    const { xcsoarVersion, xcsoarCommit, xcsoarDirty } = config.public;
    if (xcsoarDirty) return undefined;
    if (xcsoarVersion) return `${appConfig.github.url}/releases/tag/v${xcsoarVersion}`;
    return `${appConfig.github.url}/commit/${xcsoarCommit}`;
});
</script>

<template>
    <UButton
        v-if="version"
        :to="versionUrl"
        target="_blank"
        color="neutral"
        variant="subtle"
        size="xs"
        class="font-mono"
    >
        {{ version }}
    </UButton>
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
