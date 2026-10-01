import { defineNuxtModule } from '@nuxt/kit';
import { readFileSync } from 'node:fs';
import { join } from 'node:path';

// The licence page shows COPYING at the root of the repository verbatim,
// not parsed as markdown. One block per paragraph, because the print
// breaks between blocks; a single block of the whole text ends up on a
// page of its own with an empty one before it. app/app.css gives the
// blocks the look of a code block, public/print.css takes it away again.
export default defineNuxtModule({
    meta: { name: 'license' },
    setup(_, nuxt) {
        nuxt.hook('content:file:afterParse', ({ content }) => {
            const body = content.body as { type?: string, value?: unknown[] } | undefined;
            // The stem starts with the language folder, see content.config.ts.
            if (body?.type !== 'minimark'
                || !/(^|\/)1\.manual\/15\.license$/.test(content.stem as string)) return;
            const paragraphs = readFileSync(join(nuxt.options.rootDir, '..', 'COPYING'), 'utf8')
                .trimEnd().split(/\n{2,}/)
                .map(paragraph => ['div', { class: 'license-paragraph' }, paragraph]);
            // Whatever the page-meta module appended stays, whichever of
            // the two modules runs first.
            const meta = body.value!.filter(node => (node as unknown[])[0] === 'page-meta');
            body.value = [['div', { class: 'license' }, ...paragraphs], ...meta];
        });
    },
});
