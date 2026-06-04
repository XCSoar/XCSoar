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
            fields: ['description'],
        });
    }),
]);

const isIndexPage = computed(() => {
    return page.value.id === 'docs/3.infobox/0.index.md';
});

if (!page.value) {
  throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const github = computed(() => appConfig.github ? appConfig.github : null);
const headline = ref(findPageHeadline(navigation?.value, page.value?.path));

const details = [
    {
        label: 'Index',
        value: page.value.infoboxIndex,
    },
    {
        label: 'ID',
        value: page.value.infoboxId,
    },
    {
        label: 'Category',
        value: page.value.infoboxCategory,
    },
    {
        label: 'Name',
        value: page.value.title,
    },
    {
        label: 'Caption',
        value: page.value.infoboxCaption,
    },
    {
        label: 'Description',
        value: page.value.description,
    },
    {
        label: 'ID Comment',
        value: page.value.infoboxIdComment,
    },
];
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
            <UCard v-if="!isIndexPage" class="my-4">
                <dl class="divide-y divide-(--ui-border)">
                    <div v-for="item in details" :key="item.label" class="flex py-3 gap-4">
                        <dt class="font-bold w-1/4 shrink-0">{{ item.label }}</dt>
                        <dd>
                            <ProseCode v-if="['Index', 'ID'].includes(item.label)">{{ item.value }}</ProseCode>
                            <template v-else>{{ item.value }}</template>
                        </dd>
                    </div>
                </dl>
            </UCard>

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
            <UContentSurround :surround="surround" />
        </UPageBody>

        <template #right>
            <DocsAsideRight :page="page"/>
        </template>
    </UPage>
</template>
