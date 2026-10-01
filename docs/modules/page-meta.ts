import { defineNuxtModule } from '@nuxt/kit';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { join, relative } from 'node:path';

// The "Last updated" line below every page, filled in while the pages are
// parsed.

type Commit = { commit: string, date: string, author: string };

// Runs git in the repository without a shell; '' when git or the
// repository is missing.
const git = (repo: string, ...args: string[]) => {
    try {
        return execFileSync('git', args, { cwd: repo, stdio: ['ignore', 'pipe', 'ignore'] }).toString().trim();
    } catch {
        return '';
    }
};

// Last commit of every content file (author, not committer) and its
// position in the history, 0 being the newest commit, from one walk of
// the history below docs/content.
const fileCommits = (repo: string) => {
    const commits = new Map<string, { last: Commit, position: number }>();
    let current: Commit | undefined;
    let position = -1;
    for (const line of git(repo, 'log', '--format=@%H %as %an', '--name-only', '--', 'docs/content', 'docs/content-history.json').split('\n')) {
        if (line.startsWith('@')) {
            const [commit, date, ...author] = line.slice(1).split(' ');
            current = { commit, date, author: author.join(' ') };
            position++;
        } else if (line && current && !commits.has(line)) {
            commits.set(line, { last: current, position });
        }
    }
    return commits;
};

export default defineNuxtModule({
    meta: { name: 'page-meta' },
    setup(_, nuxt) {
        const docs = nuxt.options.rootDir;
        const repo = join(docs, '..');
        // Pages migrated from the LaTeX manual, the RST developer docs or
        // the InfoBox sources show the last commit of that source,
        // collected by scripts/import-history.mjs, unless the page was
        // changed after the last commit of content-history.json.
        const history: Record<string, Commit> = JSON.parse(readFileSync(join(docs, 'content-history.json'), 'utf8'));
        let commits: Map<string, { last: Commit, position: number }> | undefined;
        const lastCommit = (path: string): Commit | undefined => {
            commits ??= fileCommits(repo);
            const own = commits.get(relative(repo, path));
            const migrated = history[relative(join(docs, 'content'), path)];
            const historyPosition = commits.get('docs/content-history.json')?.position ?? -1;
            if (migrated && (!own || own.position >= historyPosition)) {
                const { commit, date, author } = migrated;
                return { commit, date, author };
            }
            return own?.last;
        };

        nuxt.hook('content:file:afterParse', ({ file, content }) => {
            const body = content.body as { type?: string, value?: unknown[] } | undefined;
            if (body?.type !== 'minimark') return;
            if (!(content.stem as string).endsWith('/index')) {
                // The last commit of the page ends its body; PageMeta.vue
                // renders it below the content. The landing page has none.
                const last = lastCommit(file.path);
                if (last) body.value!.push(['page-meta', last]);
            }
        });
    },
});
