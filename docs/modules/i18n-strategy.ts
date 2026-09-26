import { defineNuxtModule } from '@nuxt/kit';

// Docus forces the i18n strategy to "prefix", which would move the pages
// of the default language to /en/… . The documentation keeps the bare
// paths there, so the strategy of nuxt.config.ts is put back: Docus
// replaces the whole i18n key, but the configuration of this project is
// still in its layer. The module is listed before @nuxtjs/i18n so that it
// runs after the Docus layer and before the module that reads the
// strategy.
export default defineNuxtModule({
    meta: { name: 'i18n-strategy' },
    setup(_, nuxt) {
        const layer = nuxt.options._layers?.[0]?.config as { i18n?: { strategy?: string } } | undefined;
        const i18n = nuxt.options.i18n as { strategy?: string } | undefined;
        if (i18n && layer?.i18n?.strategy) i18n.strategy = layer.i18n.strategy;
    },
});
