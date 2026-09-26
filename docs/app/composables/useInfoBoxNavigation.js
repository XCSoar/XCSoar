export function useInfoBoxNavigation() {
    const collection = useDocsCollection();
    const { data: navigationData } = useAsyncData('infobox-navigation', () => {
        return queryCollectionNavigation(collection.value, ['infobox']);
    });

    const navigationByCategory = computed(() => {
        const tree = navigationData?.value ?? [];

        const section = tree.find(item => {
            return item.path === '/infobox' || item.path?.startsWith('/infobox');
        });

        const pages = section?.children ?? [];
        const groups = new Map();

        for (const page of pages) {
            const cat = page.infobox?.category ?? 'other';
            if (cat === 'hidden') continue;
            if (!groups.has(cat)) {
                groups.set(cat, {
                    title: cat.charAt(0).toUpperCase() + cat.slice(1),
                    path: `/infobox/${cat}`,
                    children: [],
                });
            }

            groups.get(cat).children.push({
                title: page.title ?? '',
                path: page.path ?? '',
                infoboxIndex: page.infobox?.index ?? '',
                infoboxCaption: page.infobox?.caption ?? '',
            });
        }

        return [...groups.values()].sort((a, b) => {
            return  (a.title ?? '').localeCompare(b.title ?? '');
        });
    });

    return { navigationByCategory };
}
