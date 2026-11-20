#!/bin/bash
# Cross-compiles NF Brutal (VST3, licence lock ON) for Windows x64 on the Mac and wraps it in a self-contained
# installer .exe (Installer/Windows/stub). No Windows, GitHub Actions or Inno Setup needed.
# Needs: ~/Documents/xwin-sdk (xwin SDK + clang-cl.cmake), LLVM (brew), ninja, the JUCE submodule (git submodule update --init).
# Result: ~/Desktop/NF Brutal <version> Setup.exe
set -e
cd "$(dirname "$0")/../.."
[ -f JUCE/CMakeLists.txt ] || { echo "JUCE submodule is empty: run  git submodule update --init" >&2; exit 1; }
TC="$HOME/Documents/xwin-sdk/clang-cl.cmake"
VER=$(grep -m1 -o 'project(NFBrutal VERSION [0-9.]*' CMakeLists.txt | awk '{print $3}')
cmake -S . -B build-win -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TC" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
  -DCMAKE_OSX_ARCHITECTURES= -DBRUTAL_LICENSE=ON
cmake --build build-win --target NFBrutal_VST3
DLL="$PWD/build-win/NFBrutal_artefacts/Release/VST3/NF Brutal.vst3/Contents/x86_64-win/NF Brutal.vst3"
[ -f "$DLL" ] || { echo "VST3 DLL not built: $DLL" >&2; exit 1; }
[ "$(strings -a "$DLL" | grep -c "/activate")" -gt 0 ] || { echo "Licence code missing from the build: refusing to make the installer" >&2; exit 1; }
rm -rf build-win-installer
cmake -S Installer/Windows/stub -B build-win-installer -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TC" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded "-DNFBR_PAYLOAD_DLL=$DLL" -DNFBR_VERSION="$VER" -DCMAKE_OSX_ARCHITECTURES=
cmake --build build-win-installer
OUT="$HOME/Desktop/NF Brutal $VER Setup.exe"
cp build-win-installer/NFBRInstaller.exe "$OUT"
echo "Done: $OUT"
