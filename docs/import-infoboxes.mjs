import fs from 'fs';
import path from 'path';
import { kebabCase } from 'scule';

const SRC_DIR = '../src/InfoBoxes/Content';

const FACTORY_FILE = path.join(SRC_DIR, 'Factory.cpp');
const TYPE_FILE = path.join(SRC_DIR, 'Type.hpp');

const OUT_DIR = 'content/3.infobox';

function readFile(filePath) {
    return fs.readFileSync(filePath, 'utf-8');
}

function readContentFiles() {
    const files = fs.readdirSync(SRC_DIR);

    const basenames = [...new Set(
        files
            .filter(f => f.endsWith('.cpp') || f.endsWith('.hpp'))
            .map(f => f.replace(/\.(cpp|hpp)$/, ''))
    )];

    return basenames.filter(name => name !== 'Factory' && name !== 'Type');
}

function buildCategoryMap(contentFiles) {
    const map = new Map();

    for (const name of contentFiles) {
        for (const ext of ['.cpp', '.hpp']) {
            const filePath = path.join(SRC_DIR, name + ext);

            if (!fs.existsSync(filePath)) continue;

            const text = readFile(filePath);

            // Match Update* function definitions (UpdateInfoBoxXxx or UpdateInfoTaskXxx)
            for (const m of text.matchAll(/void\s+(Update\w+)\s*\(/g)) {
                map.set(m[1], name);
            }

            // Match class/struct definitions: InfoBoxContentXxx, InfoBoxNearestXxx etc.
            // Handle optional 'final' keyword before : or {
            for (const m of text.matchAll(/(?:class|struct)\s+(InfoBox\w+)(?:\s+final)?\s*[:{]/g)) {
                map.set(m[1], name);
            }
        }
    }

    return map;
}

function extractIds(typeText) {
    const start = typeText.indexOf('enum Type');

    if (start === -1) {
        throw new Error('enum Type not found');
    }

    const braceStart = typeText.indexOf('{', start);

    let depth = 0;
    let block = '';

    for (let i = braceStart; i < typeText.length; i++) {
        const c = typeText[i];

        if (c === '{') depth++;
        if (c === '}') depth--;

        block += c;

        if (depth === 0) break;
    }

    const ids = [];

    for (let line of block.split('\n')) {
        line = line.trim();

        if (!line) continue;
        if (line === '{' || line === '};') continue;

        // extract comment
        const commentMatch = line.match(/\/\*\s*(.*?)\s*\*\//);
        const idComment = commentMatch?.[1]?.trim() ?? null;

        // remove comments
        line = line.replace(/\/\*.*?\*\//g, '').trim();
        line = line.replace(/\/\/.*$/, '').trim();

        // remove trailing comma
        line = line.replace(/,$/, '').trim();

        // remove assignment
        line = line.split('=')[0].trim();

        // ignore pure comment leftovers
        if (!line) continue;

        // id is first token before space or tab
        const id = line.split(/\s+/)[0].trim();

        // skip invalid junk
        if (id === 'NUM_TYPES') continue;
        if (id === '{') continue;
        if (id === '}') continue;

        ids.push({
            id,
            idComment,
        });
    }

    return ids;
}

function extractMetaDataBlock(text) {
    const marker = 'static constexpr MetaData meta_data[]';
    const startIndex = text.indexOf(marker);

    if (startIndex === -1) {
        throw new Error('meta_data[] not found');
    }

    const firstBrace = text.indexOf('{', startIndex);

    let depth = 0;

    for (let i = firstBrace; i < text.length; i++) {
        const c = text[i];

        if (c === '{') depth++;
        if (c === '}') depth--;

        if (depth === 0) {
            return text.slice(firstBrace, i + 1);
        }
    }

    throw new Error('Unclosed block');
}

function splitEntries(block) {
    const inner = block.slice(1, -1);

    const entries = [];

    let depth = 0;
    let start = 0;

    for (let i = 0; i < inner.length; i++) {
        const c = inner[i];

        if (c === '{') {
            if (depth === 0) {
                start = i;
            }

            depth++;
        }

        if (c === '}') {
            depth--;

            if (depth === 0) {
                entries.push(inner.slice(start, i + 1));
            }
        }
    }

    return entries;
}

function splitFields(entry) {
    const inner = entry.slice(1, -1);

    const fields = [];

    let current = '';
    let inString = false;
    let escape = false;
    let templateDepth = 0;

    for (let i = 0; i < inner.length; i++) {
        const c = inner[i];

        if (escape) {
            current += c;
            escape = false;
            continue;
        }

        if (c === '\\') {
            current += c;
            escape = true;
            continue;
        }

        if (c === '"') {
            inString = !inString;
            current += c;
            continue;
        }

        if (!inString) {
            if (c === '<') templateDepth++;
            if (c === '>') templateDepth--;
        }

        if (c === ',' && !inString && templateDepth === 0) {
            fields.push(current.trim());
            current = '';
        } else {
            current += c;
        }
    }

    if (current.trim()) {
        fields.push(current.trim());
    }

    return fields;
}

function extractSymbol(handlerField) {
    if (!handlerField) return null;

    // IBFHelper<InfoBoxContentXxx>::Create
    const templateMatch = handlerField.match(/IBFHelper(?:Int)?<(\w+)>/);
    if (templateMatch) return templateMatch[1];

    // UpdateInfoBoxXxx or UpdateInfoTaskXxx
    const updateMatch = handlerField.match(/(Update\w+)/);
    if (updateMatch) return updateMatch[1];

    // Lambda body: new InfoBoxContentXxx(...)
    const lambdaMatch = handlerField.match(/new\s+(InfoBox\w+)\s*\(/);
    if (lambdaMatch) return lambdaMatch[1];

    return null;
}

function clean(field) {
    if (!field) return null;

    // Unwrap N_(...) if present
    let inner = field;
    const nMatch = field.match(/^N_\(([\s\S]*)\)$/);
    if (nMatch) inner = nMatch[1];

    if (inner.trim() === 'NULL') return null;

    // Collect all adjacent C string literals (handles multi-line concatenation)
    const parts = [];
    const re = /"((?:[^"\\]|\\.)*)"/g;
    let m;
    while ((m = re.exec(inner)) !== null) {
        parts.push(m[1]);
    }

    if (parts.length === 0) return inner.trim();

    return parts
        .join('')
        .replace(/\\"/g, '"')
        .replace(/\\\\/g, '\\');
}

function parseEntry(entry) {
    const fields = splitFields(entry);

    return {
        title: clean(fields[0]),
        caption: clean(fields[1]),
        description: clean(fields[2]),
        handler: fields[3]?.trim() ?? null,
    };
}

function ensureDir(dir) {
    fs.mkdirSync(dir, { recursive: true });
}

function escapeYamlString(value) {
    if (value === null || value === undefined) {
        return '';
    }

    let str = String(value);

    str = str.replace(/\r\n/g, '\n');

    str = str
        .replace(/\\/g, '\\\\')
        .replace(/"/g, '\\"')
        .replace(/\n/g, '\\n')
        .replace(/\t/g, '\\t');

    if (
        str.startsWith('-') ||
        str.startsWith(':') ||
        str.startsWith('?') ||
        str.startsWith('@') ||
        str.startsWith('`') ||
        str.includes(':') ||
        str.includes('\\"') ||
        str.includes('\\\\') ||
        str.includes('\\n') ||
        str.trim() === '---'
    ) {
        return `"${str}"`;
    }

    return str;
}

function formatYamlBlock(item) {
    return `title: ${escapeYamlString(item.title ?? '')}
description: ${escapeYamlString(item.description ?? '')}
infoboxIndex: ${escapeYamlString(item.index ?? '')}
infoboxId: ${escapeYamlString(item.id ?? 'unknown')}
infoboxIdComment: ${escapeYamlString(item.idComment ?? '')}
infoboxCaption: ${escapeYamlString(item.caption ?? '')}
infoboxCategory: ${escapeYamlString(item.category ?? '')}`;
}

function main() {
    const contentFiles = readContentFiles();
    console.log('Content files:', contentFiles);

    const factoryText = readFile(FACTORY_FILE);
    const typeText = readFile(TYPE_FILE);
    const categoryMap = buildCategoryMap(contentFiles);

    console.log('Category map entries:', categoryMap.size);

    const ids = extractIds(typeText);
    console.log('IDS:', ids.length);

    const block = extractMetaDataBlock(factoryText);
    const rawEntries = splitEntries(block);
    console.log('ENTRIES:', rawEntries.length);

    const entries = rawEntries.map((entry, index) => {
        const item = parseEntry(entry);
        const symbol = extractSymbol(item.handler);
        const category = symbol ? (categoryMap.get(symbol) ?? null) : null;

        return {
            ...item,
            id: ids[index]?.id ?? null,
            index,
            idComment: ids[index]?.idComment ?? null,
            category,
        };
    });

    ensureDir(OUT_DIR);

    let unknownCategories = 0;

    for (let i = 0; i < entries.length; i++) {
        const item = entries[i];

        if (!item.category) {
            unknownCategories++;
            console.warn(`[WARN] no category for ${item.id} (handler: ${item.handler})`);
        }

        console.log(i, item.id, item.category, item.title);

        const kebabId = kebabCase(item.id ?? 'unknown').replace('e-', '');
        const fileName = `${String(i).padStart(3, '0')}.${kebabId}.md`;
        const filePath = path.join(OUT_DIR, fileName);

        let existingBody = '';

        if (fs.existsSync(filePath)) {
            const existing = fs.readFileSync(filePath, 'utf-8');
            // extract everything after the closing ---
            const match = existing.match(/^---\n[\s\S]*?\n---\n([\s\S]*)$/);
            if (match) {
                existingBody = match[1];
            }
        }

        const content = `---\n${formatYamlBlock(item)}\n---\n${existingBody}`;
        fs.writeFileSync(filePath, content, 'utf-8');
    }

    console.log(`Done: ${entries.length} files written, ${unknownCategories} without category`);
}

main();
