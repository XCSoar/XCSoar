<script setup>
// Everything an InfoBox page shows above its text: the InfoBox as XCSoar
// draws it, the help text of the app and the facts of the InfoBox. The
// print view renders the same component, public/print.css gives it the
// sizes of the PDF.
const props = defineProps({
    title: { type: String, required: true },
    infobox: { type: Object, required: true },
});

const demo = computed(() => props.infobox.demo ?? {});

// Name is the title in the InfoBox configuration dialogue, caption the
// label drawn in the InfoBox itself.
const facts = computed(() => [
    { label: 'Name', value: props.title },
    { label: 'Caption', value: props.infobox.caption },
    { label: 'Category', value: props.infobox.category },
    { label: 'ID', value: props.infobox.id, code: true },
    { label: 'Index', value: props.infobox.index, code: true },
]);
</script>

<template>
    <div class="infobox-summary">
        <div class="infobox-summary-head my-6 flex flex-col gap-6 sm:flex-row sm:items-start">
            <Infobox
                :title="demo.title ?? infobox.caption"
                :value="demo.value ?? '---'"
                :unit="demo.unit ?? ''"
                :comment="demo.comment ?? ''"
                :color="demo.color ?? ''"
                :comment-color="demo.commentColor ?? ''"
                :graphic="demo.graphic ?? ''"
            />
            <div class="infobox-summary-text min-w-0 flex-1">
                <div class="infobox-summary-badges mb-3 flex flex-wrap items-center gap-2">
                    <UBadge v-if="infobox.category" color="primary" variant="subtle">{{ infobox.category }}</UBadge>
                    <UBadge color="neutral" variant="outline" class="font-mono">{{ infobox.id }}</UBadge>
                </div>
                <p class="text-base/7 text-default">{{ infobox.help }}</p>
            </div>
        </div>

        <table class="infobox-summary-facts my-6 w-full text-sm">
            <tbody class="divide-y divide-default">
                <tr
                    v-for="(fact, index) in facts"
                    :key="fact.label"
                    :class="{ 'infobox-summary-last': index === facts.length - 1 }"
                >
                    <th scope="row" class="w-40 py-2 pr-4 text-left font-medium text-muted">{{ fact.label }}</th>
                    <td class="py-2">
                        <code v-if="fact.code" class="rounded-md bg-muted px-1.5 py-0.5 font-mono text-sm">{{ fact.value }}</code>
                        <template v-else>{{ fact.value }}</template>
                    </td>
                </tr>
            </tbody>
        </table>
    </div>
</template>
