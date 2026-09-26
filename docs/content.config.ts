import { defineContentConfig, defineCollection, z } from '@nuxt/content'
import { useNuxt } from '@nuxt/kit'
import { join } from 'node:path'

// One collection per language below content/, named as Docus queries
// them (docs_<code>, landing_<code>). The default language keeps the
// bare paths (/manual/…), another language lives below its code
// (/de/manual/…), like the routes of @nuxtjs/i18n; see nuxt.config.ts.
const { options } = useNuxt()
const cwd = join(options.rootDir, 'content')
const defaultLocale = options.i18n?.defaultLocale ?? 'en'
const locales = (options.i18n?.locales ?? [defaultLocale])
    .map(locale => (typeof locale === 'string' ? locale : locale.code))

const schema = z.object({
    // The id of the page in /go/<id> links from the XCSoar app,
    // see nuxt.config.ts.
    permalink: z.string().optional(),
    // InfoBox metadata imported by scripts/import-infoboxes.mjs.
    infobox: z.object({
        index: z.number(),
        id: z.string(),
        comment: z.string().optional(),
        caption: z.string(),
        help: z.string().optional(),
        category: z.string().optional(),
        demo: z.object({
            title: z.string().optional(),
            value: z.string().optional(),
            unit: z.string().optional(),
            comment: z.string().optional(),
            color: z.string().optional(),
            commentColor: z.string().optional(),
            graphic: z.string().optional(),
        }).optional(),
    }).optional(),
})

const collections = Object.fromEntries(locales.flatMap((code) => {
    const prefix = code === defaultLocale ? '/' : `/${code}`
    return [
        [`landing_${code}`, defineCollection({
            type: 'page',
            source: { cwd, include: `${code}/index.md`, prefix },
        })],
        [`docs_${code}`, defineCollection({
            type: 'page',
            source: {
                cwd,
                include: `${code}/**/*.{md,yaml}`,
                exclude: [`${code}/index.md`],
                prefix,
            },
            schema,
        })],
    ]
}))

export default defineContentConfig({ collections })
