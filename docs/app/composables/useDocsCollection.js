// The content collection of the current language. Docus names them
// docs_<code>, see content.config.ts; without @nuxtjs/i18n there is one
// collection called docs.
export function useDocsCollection() {
    const { locale, isEnabled } = useDocusI18n();
    return computed(() => (isEnabled.value ? `docs_${locale.value}` : 'docs'));
}
