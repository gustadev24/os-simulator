#!/bin/bash
# Compilación rápida - Solo pdflatex una vez
# Útil para ver cambios rápidos sin referencias completas

set -e

cd "$(dirname "$0")"

MAIN_TEX="src/New_IEEEtran_how-to.tex"
MAIN_NAME=$(basename "${MAIN_TEX}" .tex)

echo "🚀 Compilación rápida de ${MAIN_TEX}..."
pdflatex -output-directory=. -interaction=nonstopmode "${MAIN_TEX}"

echo "✅ Compilación completada: ${MAIN_NAME}.pdf"
