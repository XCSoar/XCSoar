<script setup>
import { findPageHeadline } from '@nuxt/content/utils';
import { kebabCase } from 'scule'

const navigation = inject('navigation');

const route = useRoute();
const { locale, isEnabled, t } = useDocusI18n();
const appConfig = useAppConfig();

definePageMeta({
  layout: 'docs',
});

const collectionName = computed(() => isEnabled.value ? `docs_${locale.value}` : 'docs');

const [{ data: page }, { data: surround }] = await Promise.all([
    useAsyncData(kebabCase(route.path), () => queryCollection(collectionName.value).path(route.path).first()),
    useAsyncData(`${kebabCase(route.path)}-surround`, () => {
        return queryCollectionItemSurroundings(collectionName.value, route.path, {
            fields: ['infobox'],
        });
    }),
]);

if (!page.value) {
  throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const isIndexPage = computed(() => {
    return page.value.id === 'docs/3.infobox/0.index.md';
});

const github = computed(() => appConfig.github ? appConfig.github : null);
const headline = ref(findPageHeadline(navigation?.value, page.value?.path));

const infobox = computed(() => page.value.infobox ?? {});

useSeo({
    title: page.value.title,
    description: infobox.value.help ?? page.value.description,
    type: 'article',
});

// The surround cards show the help text where Docus shows the description.
const surroundLinks = computed(() => (surround.value ?? []).map(link => {
    return link ? { ...link, description: link.infobox?.help } : link;
}));

// Facts shown below the help text. Name is the title in the InfoBox
// configuration dialogue, caption the label drawn in the InfoBox itself.
const facts = computed(() => [
    { label: 'Name', value: page.value.title },
    { label: 'Caption', value: infobox.value.caption },
    { label: 'Category', value: infobox.value.category },
    { label: 'ID', value: infobox.value.id, code: true },
    { label: 'Index', value: infobox.value.index, code: true },
]);
</script>

<template>
    <UPage v-if="page">
        <UPageHeader
            :title="page.title"
            :description="null"
            :headline="headline"
            :ui="{ wrapper: 'flex-row items-center flex-wrap justify-between', }"
        >
            <template #links>
                <UButton v-for="(link, index) in page.links" :key="index" size="sm" v-bind="link" />
                <DocsPageHeaderLinks />
            </template>
        </UPageHeader>

        <UPageBody>
            <template v-if="!isIndexPage">
                <div class="my-6 flex flex-col gap-6 sm:flex-row sm:items-start">
                    <InfoBoxPreview :caption="infobox.caption" />
                    <div class="min-w-0 flex-1">
                        <div class="mb-3 flex flex-wrap items-center gap-2">
                            <UBadge v-if="infobox.category" color="primary" variant="subtle">{{ infobox.category }}</UBadge>
                            <UBadge color="neutral" variant="outline" class="font-mono">{{ infobox.id }}</UBadge>
                        </div>
                        <p class="text-base/7 text-default">{{ infobox.help }}</p>
                    </div>
                </div>

                <table class="my-6 w-full text-sm">
                    <tbody class="divide-y divide-default">
                        <tr v-for="fact in facts" :key="fact.label">
                            <th scope="row" class="w-40 py-2 pr-4 text-left font-medium text-muted">{{ fact.label }}</th>
                            <td class="py-2">
                                <code v-if="fact.code" class="rounded-md bg-muted px-1.5 py-0.5 font-mono text-sm">{{ fact.value }}</code>
                                <template v-else>{{ fact.value }}</template>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </template>

            <ContentRenderer v-if="page" :value="page" />

            <USeparator v-if="github">
                <div class="flex items-center gap-2 text-sm text-muted">
                    <UButton
                        variant="link"
                        color="neutral"
                        :to="editLink"
                        target="_blank"
                        icon="i-lucide-pen"
                        :ui="{ leadingIcon: 'size-4' }"
                    >
                        {{ t('docs.edit') }}
                    </UButton>
                    <template v-if="github?.url">
                        <span>{{ t('common.or') }}</span>
                        <UButton
                            variant="link"
                            color="neutral"
                            :to="`${github.url}/issues/new/choose`"
                            target="_blank"
                            icon="i-lucide-alert-circle"
                            :ui="{ leadingIcon: 'size-4' }"
                        >
                            {{ t('docs.report') }}
                        </UButton>
                    </template>
                </div>
            </USeparator>
            <UContentSurround :surround="surroundLinks" />
        </UPageBody>

        <template #right>
            <DocsAsideRight :page="page"/>
        </template>
    </UPage>
</template>
