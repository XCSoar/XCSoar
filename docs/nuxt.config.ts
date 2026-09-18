import { execSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

// Serve the manual figures and map icons straight from the repository
// instead of keeping copies in docs/public. Paths must be absolute because
// nitro resolves relative publicAssets dirs against its own srcDir.
const repo = (path: string) => fileURLToPath(new URL(path, import.meta.url));

// Commit of the documentation build, shown on the PDF title page.
const gitCommit = () => {
    try {
        return execSync('git rev-parse --short=12 HEAD', { cwd: repo('..'), stdio: ['ignore', 'pipe', 'ignore'] })
            .toString().trim();
    } catch {
        return '';
    }
};

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
    content: {
        build: {
            markdown: {
                highlight: {
                    // Docus ships only web languages; the developer docs
                    // also show C++, Lua, XML and make snippets.
                    langs: ['cpp', 'lua', 'xml', 'make'],
                },
            },
        },
    },
    image: {
        // Figures come from nitro publicAssets outside docs/public, which the
        // IPX optimizer cannot read. Serve image URLs unchanged instead.
        provider: 'none',
    },
    runtimeConfig: {
        public: {
            xcsoarVersion: readFileSync(repo('../VERSION.txt'), 'utf8').trim(),
            xcsoarCommit: gitCommit(),
        },
    },
    routeRules: {
        // Print views for scripts/build-pdf.mjs, not for search engines.
        '/print/**': { robots: false },
    },
    nitro: {
        prerender: {
            autoSubfolderIndex: true,
            routes: ['/print/manual', '/print/quick-guide', '/print/dev'],
        },
        publicAssets: [
            { baseURL: '/img/figures', dir: repo('../doc/manual/en/figures') },
            { baseURL: '/img/drawings', dir: repo('../doc/manual/figures') },
            { baseURL: '/img/icons', dir: repo('../Data/icons') },
            { baseURL: '/img/graphics', dir: repo('../Data/graphics') },
        ],
    },
});
