export default defineNuxtConfig({
    devtools: false,
    extends: ['docus'],
    nitro: {
        prerender: {
            autoSubfolderIndex: true,
        },
    },
});
