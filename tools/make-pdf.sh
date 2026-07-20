#!/usr/bin/env bash
# Build CMake-by-Example.pdf from the book/ chapters.
# Requires: Node.js, and Chrome or Edge. Run from anywhere:
#   bash tools/make-pdf.sh
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"

# 1. Ensure the markdown converter is installed.
cd "$HERE"
[ -d node_modules/marked ] || npm install

# 2. Markdown -> single styled HTML, into a space-free temp dir.
TMP="$(mktemp -d)"
node make-pdf.mjs "$TMP/book.html"

# 3. Find a Chromium-based browser.
CHROME=""
for c in \
  "/c/Program Files/Google/Chrome/Application/chrome.exe" \
  "/c/Program Files (x86)/Google/Chrome/Application/chrome.exe" \
  "/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe" \
  "/c/Program Files/Microsoft/Edge/Application/msedge.exe"; do
  [ -f "$c" ] && CHROME="$c" && break
done
[ -z "$CHROME" ] && { echo "ERROR: no Chrome or Edge found"; exit 1; }

# 4. HTML -> PDF via headless print (Windows paths for the browser).
TMPWIN="$(cygpath -m "$TMP")"
"$CHROME" --headless=new --disable-gpu --no-pdf-header-footer \
  --print-to-pdf="$TMPWIN/CMake-by-Example.pdf" \
  "file:///$TMPWIN/book.html" 2>/dev/null || true

# 5. Copy the result next to the book.
cp "$TMP/CMake-by-Example.pdf" "$REPO/CMake-by-Example.pdf"
rm -rf "$TMP"
echo "Wrote $REPO/CMake-by-Example.pdf"
