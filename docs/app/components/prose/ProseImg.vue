<script setup>
// Replaces the Nuxt UI prose image: centred figure with an optional caption,
// and the image is never scaled above its natural size (the default applies
// w-full when no width is given, which blows up small screenshots). A given
// width attribute is honoured, and inside table cells the figure margin is
// dropped so small inline icons do not inflate the row height.
// The zoom-on-click behaviour of the original component is kept.
import UProseImg from '@nuxt/ui/runtime/components/prose/Img.vue';

defineOptions({ inheritAttrs: false });

const props = defineProps({
    src: { type: String, required: true },
    alt: { type: String, default: '' },
    // Markdown title (![alt](src "title")) is shown as caption.
    title: { type: String, default: '' },
    width: { type: [String, Number], required: false },
    height: { type: [String, Number], required: false },
});
</script>

<template>
    <figure class="my-5 flex flex-col items-center [td_&]:my-0">
        <UProseImg
            :src="props.src"
            :alt="props.alt"
            :width="props.width"
            :height="props.height"
            densities="x1"
            :class="[props.width ? '' : 'w-auto', 'max-w-full h-auto']"
            v-bind="$attrs"
        />
        <figcaption v-if="props.title" class="mt-2 text-sm text-muted text-center">
            {{ props.title }}
        </figcaption>
    </figure>
</template>
