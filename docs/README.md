# XCSoar documentation

The user manual, the quick guide, the InfoBox reference, the hardware pages and
the developer documentation, written in markdown and built with
[Nuxt](https://nuxt.com), [Nuxt Content](https://content.nuxt.com) and
[Docus](https://docus.dev). The same content is printed to the PDF manuals.

The conventions for writing and extending the documentation (files and
frontmatter, markup, links, components, screenshots) are in
`.cursor/rules/user-manual.mdc`. Read them before adding or changing a page.



## Build

Node 22 or newer.

```bash
cd docs
npm ci
npm run dev                  # http://localhost:3000, live reload
npm run build:docs           # the pages and the PDFs, ready to deploy
npm run generate             # the pages alone, in .output/public
npm run pdf -- manual        # one section as PDF, in .output/public/pdfs
```

The PDFs are written next to the pages, so the documentation offers the
manuals of the version it was built from. Deploy what `npm run build:docs`
leaves in `.output/public`; after `npm run generate` alone the links to the
manuals lead nowhere, and so they do under `npm run dev`.

`npm run pdf` needs the generated pages. It serves `.output/public`, opens
`/print/<section>` in headless Chromium (Puppeteer) with scripts blocked and
paginates it with Paged.js; the print styles are `public/print.css`. The PDF
is written next to the pages, so the documentation offers the manuals of the
version it was built from. Set `PUPPETEER_EXECUTABLE_PATH` to use an installed
Chromium instead of the one Puppeteer downloads.

The workflow `.github/workflows/build-docs.yml` builds the pages and the PDFs on
every change to `docs/` and uploads the PDFs as artifact. For a release it also
attaches the PDFs to the GitHub release, copies them to the download server and
copies the pages to the documentation server with rsync over ssh, using the
repository secrets `DOCS_HOST`, `DOCS_SSH_USER`, `DOCS_SSH_KEY` and
`DOCS_REMOTE_PATH`; the optional `DOCS_SSH_KNOWN_HOSTS` pins the host key.



## Layout

| Path | Content |
|------|---------|
| `content/` | All pages, markdown with frontmatter; the folder and file names give the URLs |
| `content/index.md` | Landing page |
| `content/1.manual/` | User manual, one folder per chapter |
| `content/1.manual/15.license.md` | Frontmatter only; the page shows `COPYING` verbatim, inserted by `modules/license.ts` |
| `content/2.quick-guide/` | Quick guide |
| `content/3.infobox/` | InfoBox reference, frontmatter generated (see below) |
| `content/4.hardware/`, `content/5.dev/` | Hardware and developer pages |
| `content-history.json` | Last commit of the sources each migrated page came from (see below) |
| `app/` | Components, layouts and configuration of Nuxt |
| `app/pages/print/[section].vue` | The print view of one section, input of the PDF |
| `modules/` | Local Nuxt modules run at build time: the licence page, the "Last updated" line, the permalink redirects |
| `public/print.css` | Page boxes, headers and print layout for the PDF |
| `scripts/` | Build and import scripts |

For now the images are not stored in `docs/`; `nuxt.config.ts` serves them from
the directories the LaTeX manual used. This will change when those sources are
removed:

| URL | Directory |
|-----|-----------|
| `/img/figures/` | `doc/manual/en/figures` (screenshots) |
| `/img/drawings/` | `doc/manual/figures` (gesture and icon drawings) |
| `/img/icons/` | `Data/icons` (map icons) |



## Scripts

- `scripts/import-infoboxes.mjs` writes the frontmatter of
  `content/3.infobox/*.md` from `src/InfoBoxes/Content/Factory.cpp` and keeps
  the markdown body of each page. Run it after changing an InfoBox.
- `scripts/build-pdf.mjs [section] [output.pdf]` prints one section, see above.
- `scripts/import-history.mjs` collects `content-history.json` from the LaTeX
  and RST sources at the commit named in its `BASE` constant. It is run once
  more with the final `BASE` before those sources are deleted, and never
  afterwards; the file is part of the migration.



## Permalinks

The XCSoar app links into the documentation with short URLs that survive
renamed pages: `/go/<id>`, with an anchor appended where needed
(`/go/map-display#conf-waypointicons`). A page gets its id with `permalink:`
in the frontmatter, only when the app links to it. `nuxt generate` writes the
redirects to `.output/public/.htaccess` and fails on a duplicate or invalid
id. Apache serves them as 302, so a changed target is not cached by browsers;
the deployed directory needs `AllowOverride FileInfo`. The dev server does not
redirect.



## Version badge and "Last updated"

Both come from git at build time (`nuxt.config.ts` and `modules/page-meta.ts`),
so they are only correct in a build from a git checkout.

- The badge in the header shows the release version when `HEAD` carries a
  `v<version>` tag, otherwise the short commit hash, and links to the release or
  commit on GitHub. It shows `HEAD` without a link when `docs/` has uncommitted
  changes.
- The line below a page names the author and date of the last commit that
  touched the page, with a badge linking to the commit. Applying a patch with
  `git apply` does not change the line; only a commit does.
- For pages migrated from the LaTeX manual or the RST files,
  `content-history.json` provides the last commit of the original source
  instead, until the page is changed after the last commit of that file. Before
  the LaTeX and RST sources are deleted, set the `BASE` constant in
  `scripts/import-history.mjs` to their last commit on master, run the script
  once more and commit the result, so the history keeps pointing at commits that
  still contain the sources.
