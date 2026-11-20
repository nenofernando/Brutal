#!/bin/bash
# Publica os instaladores do NF Brutal para os clientes: os links do e-mail de licença (/download/nfbr-mac | nfbr-windows?k=...) baixam estes arquivos.
#   bash tools/publish_installers.sh          -> ensaio: só mostra o que faria
#   bash tools/publish_installers.sh --yes    -> publica de verdade
# Pré-requisitos (versão = CMakeLists.txt, linha 2): em ~/Desktop/NF Brutal Instaladores
#   "NF Brutal <versão>.dmg"        (bash Installer/macos/build_dmg.sh)
#   "NF Brutal <versão> Setup.exe"  (bash Installer/Windows/build_windows_on_mac.sh)
# O que faz: 1) copia de segurança do que está no ar; 2) envia NF-Brutal.dmg e NF-Brutal-Setup.exe ao bucket mixa-downloads; 3) confere os tamanhos.
# Plug-ins não têm caixa de atualização: quem já comprou baixa de novo pelo link do e-mail (ou o dono reenvia o e-mail em /admin).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER="$(grep -m1 -oE 'project\(NFBrutal VERSION [0-9]+\.[0-9]+\.[0-9]+' "$ROOT/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"
DIR="$HOME/Desktop"
DMG="$DIR/NF Brutal $VER.dmg"; EXE="$DIR/NF Brutal $VER Setup.exe"
BUCKET="mixa-downloads"
fail() { echo "NÃO PUBLICADO: $1" >&2; exit 1; }
[ -n "$VER" ] || fail "não consegui ler a versão do CMakeLists.txt"
[ -f "$DMG" ] || fail "falta $DMG"; [ -f "$EXE" ] || fail "falta $EXE (bash Installer/Windows/build_windows_on_mac.sh)"
export PATH="$HOME/.local/node-dl/node-v24.22.0-darwin-x64/bin:$PATH"; WR="$HOME/.local/wr/node_modules/.bin/wrangler"
echo "Versão $VER"; ls -la "$DMG" "$EXE" | awk '{print "  " $5 "  " $9 " " $10 " " $11 " " $12 " " $13}'
[ "${1:-}" = "--yes" ] || { echo "Ensaio apenas. Acrescente --yes para publicar."; exit 0; }
BK="$HOME/Documents/NFBrutalBackups/r2-before-$VER"; mkdir -p "$BK"
for o in NF-Brutal.dmg NF-Brutal-Setup.exe; do
  "$WR" r2 object get "$BUCKET/$o" --file "$BK/$o" --remote >/dev/null 2>&1 && echo "  cópia de segurança: $o" || echo "  (não havia $o no ar)"
done
"$WR" r2 object put "$BUCKET/NF-Brutal.dmg" --file "$DMG" --content-type application/x-apple-diskimage --remote
"$WR" r2 object put "$BUCKET/NF-Brutal-Setup.exe" --file "$EXE" --content-type application/octet-stream --remote
echo "Publicado: NF Brutal $VER (Mac e Windows) no servidor de licenças."
