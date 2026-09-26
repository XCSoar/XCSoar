export default defineAppConfig({
    docus: {
        locale: 'en',
        colorMode: 'light',
    },
    navigation: {
        sub: 'header',
    },
    header: {
        title: 'XCSoar Docs',
        logo: {
            light: '/img/graphics/logo.svg',
            dark: '/img/graphics/logo.svg',
            alt: 'XCSoar',
        },
    },
    github: {
        url: 'https://github.com/XCSoar/XCSoar',
        branch: 'master',
        rootDir: 'docs',
    },
    ui: {
        colors: {
            primary: 'xcsoar',
        },
        prose: {
            h2: {
                slots: {
                    base: ['text-3xl'],
                    link: 'text-primary-500',
                },
            },
            // Description lists (LaTeX \begin{description}) are rendered with
            // ::field-group / ::field. Style them like a definition list
            // instead of an API parameter table.
            // Callouts (::tip, ::warning) in the regular text size.
            callout: {
                slots: {
                    base: 'text-base/7 [&_code]:text-sm',
                    icon: 'size-5',
                },
            },
            // Tables take the width of their content, not the whole page,
            // with denser cells so that wide tables fit the text column.
            table: {
                slots: {
                    base: 'w-auto max-w-full',
                },
            },
            th: {
                base: 'px-3 py-2',
            },
            td: {
                base: 'px-3 py-2',
            },
            // Code examples scroll sideways instead of wrapping; a wrapped
            // XML or shell line hides where the real lines end. The PDF
            // keeps the wrap, it cannot scroll (public/print.css).
            pre: {
                slots: {
                    base: 'whitespace-pre',
                },
            },
            fieldGroup: {
                base: 'my-5 space-y-4 divide-y-0',
            },
            field: {
                slots: {
                    root: 'my-0 scroll-mt-[calc(48px+45px+var(--ui-header-height))] lg:scroll-mt-[calc(48px+var(--ui-header-height))]',
                    container: 'font-sans text-base',
                    name: 'font-semibold text-highlighted',
                    // A field description keeps its paragraphs (see
                    // app/components/ProseField.vue); they sit tight
                    // under the name and apart from each other.
                    description: 'mt-1 ml-5 text-base text-default [&_code]:text-sm [&_p]:my-0 [&_p+p]:mt-3',
                },
            },
        },
    },
});
