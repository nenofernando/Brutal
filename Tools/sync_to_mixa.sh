#!/bin/bash
# Copies the shared files (DSP, core, editor) into Mixa Studio's Source/NFBrutal/, so the native insert is the SAME code as the plug-in.
# Usage: tools/sync_to_mixa.sh [path to the Mixa Studio repo]
set -e
MIXA="${1:-$HOME/Documents/NF-Mixa-1.0.20-installer}"
SRC="$(cd "$(dirname "$0")/.." && pwd)/Source/Shared"
mkdir -p "$MIXA/Source/NFBrutal"
cp "$SRC"/*.h "$MIXA/Source/NFBrutal/"
echo "Synced $(ls "$SRC"/*.h | wc -l | tr -d ' ') files to $MIXA/Source/NFBrutal"
