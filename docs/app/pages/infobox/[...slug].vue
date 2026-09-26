<script setup>
import { findPageHeadline } from '@nuxt/content/utils';
import { kebabCase } from 'scule'

const navigation = inject('navigation');

const route = useRoute();
const { t } = useDocusI18n();
const appConfig = useAppConfig();

definePageMeta({
  layout: 'docs',
});

const collectionName = useDocsCollection();

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

// Same "Edit this page" link as the Docus page.
const editLink = computed(() => {
    if (!github.value) return;
    return [
        github.value.url,
        'edit',
        github.value.branch,
        github.value.rootDir,
        'content',
        `${page.value?.stem}.${page.value?.extension}`,
    ].filter(Boolean).join('/');
});
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
            <InfoboxSummary v-if="!isIndexPage" :title="page.title" :infobox="infobox" />

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
