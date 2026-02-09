#!/usr/bin/env python3
"""Batch translate missing .po entries using Ollama local LLM."""

import json
import sys
import time
import urllib.request
import polib

LANG_NAMES = {
    'bg': 'Bulgarian', 'ca': 'Catalan', 'cs': 'Czech', 'da': 'Danish',
    'de': 'German', 'el': 'Greek', 'es': 'Spanish', 'fi': 'Finnish',
    'fr': 'French', 'he': 'Hebrew', 'hr': 'Croatian', 'hu': 'Hungarian',
    'it': 'Italian', 'ja': 'Japanese', 'ko': 'Korean', 'lt': 'Lithuanian',
    'nb': 'Norwegian Bokmål', 'nl': 'Dutch', 'pl': 'Polish',
    'pt': 'Portuguese', 'pt_BR': 'Brazilian Portuguese', 'ro': 'Romanian',
    'ru': 'Russian', 'sk': 'Slovak', 'sl': 'Slovenian', 'sr': 'Serbian',
    'sv': 'Swedish', 'tr': 'Turkish', 'uk': 'Ukrainian',
    'vi': 'Vietnamese', 'zh_CN': 'Chinese Simplified',
    'zh_Hant': 'Chinese Traditional',
}

OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL = "gemma2:2b"
BATCH_SIZE = 15  # strings per API call


def call_ollama(prompt: str) -> str:
    """Call Ollama API and return response text."""
    data = json.dumps({
        "model": MODEL,
        "prompt": prompt,
        "stream": False,
        "options": {"temperature": 0.1, "num_predict": 2048}
    }).encode()
    req = urllib.request.Request(
        OLLAMA_URL, data=data,
        headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            result = json.loads(resp.read().decode())
            return result.get("response", "")
    except Exception as e:
        print(f"  API error: {e}", file=sys.stderr)
        return ""


def translate_batch(strings: list[str], lang_name: str) -> dict[str, str]:
    """Translate a batch of strings and return {english: translation} dict."""
    numbered = "\n".join(f"{i+1}. {s}" for i, s in enumerate(strings))
    prompt = (
        f"Translate these English strings to {lang_name}. "
        f"This is for a glider/soaring flight computer application (XCSoar). "
        f"Use appropriate aviation terminology. "
        f"Keep abbreviations like FAI, AAT, FLARM, ADSB, PCAS, VOR, NDB, "
        f"CHT, EGT, RPM, XPDR, WPT, ETA unchanged. "
        f"Return ONLY numbered translations in the same format, "
        f"one per line, no explanations:\n\n{numbered}"
    )

    response = call_ollama(prompt)
    if not response:
        return {}

    results = {}
    for line in response.strip().split("\n"):
        line = line.strip()
        if not line:
            continue
        # Parse "1. translation" or "1) translation"
        for sep in ['. ', ') ', ': ']:
            parts = line.split(sep, 1)
            if len(parts) == 2:
                try:
                    idx = int(parts[0].strip().rstrip('.')) - 1
                    if 0 <= idx < len(strings):
                        translation = parts[1].strip().strip('"\'')
                        if translation and translation != strings[idx]:
                            results[strings[idx]] = translation
                    break
                except ValueError:
                    continue
    return results


def get_untranslated(po_path: str) -> list:
    """Get untranslated entries from a .po file."""
    po = polib.pofile(po_path)
    untranslated = []
    for entry in po.untranslated_entries():
        # Skip empty msgid, plural forms, and pure format strings
        if not entry.msgid or entry.msgid_plural:
            continue
        if entry.msgid in ('%s', '%s: %s', '%s / %s', '%d', '%u'):
            continue
        untranslated.append(entry)
    return untranslated


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <lang_code> [lang_code...]")
        print(f"Available: {', '.join(sorted(LANG_NAMES.keys()))}")
        sys.exit(1)

    languages = sys.argv[1:]

    for lang in languages:
        if lang not in LANG_NAMES:
            print(f"Unknown language: {lang}")
            continue

        lang_name = LANG_NAMES[lang]
        po_path = f"po/{lang}.po"

        print(f"\n{'='*60}")
        print(f"Processing {lang_name} ({lang}) - {po_path}")
        print(f"{'='*60}")

        untranslated = get_untranslated(po_path)
        if not untranslated:
            print(f"  No untranslated entries found!")
            continue

        print(f"  Found {len(untranslated)} untranslated entries")

        po = polib.pofile(po_path)
        translated_count = 0
        failed_count = 0

        # Process in batches
        strings = [e.msgid for e in untranslated]
        for i in range(0, len(strings), BATCH_SIZE):
            batch = strings[i:i+BATCH_SIZE]
            batch_num = i // BATCH_SIZE + 1
            total_batches = (len(strings) + BATCH_SIZE - 1) // BATCH_SIZE
            print(f"  Batch {batch_num}/{total_batches} "
                  f"({len(batch)} strings)...", end=" ", flush=True)

            start = time.time()
            translations = translate_batch(batch, lang_name)
            elapsed = time.time() - start

            # Apply translations
            for entry in po:
                if entry.msgid in translations and not entry.msgstr:
                    entry.msgstr = translations[entry.msgid]
                    translated_count += 1

            got = len(translations)
            missed = len(batch) - got
            failed_count += missed
            print(f"got {got}/{len(batch)} in {elapsed:.1f}s")

            # Save after each batch (crash safety)
            po.save(po_path)

        print(f"\n  Done: {translated_count} translated, "
              f"{failed_count} failed/skipped")
        print(f"  Saved to {po_path}")


if __name__ == "__main__":
    main()
