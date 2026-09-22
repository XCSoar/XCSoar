<script setup>
// An InfoBox as XCSoar draws it on the map: the title on top, the value in
// the middle with its unit, the comment below. The demo values of the
// InfoBox pages come from the infobox.demo frontmatter, see
// .cursor/rules/user-manual.mdc:
//
//   :infobox{title="H AGL" value="1230" unit="m" comment="4035 ft"}
//
// The colours are those of InfoBoxLook: a value or comment is drawn in
// colour when the InfoBox says something about it, red for a warning,
// green for a positive arrival height, and so on.
const colors = {
    red: 'text-red-600',
    blue: 'text-blue-600',
    green: 'text-green-600',
    yellow: 'text-yellow-600',
    magenta: 'text-fuchsia-600',
};

defineProps({
    title: { type: String, required: true },
    value: { type: String, default: '---' },
    unit: { type: String, default: '' },
    comment: { type: String, default: '' },
    // red, blue, green, yellow or magenta, empty for the normal colour
    color: { type: String, default: '' },
    commentColor: { type: String, default: '' },
    // An InfoBox that draws a diagram instead of a value names its
    // schematic drawing in public/img/infobox, without the extension.
    graphic: { type: String, default: '' },
});
</script>

<template>
    <div class="w-44 shrink-0 aspect-[3/2] flex flex-col rounded-sm border-2 border-neutral-800 bg-white text-neutral-900 font-sans select-none">
        <div class="pt-1 text-center text-[11px] font-medium leading-none tracking-wide">
            {{ title }}
        </div>
        <div class="flex flex-1 items-center justify-center overflow-hidden">
            <img
                v-if="graphic"
                :src="`/img/infobox/${graphic}.svg`"
                :alt="title"
                class="h-full w-full object-cover"
            >
            <span v-else class="flex items-baseline gap-1" :class="colors[color]">
                <span class="text-4xl font-semibold leading-none">{{ value }}</span>
                <span v-if="unit" class="text-sm">{{ unit }}</span>
            </span>
        </div>
        <div
            class="h-4 pb-1 text-center text-[11px] leading-none"
            :class="commentColor ? colors[commentColor] : 'text-neutral-500'"
        >
            {{ comment }}
        </div>
    </div>
</template>
