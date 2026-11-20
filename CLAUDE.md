# NF Brutal -- notes for Claude

Vocal compressor / limiter by NF Audio Tools (Nenno Fernando). The plug-in (VST3 + AU, Mac; VST3 Windows) is sold with an access key;
the same code is built into Mixa Studio as a native insert (no key there, the Mixa licence covers it).

## Layout
- `Source/Shared/` -- shared with Mixa (identical files): `BrutalDSP.h` (DSP), `BrutalCore.h` (parameters, state, processing), `BrutalEditor.h` (UI).
  **Never put licence code in Shared.** After changing it run `tools/sync_to_mixa.sh` (copies into `~/Documents/NF-Mixa-1.0.20-installer/Source/NFBrutal/`),
  then build and run the Mixa self-test (`Mixa Studio --selftest`, must say SELFTEST OK) and commit there (the Mixa wrapper is `Source/NativeBrutal.h`).
- `Source/PluginProcessor.*` -- plug-in wrapper: licence flag + watch thread (weekly online check, 30 days offline grace), mutes when unlicensed,
  puts `Source/License/BrutalLicenseOverlay.h` over the editor.
- `Source/License/` -- `NFBRLicense.h` (offline RSA check + POST /activate on the licence worker, 2 computers per key), `NFBRLicensePublicKey.h` (public half).
- `Tools/brutal_render.cpp` -- offline renderer for A/B tests against the reference behaviour.

## Licence
Product `nfbr` on the worker `https://mixa-licence.nenovoice.workers.dev` (`/admin`, product "NF Brutal"); short key `NFBR-XXXX-XXXX-XXXX-XXXX` traded at POST /activate
for the signed long key `NFBR2|name|expires|id`. Private key `~/.nfbr_license/private.pem` (+ `pkcs8.b64`, to be uploaded as worker secret `NFBR_KEY`) -- never commit it;
the owner must back it up. The key file lives in `~/Library/NF Audio Tools/NF Brutal/licence.key` (JUCE `userApplicationDataDirectory`; on Windows `%APPDATA%`).
CMake `-DBRUTAL_LICENSE=OFF` = development build with no lock. The worker code with the product is in `~/Documents/mixa-worker-wt/site/license-worker` (deploy with `deploy.sh --yes`, needs the owner's OK).

## Installers (built locally, never uploaded by Claude, never committed)
- Mac: `bash Installer/macos/build_dmg.sh` -> `~/Desktop/NF Brutal <version>.dmg` (universal, VST3 + AU, licence ON, ad-hoc signed, .pkg unsigned; refuses a build without the licence code). No AAX yet (needs a PACE product).
- Windows: `bash Installer/Windows/build_windows_on_mac.sh` -> `~/Desktop/NF Brutal <version> Setup.exe` (cross-compiled with clang-cl + xwin, tiny custom installer in `Installer/Windows/stub`). Not yet run on a real Windows PC.
- The version is `project(NFBrutal VERSION ...)` in `CMakeLists.txt` line 2; the installers read it from there.

## Rules that apply to every NF plug-in
Default window 810 x 270, preset tab 157 x 21, official MAIN logo as an image, UI layers kept separate. Mac builds on the owner's Intel Mac: use `-DCMAKE_OSX_ARCHITECTURES=x86_64` for quick local builds (the DMG script builds universal).
