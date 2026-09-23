// Collects the last commit of the sources the pages in content/ were
// migrated from (LaTeX manual, RST developer docs, InfoBox metadata in
// Factory.cpp) into content-history.json. nuxt.config.ts shows that
// commit below a page until the page is changed after the last commit
// of content-history.json, so commit the file as the last step of the
// migration and do not regenerate it afterwards. Run it on a full
// clone; a shallow clone cuts the history.
//
//   node scripts/import-history.mjs

import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const DOCS_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const REPO_DIR = path.join(DOCS_DIR, '..');
const CONTENT_DIR = path.join(DOCS_DIR, 'content');
const OUT_FILE = path.join(DOCS_DIR, 'content-history.json');

// Last commit on master with the sources in place. The sources are
// read from this commit, so the script keeps working after doc/ is
// removed and after the docs branch is rebased.
const BASE = '3a6bbf84753efa13c377480c398ba63cc05e2f7f';

const MANUAL_DIR = 'doc/manual/en';

// Manual chapter folder to LaTeX chapter file.
const CHAPTERS = {
    '01.introduction': 'ch01_introduction.tex',
    '02.installation': 'installation.tex',
    '03.user-interface': 'ch02_user_interface.tex',
    '04.navigation': 'ch03_navigation.tex',
    '05.tasks': 'ch04_xc_tasks.tex',
    '06.glide-computer': 'ch05_glide_computer.tex',
    '07.atmosphere': 'ch06_atmosphere_and_instruments.tex',
    '08.airspace': 'ch07_airspace_and_flarm.tex',
    '09.integration': 'integration.tex',
    '10.avionics-and-airframe': 'ch08_avionics_and_airframe.tex',
    '11.quickstart': 'ch09_quickstart.tex',
    '12.configuration': 'ch11_configuration.tex',
    '13.data-files': 'ch12_data_files.tex',
    '14.history': 'ch13_history.tex',
};

function git(...args) {
    try {
        return execFileSync('git', args, { cwd: REPO_DIR, stdio: ['ignore', 'pipe', 'ignore'] })
            .toString().trim();
    } catch {
        return '';
    }
}

// First line of a git log output; -L appends the diff of the range.
function parseCommit(output) {
    const [commit, date, author] = output.split('\n')[0].split('\t');
    return commit ? { commit, date, author } : null;
}

// Last commit that touched the whole file.
function fileCommit(file) {
    return parseCommit(git('log', '-1', '--format=%H%x09%as%x09%an', BASE, '--', file));
}

// Last commit that touched the lines from the start regex up to, but
// not including, the end regex, or up to the end of the file when the
// end regex does not match. "+N" as end takes N lines from the start.
// git blame instead of git log -L, because -L includes the end line,
// which is the first line of the next entry.
function rangeCommit(file, start, end) {
    let lines;
    if (end.startsWith('+')) {
        lines = blameLines(file, `/${start}/,${end}`);
    } else {
        lines = blameLines(file, `/${start}/,/${end}/`).slice(0, -1);
        if (!lines.length) lines = blameLines(file, `/${start}/,+99999`);
    }
    const newest = lines.sort((a, b) => b.time - a.time)[0];
    return newest ? parseCommit(git('log', '-1', '--format=%H%x09%as%x09%an', newest.commit)) : null;
}

// Commit and author time of every line of a blame range. The porcelain
// output names the author only the first time a commit appears.
function blameLines(file, range) {
    const times = new Map();
    const lines = [];
    let commit;
    for (const line of git('blame', '--porcelain', '-L', range, BASE, '--', file).split('\n')) {
        const header = /^([0-9a-f]{40}) \d+ \d+/.exec(line);
        if (header) commit = header[1];
        else if (line.startsWith('author-time ')) times.set(commit, Number(line.slice(12)));
        else if (line.startsWith('\t')) lines.push({ commit, time: times.get(commit) });
    }
    return lines;
}

// Escapes text for a git -L basic regular expression.
function regexEscape(text) {
    return text.replace(/[\\.*^$/[\]]/g, '\\$&');
}

