<!-- From totk-pc-recomp docs/installer.md @ c944f9f94520874f2a669add30dd08d29ee9f7b5 (MIT, Copyright (c) 2026 Valiantsin Dzerakh), adapted to FE3H. -->
# Installer

A public release that turns the player's own game files into a playable FE3H folder. The release carries no game
data and no code derived from the game: the installer builds the game on the player's machine. The installed folder
holds only the game; the installer and its tools stay outside it.

The installer is totk-pc-recomp's (`installer/`, MIT), renamed and changed where this game differs: one supported
update, our extraction tools, no RomFS patches and no shader seed.

## What the player does

1. Downloads `FE3H-Setup-<version>-windows.zip` from GitHub Releases, unpacks it anywhere and starts `FE3H-Setup.exe`.
2. Picks:
   - the base game (XCI or NSP) and the 1.1.1 update (NSP);
   - their `prod.keys`;
   - the game language and the install folder.
3. Waits for the setup, one time. The window shows each step and its progress.
4. Plays `fe3h.exe` from the install folder. The installer and the source files can be deleted.

The install folder (`main.c` `portable_paths()` reads this layout; it stays portable):

```
<install folder>/
  fe3h.exe, SDL3.dll, libopus-0.dll, libiconv-2.dll, fe3h_xlate.exe
  fe3h.ini                       language; the other keys as commented defaults (fe3h.ini.example)
  exefs/, romfs/                 extracted from the player's files
  profile/                       saves
  logs/                          fe3h.log of the last run, fe3h.1.log to fe3h.4.log of the runs before it
  shader cache                   the translations made in play, and the answers met (shader-answers.bin)
  fe3h-install.json              what built this folder: release version, input hashes, finished steps
```

## Updating the port

The player runs a newer installer on the same folder. It reads `fe3h-install.json`, keeps `romfs/`, `exefs/`,
`profile/` and `fe3h.ini` after checking their hashes, rebuilds `fe3h.exe` and reinstalls the translator. A newer
release that changes nothing game-derived skips the rebuild.

## Setup steps

1. **Check** before writing anything: the files open with the keys, the base is 1.0.0 and the update 1.1.1 (by the
   hashes of their code modules, `tools/data/expected-v*.json`), the keys are new enough for the update, enough space
   is free on the target disk (about 25 GiB at the peak of a first install, 13 GiB after it), the CPU has AVX2
   (`-march=x86-64-v3`), a Vulkan 1.3 GPU exists. Each failure is one clear message.
2. **Extract** the ExeFS of the update and the RomFS (the base image with the update's BKTR patch, merged by hactool,
   which verifies every IVFC level) into the install folder and verify the hashes.
