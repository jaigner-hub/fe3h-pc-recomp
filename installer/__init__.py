# From totk-pc-recomp installer/__init__.py @ c944f9f94520874f2a669add30dd08d29ee9f7b5 (MIT, Copyright (c) 2026 Valiantsin Dzerakh), renamed to fe3h; FE3H changes are noted where made.
"""The FE3H installer: builds a playable folder from the player's own game files (docs/installer.md).

    python -m installer --base FILE --update FILE --keys FILE --lang CODE --dest DIR [--root DIR] [--temp DIR]

Modules: layout (where the bundled tools are, release or repository), events (JSON progress on stdout and the log
file), state (fe3h-install.json), runner (which steps run: resume and update), steps (the six steps), ini (fe3h.ini),
cli (the command line). text/en.json holds the English text of every message id the events carry.

Standard library only, apart from what the extraction tools import (cryptography). Runs on Python 3.14.
"""
