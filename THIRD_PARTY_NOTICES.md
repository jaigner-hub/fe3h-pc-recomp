# Third-party notices

Code copied or adapted from another project is listed here with its source, the exact commit, its license, and the files it went into. Each copied file keeps its original copyright header.

| Component | Source and commit | License | Used in |
|---|---|---|---|
| Content guard, its pre-commit hook and CI workflow, reference fetch script | https://github.com/valentderah/totk-pc-recomp @ `c944f9f94520874f2a669add30dd08d29ee9f7b5`, Copyright (c) 2026 Valiantsin Dzerakh | MIT | `tools/check_content.py` (new marker name, all `subsdk` names), `.githooks/pre-commit` (falls back to python3), `.github/workflows/content-guard.yml`, `tools/fetch_refs.sh` (different reference list) |
