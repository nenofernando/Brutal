# NF Brutal

Vocal compressor / limiter plug-in by **NF Audio Tools** (Nenno Fernando). VST3 and AU, macOS (Intel and Apple Silicon).

Very fast, hard-limiting compressor with make-up gain and odd-harmonic saturation, built for loud, forward vocals.

Controls: **Slay** (amount / hardness), **Input**, **Gain**, **Body** (low-end cut), **Mix**, **Lofi** (band-pass) and **On**.

Status: early prototype (v0.1.0). No licence lock, no installer, no Windows build yet.

## Build (macOS)

```sh
git clone --recurse-submodules https://github.com/nenofernando/Brutal.git
cd Brutal
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
cmake --build build
```

Bundles are written to `build/NFPorrada_artefacts/Release/` (the internal CMake targets still carry the working name).

* `Source/PorradaDSP.h` – DSP core (header only).
* `Source/PorradaEditor.h` – user interface.
* `Tools/porrada_render.cpp` – offline renderer used for tests.
* `build/NFPorradaShot_artefacts/Release/NFPorradaShot out.png [width] [grDb] [slay]` renders the UI to a PNG without a host.
