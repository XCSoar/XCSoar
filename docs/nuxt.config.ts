import { fileURLToPath } from 'node:url';

// Serve the manual figures and map icons straight from the repository
// instead of keeping copies in docs/public. Paths must be absolute because
// nitro resolves relative publicAssets dirs against its own srcDir.
const repo = (path: string) => fileURLToPath(new URL(path, import.meta.url));

export default defineNuxtConfig({
    devtools: false,
    extends: ['docus'],
    app: {
        head: {
            script: [
                {
                    // Static hosts redirect "/manual/preface" to
                    // "/manual/preface/", but the page was prerendered
                    // without the slash. Drop it before Nuxt boots so the
                    // prerendered payload is reused on hydration.
                    innerHTML: "if (location.pathname.length > 1 && location.pathname.endsWith('/')) history.replaceState(null, '', location.pathname.replace(/\\/+$/, '') + location.search + location.hash)",
                },
            ],
        },
    },
    image: {
        // Figures come from nitro publicAssets outside docs/public, which the
        // IPX optimizer cannot read. Serve image URLs unchanged instead.
        provider: 'none',
    },
    nitro: {
        prerender: {
            autoSubfolderIndex: true,
        },
        publicAssets: [
            { baseURL: '/img/figures', dir: repo('../doc/manual/en/figures') },
            { baseURL: '/img/drawings', dir: repo('../doc/manual/figures') },
            { baseURL: '/img/icons', dir: repo('../Data/icons') },
            { baseURL: '/img/graphics', dir: repo('../Data/graphics') },
        ],
    },
});
