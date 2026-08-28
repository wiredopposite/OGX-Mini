# Pushing submodule forks (bluepad32 & tinyusb)

`.gitmodules` points **bluepad32** and **tinyusb** at **MegaCadeDev** forks so OGX-Mini patches are fetchable. If those repos do not exist yet:

1. On GitHub, **fork** [ricardoquesada/bluepad32](https://github.com/ricardoquesada/bluepad32) → `MegaCadeDev/bluepad32`.
2. **Fork** [hathach/tinyusb](https://github.com/hathach/tinyusb) → `MegaCadeDev/tinyusb`.

Then push the patched branches **before** (or right after) pushing **OGX-Mini-2026**:

```bash
# From repo root — bluepad32 (branch ogxm-mini-2026)
cd Firmware/external/bluepad32
git remote add megacade https://github.com/MegaCadeDev/bluepad32.git 2>/dev/null || true
git push megacade ogxm-mini-2026:main

# tinyusb
cd ../tinyusb
git remote add megacade https://github.com/MegaCadeDev/tinyusb.git 2>/dev/null || true
git push megacade ogxm-mini-2026:main
```

If `main` on the fork rejects (non-fast-forward), use a named branch instead, e.g. `git push megacade ogxm-mini-2026:ogxm-mini-2026`, then set the fork’s default branch to that or merge via PR.

After forks are updated, clone with:

```bash
git clone --recursive https://github.com/MegaCadeDev/OGX-Mini-2026.git
```
