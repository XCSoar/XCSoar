<script setup>
// Paragraph with an InfoBox in a column on its left, like GestureAside:
//
//   ::infobox-aside{title="WP AltD" value="+310" unit="m" color="green"}
//   The arrival height is positive, the waypoint is within reach.
//   ::
defineProps({
    title: { type: String, required: true },
    value: { type: String, default: '---' },
    unit: { type: String, default: '' },
    comment: { type: String, default: '' },
    color: { type: String, default: '' },
    commentColor: { type: String, default: '' },
    graphic: { type: String, default: '' },
});
</script>

<template>
    <div class="infobox-aside my-5 flex items-start gap-4">
        <Infobox
            :title="title"
            :value="value"
            :unit="unit"
            :comment="comment"
            :color="color"
            :comment-color="commentColor"
            :graphic="graphic"
        />
        <div class="min-w-0 flex-1 [&>*:first-child]:mt-0 [&>*:last-child]:mb-0">
            <slot />
        </div>
    </div>
</template>
