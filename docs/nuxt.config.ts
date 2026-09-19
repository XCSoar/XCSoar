import { execFileSync } from 'node:child_process';
import { relative } from 'node:path';
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

// Staged or unstaged changes below docs/: the build matches no commit.
const xcsoarDirty = git('status', '--porcelain', '--untracked-files=no', '--', 'docs') !== '';

// Last commit of every content file (author, not committer), from one
// walk of the history below docs/content.
type LastCommit = { commit: string, date: string, author: string };
let lastCommits: Map<string, LastCommit> | undefined;
const lastCommit = (path: string) => {
    if (!lastCommits) {
        lastCommits = new Map();
        let current: LastCommit | undefined;
        for (const line of git('log', '--format=@%H %as %an', '--name-only', '--', 'docs/content').split('\n')) {
            if (line.startsWith('@')) {
                const [commit, date, ...author] = line.slice(1).split(' ');
                current = { commit, date, author: author.join(' ') };
            } else if (line && current && !lastCommits.has(line)) {
                lastCommits.set(line, current);
            }
        }
    }
    return lastCommits.get(relative(repo('..'), path));
};

export default defineNuxtConfig({
    devtools: false,
    extends: ['docus'],
    hooks: {
        // Append the last commit of the page to its body; PageMeta.vue
        // renders it below the content. The landing page has none.
        'content:file:afterParse'({ file, content }) {
            const body = content.body as { type?: string, value?: unknown[] } | undefined;
            const last = body?.type === 'minimark' && content.stem !== 'index' ? lastCommit(file.path) : undefined;
            if (last) body.value!.push(['page-meta', last]);
        },
    },
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
                // Nuxt Content caches parsed pages by file content and
                // these options. The commit makes every page re-parse after
                // a new commit, so the last-commit line below the page (see
                // hooks) does not go stale.
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
