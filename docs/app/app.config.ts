export default defineAppConfig({
    docus: {
        locale: 'en',
        colorMode: 'light',
    },
    navigation: {
        sub: 'header',
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
            fieldGroup: {
                base: 'my-5 space-y-4 divide-y-0',
            },
            field: {
                slots: {
                    root: 'my-0 scroll-mt-[calc(48px+45px+var(--ui-header-height))] lg:scroll-mt-[calc(48px+var(--ui-header-height))]',
                    container: 'font-sans text-base',
                    name: 'font-semibold text-highlighted',
                    description: 'mt-1 ml-5 text-base text-default [&_code]:text-sm',
                },
            },
        },
    },
});
