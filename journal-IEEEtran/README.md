# Artículo IEEE - OS Simulator

Template de artículo IEEE en LaTeX para documentar el proyecto OS Simulator.

## Estructura del Proyecto

```
journal-IEEEtran/
├── src/
│   ├── New_IEEEtran_how-to.tex    # Documento principal
│   └── referencias.bib             # Bibliografía BibTeX
├── figures/
│   └── fig1.png                    # Imágenes del artículo
├── build/
│   └── ...                         # Archivos auxiliares (.aux, .log, etc.)
├── docs-original/
│   └── ...                         # Templates y documentación original
├── compile-quick.sh                # Script de compilación rápida
├── compile-full.sh                 # Script de compilación completa
├── .gitignore                      # Ignorar archivos temporales
├── README.md                       # Este archivo
└── New_IEEEtran_how-to.pdf        # PDF generado
```

## Compilación

### Compilación Rápida
Para ver cambios rápidos sin procesar referencias:

```bash
./compile-quick.sh
```

### Compilación Completa
Para compilación final con bibliografía y referencias:

```bash
./compile-full.sh
```

El PDF generado se creará en el directorio raíz, y los archivos auxiliares se moverán automáticamente a `build/`.

## Modificar el Documento

1. **Título y Autor**: Editar en `src/New_IEEEtran_how-to.tex` (líneas 30-32)
2. **Abstract**: Editar en el mismo archivo (líneas 40-45)
3. **Keywords**: Editar línea 48
4. **Contenido**: Modificar secciones desde línea 52
5. **Referencias**: Agregar en `src/referencias.bib`
6. **Imágenes**: Colocar en `figures/` y referenciar sin ruta (ej: `\includegraphics{fig1.png}`)

## Requisitos

- `pdflatex`
- `bibtex`
- Clase `IEEEtran` (incluida en TeX Live)

## Notas

- Los archivos auxiliares se mueven automáticamente a `build/` tras compilación completa
- El PDF compilado permanece en el directorio raíz para fácil acceso
- Usa compilación rápida durante edición
- Usa compilación completa antes de entregar
- Los templates originales están en `docs-original/` para referencia
