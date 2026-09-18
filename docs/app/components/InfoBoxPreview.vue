<script setup>
// Schematic InfoBox as drawn on the XCSoar map: caption on top, value in
// the middle, optional unit and comment.
defineProps({
    caption: { type: String, required: true },
    value: { type: String, default: '---' },
    unit: { type: String, default: '' },
    comment: { type: String, default: '' },
});
</script>

<template>
    <div class="w-44 shrink-0 aspect-[3/2] flex flex-col rounded-sm border-2 border-neutral-800 bg-white text-neutral-900 font-sans select-none">
        <div class="pt-1 text-center text-[11px] font-medium leading-none tracking-wide">
            {{ caption }}
        </div>
        <div class="flex flex-1 items-center justify-center">
            <span class="flex items-baseline gap-1">
                <span class="text-4xl font-semibold leading-none">{{ value }}</span>
                <span v-if="unit" class="text-sm">{{ unit }}</span>
            </span>
        </div>
        <div class="h-4 pb-1 text-center text-[11px] leading-none text-neutral-500">
            {{ comment }}
        </div>
    </div>
</template>
