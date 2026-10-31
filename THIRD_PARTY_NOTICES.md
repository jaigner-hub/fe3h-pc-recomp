# Third-party notices

Code copied or adapted from another project is listed here with its source, the exact commit, its license, and the files it went into. Each copied file keeps its original copyright header.

| Component | Source and commit | License | Used in |
|---|---|---|---|
| Content guard, its pre-commit hook and CI workflow, reference fetch script | https://github.com/valentderah/totk-pc-recomp @ `c944f9f94520874f2a669add30dd08d29ee9f7b5`, Copyright (c) 2026 Valiantsin Dzerakh | MIT | `tools/check_content.py` (new marker name, all `subsdk` names), `.githooks/pre-commit` (falls back to python3), `.github/workflows/content-guard.yml`, `tools/fetch_refs.sh` (different reference list) |
| hactool build script and patches | https://github.com/valentderah/totk-pc-recomp @ `c944f9f94520874f2a669add30dd08d29ee9f7b5`, Copyright (c) 2026 Valiantsin Dzerakh | MIT | `tools/build_hactool.py` (adds a Linux/WSL build with the system gcc), `tools/hactool/portable.patch` and `tools/hactool/unicode.patch` (unchanged) |
| hactool patch content from MacNativeGames `build_app.py` `build_hactool()` (via totk-pc-recomp `portable.patch`): `MNG_TITLE_KEYS`, relaxed BKTR layout checks | https://github.com/ly05010419/MacNativeGames @ `b75be8cfa704df3701e86130c8ced268e79d6b11`, Copyright (c) 2026 MacNativeGames contributors | MIT | `tools/hactool/portable.patch` |
| hactool (fetched and built by `tools/build_hactool.py` into the ignored `toolchain/`; not committed) | https://github.com/SciresM/hactool @ `1d64a83450e025622f3468c28fc4164dad2c5ef6` | ISC | `toolchain/hactool/` |
| ExeFS extractor | https://github.com/ly05010419/MacNativeGames @ `b75be8cfa704df3701e86130c8ced268e79d6b11` (`games/totk/scripts/extract_exefs.py`), Copyright (c) 2026 MacNativeGames contributors, as vendored unchanged by totk-pc-recomp @ `c944f9f94520874f2a669add30dd08d29ee9f7b5` (`tools/vendor/macnativegames/`) | MIT | `tools/extract_exefs.py` (title id taken from the package instead of TotK's, PFS0 section reader factored out, CNMT title id and version read from the Meta NCA) |
