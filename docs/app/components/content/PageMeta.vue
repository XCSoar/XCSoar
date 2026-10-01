<script setup>
// Last commit of the page, appended to every page body by the
// content:file:afterParse hook in nuxt.config.ts.
const props = defineProps({
    commit: { type: String, required: true },
    date: { type: String, required: true },
    author: { type: String, required: true },
});

const appConfig = useAppConfig();

const commitUrl = computed(() => `${appConfig.github.url}/commit/${props.commit}`);

// The date is a plain YYYY-MM-DD; format it without a time zone so the
// server and the browser agree.
const date = computed(() => {
    const [year, month, day] = props.date.split('-').map(Number);
    return new Date(Date.UTC(year, month - 1, day))
        .toLocaleDateString('en-US', { year: 'numeric', month: 'long', day: 'numeric', timeZone: 'UTC' });
});
</script>

<template>
    <p class="mt-10 flex flex-wrap items-center gap-x-1.5 gap-y-1 text-sm italic text-muted">
        <span>Last updated by <span class="text-default">{{ author }}</span> on {{ date }}</span>
        <UButton
            :to="commitUrl"
            target="_blank"
            color="neutral"
            variant="subtle"
            size="xs"
            class="font-mono"
        >
            {{ commit.slice(0, 7) }}
        </UButton>
    </p>
</template>
