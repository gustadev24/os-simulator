#!/bin/bash
# Compilación completa - Con bibliografía y referencias
# Ejecuta el ciclo completo: pdflatex -> bibtex -> pdflatex -> pdflatex

set -e

cd "$(dirname "$0")"

MAIN_TEX="src/New_IEEEtran_how-to.tex"
MAIN_NAME=$(basename "${MAIN_TEX}" .tex)

echo "📚 Compilación completa de ${MAIN_TEX}..."

echo "  [1/4] Primera pasada de pdflatex..."
pdflatex -output-directory=. -interaction=nonstopmode "${MAIN_TEX}"

echo "  [2/4] Procesando bibliografía con bibtex..."
BIBINPUTS=src:$BIBINPUTS bibtex "${MAIN_NAME}"

echo "  [3/4] Segunda pasada de pdflatex..."
pdflatex -output-directory=. -interaction=nonstopmode "${MAIN_TEX}"

echo "  [4/4] Tercera pasada de pdflatex..."
pdflatex -output-directory=. -interaction=nonstopmode "${MAIN_TEX}"

# Mover archivos auxiliares a build/
mv *.aux *.log *.bbl *.blg *.out 2>/dev/null build/ || true

echo ""
echo "✅ Compilación completada: ${MAIN_NAME}.pdf"
echo ""
echo "📄 Archivos generados:"
ls -lh "${MAIN_NAME}.pdf" 2>/dev/null || echo "  (PDF no encontrado)"
