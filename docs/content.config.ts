import { defineContentConfig, defineCollection, z } from '@nuxt/content'

export default defineContentConfig({
    collections: {
        docs: defineCollection({
            type: 'page',
            source: '**/*.{md,yaml}',
            schema: z.object({
                // InfoBox metadata imported by scripts/import-infoboxes.mjs.
                infobox: z.object({
                    index: z.number(),
                    id: z.string(),
                    comment: z.string().optional(),
                    caption: z.string(),
                    help: z.string().optional(),
                    category: z.string().optional(),
                }).optional(),
            }),
        }),
    },
})
