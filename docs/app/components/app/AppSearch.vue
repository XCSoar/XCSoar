<script setup>
// The search of Docus, with the ranking and the grouping of the results
// done here: the pages are searched with the full-text search of Nuxt
// Content, which ranks a section by how well it matches and returns the
// passage around the match, the sections that contain the words as they
// were typed come first, and the rest is grouped by documentation section
// as the navigation groups them.

const props = defineProps({
    navigation: { type: Array, required: false },
});

const { forced: forcedColorMode } = useDocusColorMode();
const { open } = useContentSearch();
const appConfig = useAppConfig();

const collectionName = useDocsCollection();
const { search: searchSections, status, init } = useSearchCollection(collectionName, {
    immediate: false,
    ignoredTags: ['style'],
});

// The index is built in the browser, on the first opening of the dialogue.
watch(open, value => {
    if (value && status.value === 'idle') init();
});

const searchTerm = ref('');
const results = ref([]);

// Characters of the passage shown around the words as they were typed.
const AROUND = 60;
// Sections searched, and shown, at most.
const LIMIT = 24;

const escapeHtml = text => text.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;');

// The text of a section keeps the line breaks of the markdown, which run
// through the words the pilot typed in one line.
const collapse = text => (text ?? '').replace(/\s+/g, ' ');

// A snippet of the full-text search, its <mark> kept and the text around
// it escaped.
const sanitize = snippet => snippet.split(/(<\/?mark>)/)
    .map(part => (part === '<mark>' || part === '</mark>' ? part : escapeHtml(part)))
    .join('');

// The passage around the first occurrence of the words as they were
// typed, marked like the snippets of the full-text search.
function passage(content, phrase) {
    const at = content.toLowerCase().indexOf(phrase);
    if (at < 0) return undefined;
    const start = Math.max(0, at - AROUND);
    const end = Math.min(content.length, at + phrase.length + AROUND);
    return [
        start > 0 ? '…' : '',
        escapeHtml(content.slice(start, at)),
        '<mark>', escapeHtml(content.slice(at, at + phrase.length)), '</mark>',
        escapeHtml(content.slice(at + phrase.length, end)),
        end < content.length ? '…' : '',
    ].join('');
}

// The sections of the documentation, one group of results each.
const sections = computed(() => props.navigation?.filter(item => item.children?.length) ?? []);

// The navigation item of a page and the items above it.
function findPage(path, items = props.navigation ?? [], ancestors = []) {
    for (const item of items) {
        if (item.path === path) return { item, ancestors };
        if (item.children?.length) {
            const found = findPage(path, item.children, [...ancestors, item]);
            if (found) return found;
        }
    }
    return undefined;
}

const sectionOf = path => sections.value.find(section => path === section.path || path.startsWith(`${section.path}/`));

// One entry of the dialogue. Below a group the documentation section is
// its heading, so the path of the entry starts below it; the exact
// matches stand before the groups and name it.
function toItem({ result, text, phrase }, exact = false) {
    const path = result.id.split('#')[0];
    const page = findPage(path);
    const above = (page?.ancestors ?? []).slice(exact ? 0 : 1).map(item => item.title);
    const prefix = [...new Set([...above, ...result.titles].filter(Boolean))];
    const found = exact && phrase && passage(text, phrase);
    return {
        label: result.title,
        prefix: prefix.length ? `${prefix.join(' > ')} >` : undefined,
        description: text,
        descriptionHtml: found || (result.snippets?.content ? sanitize(result.snippets.content) : undefined),
        to: result.id,
        icon: page?.item?.icon || (result.level > 1 ? appConfig.ui.icons.hash : appConfig.ui.icons.file),
    };
}

// A result the pilot typed the words of: the whole term in the title, or
// in the text of the section. A title says more than a mention in the
// text, so it comes first in the group.
function exactness(result, text, phrase) {
    if (!phrase) return 0;
    if (collapse(result.title).toLowerCase().includes(phrase)) return 2;
    if (phrase.includes(' ') && text.toLowerCase().includes(phrase)) return 1;
    return 0;
}

async function runSearch(term) {
    const query = term.trim();
    if (!query) {
        results.value = [];
        return;
    }
    const found = await searchSections(query, { limit: LIMIT, snippet: { columns: ['content'], around: 20 } });
    const phrase = collapse(query).toLowerCase();
    results.value = found.map(result => {
        const text = collapse(result.content);
        return { result, text, exact: exactness(result, text, phrase), phrase };
    });
}

let pending;
watch(searchTerm, term => {
    clearTimeout(pending);
    pending = setTimeout(() => runSearch(term), 150);
});

const groups = computed(() => {
    if (!results.value.length) return [];
    const best = results.value.filter(found => found.exact)
        .sort((a, b) => b.exact - a.exact);
    const groups = best.length
        ? [{
            id: 'best-match',
            label: 'Exact match',
            ignoreFilter: true,
            items: best.map(found => toItem(found, true)),
        }]
        : [];

    const rest = results.value.filter(found => !found.exact);
    const grouped = new Set();
    for (const section of sections.value) {
        const items = rest.filter(found => sectionOf(found.result.id.split('#')[0]) === section);
        items.forEach(found => grouped.add(found));
        if (items.length) {
            groups.push({
                id: section.path,
                label: section.title,
                ignoreFilter: true,
                items: items.map(found => toItem(found)),
            });
        }
    }

    // A page outside the navigation still gets its result shown.
    const others = rest.filter(found => !grouped.has(found));
    if (others.length) {
        groups.push({ id: 'other', ignoreFilter: true, items: others.map(found => toItem(found)) });
    }
    return groups;
});

const links = computed(() => sections.value.map(section => ({
    label: section.title,
    icon: section.icon,
    to: section.children[0].path,
})));
</script>

<template>
    <LazyUContentSearch
        v-model:search-term="searchTerm"
        :groups="groups"
        :links="links"
        :loading="status === 'loading'"
        :color-mode="!forcedColorMode"
        preserve-group-order
    />
</template>
