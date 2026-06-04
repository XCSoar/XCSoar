<script setup>
const route = useRoute();
const { sidebarNavigation } = useSubNavigation();
const { navigationByCategory } = useInfoBoxNavigation();

console.log(route.path);

const isInfoBoxes = computed(() => route.path.startsWith('/infobox'));

const searchTerm = ref('');
const cleanedSearchTerm = computed(() => searchTerm.value.trim().toLowerCase());

function fuzzysearch(needle, haystack) {
    needle = needle.toLowerCase();
    haystack = haystack.toLowerCase();
    const hlen = haystack.length;
    const nlen = needle.length;
    if (nlen > hlen) return false;
    if (nlen === hlen) return needle === haystack;
    outer: for (let i = 0, j = 0; i < nlen; i++) {
        const nch = needle.charCodeAt(i);
        while (j < hlen) {
            if (haystack.charCodeAt(j++) === nch) continue outer;
        }
        return false;
    }
    return true;
}

const filteredNavigation = computed(() => {
    if (!cleanedSearchTerm.value) return navigationByCategory.value;

    return navigationByCategory.value
        .filter(group => !group.hidden)
        .map(group => ({
            ...group,
            children: group.children?.filter(child => {
                const searchTerm = `${String(child.infoboxIndex).padStart(3, '0')} ${child.title} ${child.infoboxCaption}`;
                return fuzzysearch(cleanedSearchTerm.value, searchTerm)
        }),
    }))
    .filter(group => group.children && group.children.length > 0)
});
</script>

<template>
    <template v-if="isInfoBoxes">
        <UInput v-model="searchTerm" variant="soft" placeholder="Filter..." class="mb-3 w-full">
            <template v-if="searchTerm?.length" #trailing>
                <UButton
                    color="neutral"
                    variant="link"
                    size="sm"
                    icon="i-lucide-circle-x"
                    aria-label="Clear input"
                    @click="searchTerm = ''"
                />
            </template>
        </UInput>
        <UContentNavigation :collapsible="true" :navigation="filteredNavigation" highlight />
    </template>
    <UContentNavigation v-else :navigation="sidebarNavigation" highlight />
</template>
