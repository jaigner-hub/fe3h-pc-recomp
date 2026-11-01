# From totk-pc-recomp installer/ini.py @ c944f9f94520874f2a669add30dd08d29ee9f7b5 (MIT, Copyright (c) 2026 Valiantsin Dzerakh), renamed to fe3h; FE3H changes are noted where made.
"""fe3h.ini: set the game language and keep everything else (keys and syntax as runtime/cfg.c reads them).

A new file starts from fe3h.ini.example, whose lines are all commented defaults. The language goes into `lang`
(a Switch language code, runtime/ov_system.c): an active `lang = ...` line is rewritten in place, else the commented
`# lang = ...` line of the example is activated, else the line is added to [language] (created at the end if
missing). Other lines, comments and line order stay as they are.
"""
import os
import re
from pathlib import Path

# runtime/ov_system.c ov_language: the codes FE3H_LANG accepts. Each language appears once, under the code the game
# accepts. FE3H itself ships Japanese, English (both), French (both), German, Italian, Spanish (both), Chinese (both)
# and Korean; the others fall back inside the game.
LANGUAGES = ('ja', 'en-US', 'en-GB', 'fr', 'fr-CA', 'de', 'it', 'es', 'es-419', 'nl', 'pt', 'pt-BR', 'ru', 'ko',
             'zh-Hans', 'zh-Hant')
# Older installer builds wrote these two codes; they are read as the canonical ones and rewritten on the next install.
ALIASES = {'zh-CN': 'zh-Hans', 'zh-TW': 'zh-Hant'}
# TotK's Korean and Chinese text fell back to the Switch shared fonts, which the installer does not extract; FE3H
# carries its own fonts (NintendoWare font archives in its RomFS), so no language warns.
NEED_SHARED_FONTS = ()

ACTIVE = re.compile(r'^(\s*)(?:fe3h_)?lang(\s*=\s*)(.*?)(\s*)$', re.IGNORECASE)
COMMENTED = re.compile(r'^\s*[#;]\s*(?:fe3h_)?lang\s*=.*$', re.IGNORECASE)
SECTION = re.compile(r'^\s*\[(.*)\]\s*$')


def canonical_language(lang):
    """The code LANGUAGES lists for lang: an alias of an older installer build is mapped, anything else is kept."""
    return ALIASES.get(lang, lang)


def set_language(text, lang):
    """text with `lang = <lang>` (an alias written as its canonical code); see the module docstring."""
    lang = canonical_language(lang)
    lines = text.splitlines()
    for i, line in enumerate(lines):
        m = ACTIVE.match(line)
        if m:
            lines[i] = f'{m.group(1)}lang{m.group(2)}{lang}'
            return '\n'.join(lines) + '\n'
    for i, line in enumerate(lines):
        if COMMENTED.match(line):
            lines[i] = f'lang = {lang}'
            return '\n'.join(lines) + '\n'
    for i, line in enumerate(lines):
        m = SECTION.match(line)
        if m and m.group(1).strip().lower() == 'language':
            lines.insert(i + 1, f'lang = {lang}')
            return '\n'.join(lines) + '\n'
    if lines and lines[-1].strip():
        lines.append('')
    lines += ['[language]', f'lang = {lang}']
    return '\n'.join(lines) + '\n'


def get_language(text):
    for line in text.splitlines():
        m = ACTIVE.match(line)
        if m:
            return m.group(3)
    return None


def write(dest, lang, example=None):
    """Write dest/fe3h.ini with lang; an existing file keeps its other lines. Returns the path."""
    path = Path(dest) / 'fe3h.ini'
    if path.is_file():
        text = path.read_text(encoding='utf-8-sig')
    elif example and Path(example).is_file():
        text = Path(example).read_text(encoding='utf-8')
    else:
        text = ''
    new = set_language(text, lang)
    if path.is_file() and new == text:
        return path
    tmp = path.with_name('fe3h.ini.tmp')
    tmp.write_text(new, encoding='utf-8', newline='\n')
    os.replace(tmp, path)
    return path
