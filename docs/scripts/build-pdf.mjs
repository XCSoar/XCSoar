// Print one documentation section to PDF.
//
// Serves the generated pages from .output/public (run `npx nuxt generate`
// first), opens /print/<section> in headless Chromium, paginates it with
// Paged.js and writes the PDF. The page scripts are blocked so the
// prerendered HTML is printed as is.
//
//   node scripts/build-pdf.mjs [section] [output.pdf]

import { createServer } from 'node:http';
import { createReadStream, existsSync, mkdirSync, statSync } from 'node:fs';
import { dirname, extname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import puppeteer from 'puppeteer';

const docsDir = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const section = process.argv[2] ?? 'manual';
const output = resolve(process.argv[3] ?? join(docsDir, '..', 'output', 'manual', `XCSoar-${section}.pdf`));
const root = join(docsDir, '.output', 'public');

if (!existsSync(join(root, 'print', section, 'index.html'))) {
    console.error(`${root}/print/${section} not found, run "npx nuxt generate" first`);
    process.exit(1);
}

const types = {
    css: 'text/css',
    html: 'text/html',
    jpg: 'image/jpeg',
    js: 'text/javascript',
    json: 'application/json',
    png: 'image/png',
    svg: 'image/svg+xml',
    woff: 'font/woff',
    woff2: 'font/woff2',
};

const server = createServer((request, response) => {
    let file = join(root, decodeURIComponent(new URL(request.url, 'http://localhost').pathname));
    if (existsSync(file) && statSync(file).isDirectory()) file = join(file, 'index.html');
    if (!existsSync(file)) {
        response.writeHead(404);
        response.end();
        return;
    }
    response.writeHead(200, { 'content-type': types[extname(file).slice(1)] ?? 'application/octet-stream' });
    createReadStream(file).pipe(response);
});
await new Promise(done => server.listen(0, '127.0.0.1', done));

const browser = await puppeteer.launch({
    executablePath: process.env.PUPPETEER_EXECUTABLE_PATH,
    // Chromium refuses to start its sandbox as root (CI containers).
    args: process.getuid?.() === 0 ? ['--no-sandbox'] : [],
});
try {
    const page = await browser.newPage();
    await page.setRequestInterception(true);
    page.on('request', request => (request.resourceType() === 'script' ? request.abort() : request.continue()));
    await page.goto(`http://127.0.0.1:${server.address().port}/print/${section}`, { waitUntil: 'networkidle0' });
    await page.evaluate(() => document.fonts.ready);

    await page.evaluate(() => { window.PagedConfig = { auto: false }; });
    const pagedRoot = join(dirname(fileURLToPath(import.meta.resolve('pagedjs'))), '..');
    await page.addScriptTag({ path: join(pagedRoot, 'dist', 'paged.polyfill.js') });
    const pages = await page.evaluate(async () => {
        // Paginate the print view alone, with only the print stylesheet.
        // Chromium renders @page margin boxes itself, so the page's own
        // copy of the stylesheet has to go before Paged.js adds its output.
        document.querySelector('link[href="/print.css"]')?.remove();
        document.body.replaceChildren(document.querySelector('.print-root'));
        const flow = await window.PagedPolyfill.preview(undefined, ['/print.css']);
        return flow.total;
    });

    mkdirSync(dirname(output), { recursive: true });
    await page.pdf({ path: output, preferCSSPageSize: true, printBackground: true, outline: true, timeout: 0 });
    console.log(`${output}: ${pages} pages`);
} finally {
    await browser.close();
    server.close();
}