// Escapes a title for a git -L basic regular expression, with the
// characters LaTeX writes with a backslash.
function regexTitle(title) {
    return regexEscape(title).replace(/[&%#_]/g, '\\\\$&');
}

// The developer pages are named after their content, the RST files they
// came from after their topic; these are the ones that differ.
const RST_FILES = {
    environment: 'devsetup',
    translations: 'i18n',
    map_file: 'mapfile',
    test_utilities: 'test_debug_utilities',
};

// Matches letters in either case, for titles the LaTeX manual spells
// differently.
function anyCase(regex) {
    return regex.replace(/[a-zA-Z]/g, c => `[${c.toUpperCase()}${c.toLowerCase()}]`);
}

function latest(commits) {
    return commits.filter(Boolean).sort((a, b) => b.date.localeCompare(a.date))[0] ?? null;
}

// Commit of the LaTeX section with one of the titles, or null.
function sectionCommit(file, titles) {
    const found = [];
    for (const title of titles) {
        const escaped = regexTitle(title);
        for (const start of [`\\\\section{${escaped}}`, `\\\\section\\*{${escaped}}`, `\\\\subsection{${escaped}}`]) {
            const commit = rangeCommit(file, start, '^\\\\section');
            if (commit) {
                found.push(commit);
                break;
            }
        }
    }
    return latest(found);
}

// A frontmatter value, unquoted.
function frontmatter(text, key) {
    const value = text.match(new RegExp(`^ *${key}: (.*)$`, 'm'))?.[1] ?? '';
    return value.startsWith('"') ? value.slice(1, -1).replace(/\\(["\\])/g, '$1') : value;
}

function readPage(file) {
    const text = fs.readFileSync(file, 'utf8');
    const headings = [...text.matchAll(/^## (.*)$/gm)].map(m => m[1]);
    return {
        title: frontmatter(text, 'title'),
        caption: frontmatter(text, 'caption'),
        help: frontmatter(text, 'help'),
        headings,
    };
}

// Source commit of one page, or null when the source has no usable history.
function pageCommit(relative) {
    const [section, ...rest] = relative.split('/');
    const page = readPage(path.join(CONTENT_DIR, relative));

    if (section === '1.manual') {
        if (rest[0] === '00.index.md') {
            const file = `${MANUAL_DIR}/XCSoar-manual.tex`;
            return { file, commit: rangeCommit(file, '\\\\chapter\\*{Preface}', '^\\\\chapter{') };
        }
        if (rest[0] === '15.license.md') {
            const file = `${MANUAL_DIR}/gpl.tex`;
            return { file, commit: fileCommit(file) };
        }
        const file = `${MANUAL_DIR}/${CHAPTERS[rest[0]]}`;
        // A page numbered 0 holds the text between \\chapter and the
        // first \\section.
        if (/^0+\./.test(rest[1])) return { file, commit: rangeCommit(file, '\\\\chapter{', '^\\\\section') };
        const commit = sectionCommit(file, [page.title, ...page.headings]);
        if (!commit) console.warn(`[FILE] ${relative}: no section matches, using the whole file`);
        return { file, commit: commit ?? fileCommit(file) };
    }

    if (section === '2.quick-guide') {
        const file = `${MANUAL_DIR}/XCSoar-in-a-flash.tex`;
        if (rest[0] === '1.index.md') return { file, commit: fileCommit(file) };
        const commit = sectionCommit(file, [page.title]);
        if (!commit) console.warn(`[FILE] ${relative}: no section matches, using the whole file`);
        return { file, commit: commit ?? fileCommit(file) };
    }

    if (section === '3.infobox') {
        if (rest[0] === '0.index.md') {
            const file = `${MANUAL_DIR}/ch10_infobox_reference.tex`;
            return { file, commit: fileCommit(file) };
        }
        // The \\ibi{name}{caption}{help} entry of the LaTeX reference,
        // found by name or caption. Some source strings end with a space.
        const tex = `${MANUAL_DIR}/ch10_infobox_reference.tex`;
        const end = '^\\\\ibi\\|^\\\\section';
        const name = anyCase(regexTitle(page.title));
        const caption = anyCase(regexTitle(page.caption));
        const entry = rangeCommit(tex, `\\\\ibi{${name} *}`, end)
            ?? rangeCommit(tex, `\\\\ibig{${name} *}`, end)
            ?? (page.caption && rangeCommit(tex, `\\\\ibi{[^}]*}{${caption} *}`, end))
            ?? (page.caption && rangeCommit(tex, `\\\\ibig{[^}]*}{${caption} *}`, end));
        if (entry) return { file: tex, commit: entry };

        // Otherwise the line with the help text in the code, or the one
        // with the name when there is no help text.
        const file = 'src/InfoBoxes/Content/Factory.cpp';
        const line = page.help ? page.help.slice(0, 40) : `${page.title}"`;
        return { file, commit: rangeCommit(file, `"${regexEscape(line)}`, '+1') };
    }

    if (section === '5.dev') {
        // The overview came from the website, not from index.rst.
        if (rest[0] === '00.index.md') return null;
        const name = rest[0].replace(/^\d+\./, '').replace(/\.md$/, '').replaceAll('-', '_');
        const file = `doc/${RST_FILES[name] ?? name}.rst`;
        if (!git('rev-parse', '--verify', '--quiet', `${BASE}:${file}`)) return null;
        return { file, commit: fileCommit(file) };
    }

    // Landing page and pages migrated from the website: no history here.
    return null;
}

function* pages(dir = CONTENT_DIR) {
    for (const entry of fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name))) {
        const file = path.join(dir, entry.name);
        if (entry.isDirectory()) yield* pages(file);
        else if (entry.name.endsWith('.md')) yield path.relative(CONTENT_DIR, file);
    }
}

const history = {};
let skipped = 0;

for (const relative of pages()) {
    const found = pageCommit(relative);
    if (!found?.commit) {
        skipped++;
        console.warn(`[SKIP] ${relative}`);
        continue;
    }
    history[relative] = { ...found.commit, source: found.file };
    console.log(relative, found.commit.date, found.commit.author);
}

fs.writeFileSync(OUT_FILE, JSON.stringify(history, null, 2) + '\n');
console.log(`Done: ${Object.keys(history).length} pages written, ${skipped} skipped`);
