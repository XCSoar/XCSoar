import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

// Serve the manual figures and map icons straight from the repository
// instead of keeping copies in docs/public. Paths must be absolute because
// nitro resolves relative publicAssets dirs against its own srcDir.
const repo = (path: string) => fileURLToPath(new URL(path, import.meta.url));

// Runs git in the repository without a shell; '' when git or the
// repository is missing.
const git = (...args: string[]) => {
    try {
        return execFileSync('git', args, { cwd: repo('..'), stdio: ['ignore', 'pipe', 'ignore'] })
            .toString().trim();
    } catch {
        return '';
    }
};

// Commit of the documentation build, shown on the PDF title page.
const xcsoarCommit = git('rev-parse', '--short=12', 'HEAD');

// Like the XCSoar build system: a commit tagged v<version> is the release
// of that version, anything else a development build without a version.
const xcsoarVersion = git('tag', '--points-at', 'HEAD').split('\n')
    .map(tag => /^v(\d[\w.]*)$/.exec(tag)?.[1])
    .find(Boolean) ?? '';

// The languages of the documentation, the first one the default: its
// pages keep the bare paths (/manual/…), the others live below their code
// (/de/manual/…). Every language has its own folder below content/, see
// content.config.ts.
const locales = [
    { code: 'en', name: 'English', language: 'en-GB', dir: 'ltr' },
];
const defaultLocale = locales[0].code;

// Staged or unstaged changes below docs/: the build matches no commit.
const xcsoarDirty = git('status', '--porcelain', '--untracked-files=no', '--', 'docs') !== '';

export default defineNuxtConfig({
    devtools: false,
    extends: ['docus'],
    hooks: {
        // The landing page gets the docs layout, which shows the row of
        // sections below the header; app/layouts/docs.vue leaves out the
        // sidebar for it. With @nuxtjs/i18n Docus puts it on /:lang?, which
        // would also swallow /manual and the other sections, so the
        // parameter is narrowed to the languages.
        'pages:resolved'(pages) {
            const landing = pages.find(page => page.path === '/' || page.path === '/:lang?');
            if (!landing) return;
            if (landing.path === '/:lang?') {
                landing.path = `/:lang(${locales.map(locale => locale.code).join('|')})?`;
            }
            landing.meta = { ...landing.meta, layout: 'docs' };
        },
        // Docus prerenders /<code> for every language. The pages of the
        // default language are at /, where the landing page is already
        // prerendered, so /en would only be a copy of it.
        'nitro:config'(nitro) {
            const routes = nitro.prerender?.routes;
            const copy = `/${defaultLocale}`;
            if (routes) nitro.prerender!.routes = routes.filter(route => route !== copy);
        },
    },
    // The strategy module runs before @nuxtjs/i18n reads its options,
    // see modules/i18n-strategy.ts.
    modules: ['~~/modules/i18n-strategy', '@nuxtjs/i18n'],
    i18n: {
        defaultLocale,
        // The default language keeps the bare paths (/manual/…), another
        // language lives below its code (/de/manual/…). Docus forces
        // "prefix" on every language, modules/i18n-strategy.ts takes that
        // back.
        strategy: 'prefix_except_default',
        locales,
    },
    components: [
        // Nuxt Content resolves prose components by their global name, so an
        // override of one (app/components/prose/) has to be global as well.
        { path: '~/components/prose', pathPrefix: false, global: true },
        '~/components',
    ],
    content: {
        build: {
            markdown: {
                highlight: {
                    // Docus ships only web languages; the developer docs
                    // also show C++, Lua, XML and make snippets.
                    langs: ['cpp', 'lua', 'xml', 'make'],
                },
                // Nuxt Content caches parsed pages by file content and
                // these options. The commit makes every page re-parse after
                // a new commit, so the last-commit line below the page (see
                // modules/page-meta.ts) does not go stale.
                ...({ gitHead: xcsoarCommit } as object),
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
            xcsoarVersion,
            xcsoarCommit,
            xcsoarDirty,
        },
    },
    routeRules: {
        // Print views for scripts/build-pdf.mjs, not for search engines.
        '/print/**': { robots: false },
    },
    nitro: {
        prerender: {
            autoSubfolderIndex: true,
            routes: ['/print/manual', '/print/quick-guide', '/print/infobox', '/print/dev'],
        },
        publicAssets: [
            { baseURL: '/img/figures', dir: repo('../doc/manual/en/figures') },
            { baseURL: '/img/drawings', dir: repo('../doc/manual/figures') },
            { baseURL: '/img/icons', dir: repo('../Data/icons') },
            { baseURL: '/img/graphics', dir: repo('../Data/graphics') },
        ],
    },
});
