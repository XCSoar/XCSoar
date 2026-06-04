import { defineContentConfig, defineCollection, z } from '@nuxt/content'

export default defineContentConfig({
    collections: {
        docs: defineCollection({
            type: 'page',
            source: '**/*.md',
            schema: z.object({
                infoboxIndex: z.number().optional(),
                infoboxId: z.string().optional(),
                infoboxIdComment: z.string().optional(),
                infoboxCategory: z.string().optional(),
                infoboxCaption: z.string().optional(),
            }),
        }),
    },
})
