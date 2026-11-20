# NF Brutal

Vocal compressor / limiter plug-in by **NF Audio Tools** (Nenno Fernando). VST3 and AU, macOS (Intel and Apple Silicon).

Very fast, hard-limiting compressor with make-up gain and odd-harmonic saturation, built for loud, forward vocals.

Controls: **Slay** (amount / hardness), **Input**, **Gain**, **Body** (low-end cut), **Mix**, **Lofi** (band-pass) and **On**.

Status: early release (v0.1.0). The plug-in needs an access key (built with `-DBRUTAL_LICENSE=OFF` for development). Built into Mixa Studio as a native insert (no key there). Installers: `Installer/macos/build_dmg.sh` (Mac) and `Installer/Windows/build_windows_on_mac.sh` (Windows, cross-compiled).

## Build (macOS)

```sh
git clone --recurse-submodules https://github.com/nenofernando/Brutal.git
cd Brutal
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
cmake --build build
```

Bundles are written to `build/NFBrutal_artefacts/Release/` (NF Brutal.vst3 and NF Brutal.component).

* `Source/Shared/BrutalDSP.h` – DSP core (header only).
* `Source/Shared/BrutalEditor.h` – user interface.
* `Tools/brutal_render.cpp` – offline renderer used for tests.
* `build/NFBrutalShot_artefacts/Release/NFBrutalShot out.png [width] [grDb] [slay]` renders the UI to a PNG without a host.
