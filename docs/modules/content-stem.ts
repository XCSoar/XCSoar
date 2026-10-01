import { defineNuxtModule } from '@nuxt/kit';
import { join, relative } from 'node:path';

// The stem of a page is its path below the source of its collection, which
// is the language folder (content/en/), so the language is missing from it.
// The "Edit this page" link of Docus builds the path in the repository from
// the stem, so the language is put back. Nuxt Content runs the hook while
// it parses a page, and it parses one only after its file or the build
// options changed; a build with pages from the cache (.data/content) keeps
// what they had.
export default defineNuxtModule({
    meta: { name: 'content-stem' },
    setup(_, nuxt) {
        const content = join(nuxt.options.rootDir, 'content');
        nuxt.hook('content:file:afterParse', ({ file, content: page }) => {
            page.stem = relative(content, file.path).replace(/\.[^./]+$/, '');
        });
    },
});
