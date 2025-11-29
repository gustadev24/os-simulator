# Simulador Integrado de Planificación de Procesos y Gestión de Memoria Virtual

## Descripción

El simulador desarrollado integra módulos de planificación de procesos y gestión de memoria virtual, manteniendo control en la interacción de ambos mediante el uso de políticas establecidas con el objetivo de lograr un rendimiento óptimo del sistema.

## Desarrollo

### Requisitos

- Compilador C++ (GCC, Clang, etc.)
- [Python 3.12+](https://www.python.org/downloads/) (para visualización)
- [CMake](https://cmake.org/download/) (para automatización de compilación)
- [Just](https://github.com/casey/just) (opcional, para automatización de comandos)
- [Ninja](https://ninja-build.org/) (opcional, para compilación rápida)

### Documentación

_Doxygen recientemente tuvo un problema con cambios hechos en el kernel de LaTeX, por lo que cualquier versión <1.15 va a fallar con versiones de LaTeX del año 2025_

- [Doxygen 1.15+](https://www.doxygen.nl/index.html) (para documentación)
- [LaTeX](https://www.latex-project.org/get/) (para documentación en PDF)

### Servidor de lenguaje

Se recomienda usar el servidor de lenguaje clangd que ya se encuentra configurado para este proyecto.

### Comandos útiles

#### Compilación

**La primera build puede tardar un poco más ya que se descargan y construyen las dependencias.**

Usando CMake:

```bash
cmake -S . -B build
cmake --build build
```

Usando Just:

```bash
just build
```

#### Ejecutar el programa

El simulador se puede ejecutar de varias formas:

**Modo por defecto** (carga archivos desde `data/procesos/` con métricas habilitadas):

```bash
./build/bin/os_simulator
```

**Modo demostración** (ejecuta todos los algoritmos con datos predefinidos):

```bash
./build/bin/os_simulator --demo
```

**Con archivos personalizados**:

```bash
./build/bin/os_simulator -f mi_archivo.txt -c mi_config.txt
```

**Con archivo de métricas personalizado**:

```bash
./build/bin/os_simulator -m mis_metricas.jsonl
```

**Opciones disponibles**:

- `-f <archivo>`: Archivo de procesos (default: `data/procesos/procesos.txt`)
- `-c <archivo>`: Archivo de configuración (default: `data/procesos/config.txt`)
- `-m [archivo]`: Habilitar métricas (default: `data/resultados/metrics.jsonl`)
- `-d, --demo`: Ejecutar demostración con algoritmos predefinidos
- `-h, --help`: Mostrar ayuda

Usando Just:

```bash
just run
```

#### Ejecutar tests

Usando CMake:

```bash
cmake -S . -B build
cmake --build build --target tests
./build/bin/tests
```

Usando Just:

```bash
just test
```

#### Generar documentación

Usando Doxygen:

```bash
doxygen
```

Usando Just:

```bash
just build-docs
```

#### Servir documentación localmente

Usando python http.server:

```bash
cd docs/html && python3 -m http.server 8000
```

Usando Just:

```bash
just build-docs-serve
```

#### Convertir documentación a PDF

Usando latex:

```bash
cd docs/latex && make
```

Usando Just:

```bash
just build-docs-pdf
```
