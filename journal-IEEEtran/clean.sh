#!/bin/bash
# Limpia todos los archivos auxiliares de LaTeX

cd "$(dirname "$0")"

echo "🧹 Limpiando archivos auxiliares..."

# Mover archivos auxiliares a build/
mv *.aux *.log *.bbl *.blg *.out *.toc *.lof *.lot *.fls *.fdb_latexmk *.synctex.gz 2>/dev/null build/ || true

# Limpiar build/ si está vacío o quieres empezar de cero
# rm -rf build/*

echo "✅ Limpieza completada"
echo "📁 Archivos en build/:"
ls -lh build/ 2>/dev/null | grep -v "^total" || echo "  (vacío)"
