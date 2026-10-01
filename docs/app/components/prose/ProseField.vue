<script setup>
// The Field of Nuxt UI, with one change: the description keeps its
// paragraphs. The original renders the slot with mdc-unwrap="p", which
// runs the paragraphs of a :::field together without a break.
import { computed } from 'vue';
import theme from '#build/ui/prose/field';
import { tv } from '@nuxt/ui/utils/tv';

const props = defineProps({
    name: { type: String, required: false },
    type: { type: String, required: false },
    description: { type: String, required: false },
    required: { type: Boolean, required: false },
    class: { type: null, required: false },
});

const slots = defineSlots();
const appConfig = useAppConfig();
const ui = computed(() => tv({ extend: theme, ...appConfig.ui?.prose?.field || {} })());
</script>

<template>
    <div :class="ui.root({ class: props.class })">
        <div :class="ui.container()">
            <span v-if="props.name" :class="ui.name()">{{ props.name }}</span>

            <div v-if="props.type || props.required" :class="ui.wrapper()">
                <span v-if="props.type" :class="ui.type()">{{ props.type }}</span>
                <span v-if="props.required" :class="ui.required()">required</span>
            </div>
        </div>

        <div v-if="!!slots.default || props.description" :class="ui.description()">
            <slot>{{ props.description }}</slot>
        </div>
    </div>
</template>
