<script setup>
// Docus shows either the logo image or the title. Show both: the XCSoar
// emblem next to the site name.
const appConfig = useAppConfig();
</script>

<template>
    <span class="flex items-center gap-2">
        <img
            :src="appConfig.header.logo.light"
            :alt="appConfig.header.logo.alt"
            class="size-7 shrink-0"
        >
        <span class="font-semibold text-highlighted">
            {{ appConfig.header.title }}
        </span>
    </span>
</template>
