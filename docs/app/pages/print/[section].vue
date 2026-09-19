<script setup>
// Print view of one documentation section: every page of the section on
// a single route in navigation order, preceded by a title page and a
// table of contents. scripts/build-pdf.mjs paginates it with Paged.js
// and prints it to PDF; the @page rules in public/print.css only take
// effect there.
definePageMeta({ layout: false, header: false, footer: false });

const route = useRoute();
const section = String(route.params.section);
const config = useRuntimeConfig();

const [{ data: pages }, { data: navigation }] = await Promise.all([
    useAsyncData(`print-pages-${section}`, () =>
        queryCollection('docs').where('path', 'LIKE', `/${section}/%`).all()),
    useAsyncData(`print-navigation-${section}`, () =>
        queryCollectionNavigation('docs')),
]);

const sectionNav = navigation.value?.find(item => item.path === `/${section}`);
if (!sectionNav || !pages.value?.length) {
    throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const byPath = new Map(pages.value.map(page => [page.path, page]));

// Anchor of a content path inside the combined document.
function anchorId(path) {
    return path.replace(`/${section}/`, '').replace(/^\//, '').replaceAll('/', '-');
}

// Rewrites a page body for the combined document: heading levels are
// shifted below the chapter and page titles, sections get their number,
// element ids are prefixed with the page anchor so they stay unique, and
// links into this section become links to those anchors.
function printable(page, pageId, number) {
    const shift = number ? 1 : 0;
    let sections = 0;

    function transform(node) {
        if (!Array.isArray(node)) return node;
        let [tag, props, ...children] = node;
        props = { ...props };
        const level = /^h([1-6])$/.exec(tag);
        if (level) tag = `h${Math.min(6, Number(level[1]) + shift)}`;
        if (number && tag === 'h3') children.unshift(`${number}.${++sections}  `);
        // Gesture components use "id" for the gesture name, not as anchor.
        if (props.id && !['gesture', 'gesture-note'].includes(tag)) props.id = `${pageId}-${props.id}`;
        if (typeof props.href === 'string' && props.href.startsWith(`/${section}`)) {
            const [path, hash] = props.href.split('#');
            const target = path === `/${section}` ? section : anchorId(path);
            props.href = '#' + (hash ? `${target}-${hash}` : target);
        }
        return [tag, props, ...children.map(transform)];
    }

    // The last-commit line of the web page has no place in the PDF.
    const value = page.body.value.filter(node => node[0] !== 'page-meta');
    return { ...page, body: { ...page.body, value: value.map(transform) } };
}

// Chapters follow the navigation tree: a folder becomes a numbered
// chapter with pages, a top-level page an unnumbered chapter of its own.
let chapterCount = 0;
const chapters = (sectionNav.children ?? []).flatMap((item) => {
    const id = anchorId(item.path);
    if (item.children?.length) {
        const children = item.children.filter(child => byPath.has(child.path));
        if (!children.length) return [];
        const number = String(++chapterCount);
        return [{
            id,
            number,
            title: item.title,
            single: false,
            pages: children.map((child, index) => {
                const pageId = anchorId(child.path);
                const pageNumber = `${number}.${index + 1}`;
                return { id: pageId, number: pageNumber, page: printable(byPath.get(child.path), pageId, pageNumber) };
            }),
        }];
    }
    const page = byPath.get(item.path);
    return page ? [{ id, number: '', title: page.title, single: true, pages: [{ id, number: '', page: printable(page, id, '') }] }] : [];
});

const title = `XCSoar ${sectionNav.title}`;
const date = new Date().toLocaleDateString('en-US', { year: 'numeric', month: 'long', day: 'numeric' });

// The print stylesheet stays outside the Nuxt CSS bundle: Paged.js parses
// it on its own and must not see the Tailwind output.
useHead({
    title,
    titleTemplate: '%s',
    meta: [{ name: 'robots', content: 'noindex' }],
    link: [{ rel: 'stylesheet', href: '/print.css' }],
});
</script>

<template>
    <div class="print-root">
        <section class="print-cover">
            <span class="print-doc-title">{{ title }}</span>
            <div class="print-cover-top">
                <img src="/img/graphics/logo.svg" alt="" class="print-cover-logo">
                <img src="/img/graphics/title.svg" alt="XCSoar" class="print-cover-wordmark">
                <p class="print-cover-tagline">the open-source glide computer</p>
                <p class="print-cover-title">{{ sectionNav.title }}</p>
            </div>
            <div class="print-cover-bottom">
                <p>{{ date }}</p>
                <p v-if="config.public.xcsoarVersion">For XCSoar version {{ config.public.xcsoarVersion }}</p>
                <p v-if="config.public.xcsoarCommit">Commit {{ config.public.xcsoarCommit }}</p>
                <p>https://xcsoar.org</p>
            </div>
        </section>

        <nav class="print-toc">
            <h1>Contents</h1>
            <ol class="print-toc-chapters">
                <li v-for="chapter in chapters" :key="chapter.id">
                    <a :href="`#${chapter.id}`">
                        <span class="print-toc-number">{{ chapter.number }}</span>
                        <span class="print-toc-title">{{ chapter.title }}</span>
                    </a>
                    <ol v-if="!chapter.single" class="print-toc-pages">
                        <li v-for="entry in chapter.pages" :key="entry.id">
                            <a :href="`#${entry.id}`">
                                <span class="print-toc-number">{{ entry.number }}</span>
                                <span class="print-toc-title">{{ entry.page.title }}</span>
                            </a>
                        </li>
                    </ol>
                </li>
            </ol>
        </nav>

        <div class="print-body">
            <section v-for="chapter in chapters" :key="chapter.id" class="print-chapter">
                <h1 :id="chapter.id" class="print-chapter-title">
                    <span v-if="chapter.number" class="print-number">{{ chapter.number }}&nbsp;&nbsp;</span>{{ chapter.title }}
                </h1>
                <section v-for="entry in chapter.pages" :key="entry.id" class="print-page">
                    <h2 v-if="!chapter.single" :id="entry.id" class="print-page-title">
                        <span class="print-number">{{ entry.number }}&nbsp;&nbsp;</span>{{ entry.page.title }}
                    </h2>
                    <ContentRenderer :value="entry.page" class="print-page-body" />
                </section>
            </section>
        </div>
    </div>
</template>