3. **Lift** the ExeFS to C (`recomp/gen.py`, `recomp/patches-v262144.txt`) into a temp folder next to the installer.
4. **Compile** the generated C, `modules.c`, `romfs.c` and `patches.c` (they bake the per-install layout, RomFS hashes
   and the game version's patch page) and link them with the prebuilt runtime library into `fe3h.exe`
   (`build_game.py --runtime-lib`).
5. **Prepare**: install the shader translator (`fe3h_xlate.exe`) next to the game. Shaders are translated and cached
   as they are met in play; the next start of the game rebuilds what was met into the driver's cache.
6. **Clean up** the temp folder (generated C and objects).

A cancelled or failed setup resumes from the last finished step (`fe3h-install.json`).

## Command line

`installer/` (stdlib only; `installer/layout.py` maps the release folder and, in a repository, the dev toolchain):

```
python -m installer --base FILE --update FILE --keys FILE --lang CODE --dest DIR [--root DIR] [--temp DIR] [--verify]
```

- On a folder with `fe3h-install.json` only `--dest` is needed; the game files only when the extraction must run again.
- stdout: one JSON event per line, types `start`, `plan`, `step` (started, done, skipped), `progress` (step, phase,
  done/total/unit), `space` (bytes needed and free on the install disk and the temp disk), `warning`, `error`, `end`
  (`installer/events.py`). Messages are ids with parameters; `installer/text/en.json` holds their English text. The
  human-readable log goes to `<temp>/logs/`.
- Exit codes: 0 done, 1 failed, 2 bad command line, 3 cancelled. The window cancels with Ctrl+Break to the
  installer's process group.
- Each step records a key over its inputs (runner.py). A step with the recorded key and intact output is skipped, so
  resume and update are one rule; output kept from another release is re-hashed first.
- Peak space on the install disk: the extraction copies both Program NCAs (about 12.5 GiB) next to the merged RomFS
  (11.3 GiB) before it deletes them. The check computes the exact figure.
- Window (`python -m installer.gui`, `installer/gui.py` draws, `installer/guilogic.py` decides): form, step list with
  progress, space line, collapsible log, Cancel, finish screen with Open folder. English or Russian, chosen from the
  Windows display language, switchable in the window; every sentence is an id in `installer/text/<en|ru>.json`. A
  folder with `fe3h-install.json` needs only the folder. `--selftest [--shots DIR]` builds every screen in both
  languages and closes itself.
- `tests/installer_test.py`: bookkeeping with fake steps.
- Dev mode (a repository checkout, no `python/python.exe`): the Python that runs the module runs the tools, so use
  one with numpy, lz4 and cryptography (`.venv-win`); `toolchain/llvm-mingw-*` and `toolchain/deps-mingw` when they
  exist, else MSYS2 UCRT64's clang, ninja, headers, libraries and DLLs (the usual build's); the runtime library from
  `build/install-rtlib` (`tools/build_game.py --make-runtime-lib --out build/install-rtlib`); `toolchain/hactool/hactool.exe`
  (`tools/build_hactool.py` on Windows); `build/xlate/fe3h_xlate.exe`; the temp folder `build/install-temp`.

## Release layout

The unpacked release; `tools/make_release.py` assembles it and `--zip` packs it:

```
FE3H-Setup.exe        starts python/pythonw.exe -m installer.gui (installer/launcher/launcher.c, no console)
installer/            the installer: steps, CLI, window, en/ru text catalog
python/               Python 3.14 embeddable + numpy, lz4, cryptography, cffi, pycparser (+ tkinter for the window)
recomp/, tools/       the lifter, the build and extraction scripts the steps run (copied from the repo), tools/data/
llvm/                 llvm-mingw, x86_64-only subset (Apache-2.0 with LLVM exception; mingw-w64 parts permissive)
runtime/              libfe3hrt.a and the headers the generated code and modules.c/romfs.c/patches.c include
bin/                  fe3h_xlate.exe, hactool.exe, SDL3.dll, libopus-0.dll, ninja.exe
THIRD_PARTY_NOTICES.md
fe3h.ini.example, VERSION  the default ini; the release string (the repository's VERSION plus the commit)
```

- SDL3 comes from the upstream release, not MSYS2: MSYS2's DLL imports the LGPL libiconv.
- Every bundled third-party piece gets a row in `THIRD_PARTY_NOTICES.md`.
- The release contains nothing game-derived; `make_release.py` runs `tools/check_content.py` over the folder before it
  finishes.

## Constraints

- The installer steps contain no Win32 calls: Linux follows with its own compiler and the same steps.
- The keys file path is passed to the tools; key values never appear in the UI, logs or files.
- The window is a front end over the CLI, which prints one JSON progress event per line. Tests and CI use the CLI.
- Paths may hold any Unicode characters. Every exe the setup runs or builds reads its arguments and paths as UTF-8
  through an `activeCodePage` manifest: hactool (our patch), fe3h.exe (`runtime/fe3h.rc`), ninja (upstream).
- A game file whose data fails its hash checks or ends early stops the setup with `check.input_damaged`, which names
  the file and asks to copy it again (`tools/extract.py` `DamagedFile`).
- Unsigned exe: SmartScreen warns on first start.

## Later

- An icon for `fe3h.exe` and the setup (`runtime/fe3h.ico`; the launcher builds without one for now).
- A GitHub release workflow: a release is `make_release.py --zip` on a machine with uv and Python 3.14, a tag at the
  commit the zip names in its `VERSION`, and `gh release create` with the zip as `FE3H-Setup-<version>-windows.zip`.
  NSZ and XCZ input. Linux release.
