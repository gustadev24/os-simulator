# Simulador Integrado de Planificación de Procesos y Gestión de Memoria Virtual

## 1. Descripción

El simulador desarrollado busca simular el comportamiento de un sistema operativo en cuanto a la planificación de procesos, de entrada/salida y gestión de memoria virtual. Además, permite generar métricas y visualizaciones que ayudan a comprender el comportamiento del sistema.

### Características principales

- **Planificación de CPU**: Algoritmos FCFS, SJF, Round Robin y Priority
- **Gestión de memoria virtual**: Paginación con algoritmos de reemplazo FIFO, LRU, NRU y Óptimo
- **Gestión de E/S**: Simulación de dispositivos de entrada/salida con planificación FCFS y Round Robin
- **Recolección de métricas**: Generación de archivos JSONL con datos de ejecución
- **Visualización**: Generación de diagramas y gráficos (modo individual y por lotes)
- **Sincronización de hilos**: Ejecución concurrente de procesos con mecanismos de sincronización (ver [documentación detallada](docs/THREAD_SYNCHRONIZATION.md))

---

## 2. Instalación

### 2.1. Pre-requisitos

#### Compilador C++ (C++17 o superior)

| Sistema Operativo | Instalación                                                                                                                                |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| Windows           | Instalar [Visual Studio](https://visualstudio.microsoft.com/) con "Desktop development with C++" o [MinGW-w64](https://www.mingw-w64.org/) |
| Linux             | `sudo apt install build-essential` (Debian/Ubuntu) o `sudo dnf install gcc-c++` (Fedora)                                                   |
| macOS             | `xcode-select --install`                                                                                                                   |

#### CMake (3.16 o superior)

| Sistema Operativo | Instalación                                                                               |
| ----------------- | ----------------------------------------------------------------------------------------- |
| Windows           | Descargar desde [cmake.org](https://cmake.org/download/) o `winget install Kitware.CMake` |
| Linux             | `sudo apt install cmake` (Debian/Ubuntu) o `sudo dnf install cmake` (Fedora)              |
| macOS             | `brew install cmake`                                                                      |

#### Python (3.10 o superior)

| Sistema Operativo | Instalación                                                                                                                              |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| Windows           | Descargar desde [python.org](https://www.python.org/downloads/) o `winget install Python.Python.3.12`                                    |
| Linux             | `sudo apt install python3 python3-pip python3-venv` (Debian/Ubuntu) o `sudo dnf install python3 python3-pip python3-virtualenv` (Fedora) |
| macOS             | `brew install python`                                                                                                                    |

#### Ninja (opcional, compilación más rápida)

| Sistema Operativo | Instalación                                                                              |
| ----------------- | ---------------------------------------------------------------------------------------- |
| Windows           | `winget install Ninja-build.Ninja`                                                       |
| Linux             | `sudo apt install ninja-build` (Debian/Ubuntu) o `sudo dnf install ninja-build` (Fedora) |
| macOS             | `brew install ninja`                                                                     |

#### Just (opcional, automatización de comandos)

| Sistema Operativo | Instalación                                                                          |
| ----------------- | ------------------------------------------------------------------------------------ |
| Windows           | `winget install Casey.Just`                                                          |
| Linux             | `cargo install just` o ver [instalación](https://github.com/casey/just#installation) |
| macOS             | `brew install just`                                                                  |

#### Doxygen (para documentación)

> **Nota**: Doxygen versión 1.15+ es requerido debido a cambios en el kernel de LaTeX en 2025.

| Sistema Operativo | Instalación                                                        |
| ----------------- | ------------------------------------------------------------------ |
| Windows           | Descargar desde [doxygen.nl](https://www.doxygen.nl/download.html) |
| Linux             | `sudo apt install doxygen` (Debian/Ubuntu)                         |
| macOS             | `brew install doxygen`                                             |

#### PlantUML y clang-uml (para diagramas UML)

| Sistema Operativo | Instalación                                                                                     |
| ----------------- | ----------------------------------------------------------------------------------------------- |
| Windows           | Descargar desde [plantuml.com](https://plantuml.com/download), clang-uml desde GitHub releases |
| Linux             | `sudo apt install plantuml` (Debian/Ubuntu), clang-uml compilar desde fuente                   |
| macOS             | `brew install plantuml`, clang-uml compilar desde fuente                                        |

> **Nota**: clang-uml se usa para generar automáticamente el diagrama UML del simulador C++ desde el código fuente. PlantUML renderiza los archivos `.puml` a imágenes.

#### LaTeX (para documentación en PDF)

| Sistema Operativo | Instalación                                                                           |
| ----------------- | ------------------------------------------------------------------------------------- |
| Windows           | Instalar [MiKTeX](https://miktex.org/download) o [TeX Live](https://tug.org/texlive/) |
| Linux             | `sudo apt install texlive-full` (Debian/Ubuntu)                                       |
| macOS             | `brew install --cask mactex`                                                          |

---

### 2.2. Descarga de archivos

Clone el repositorio usando Git:

```bash
git clone https://github.com/gustadev24/os-simulator.git
cd os-simulator
```

---

### 2.3. Proceso de instalación (Build de C++)

> **Nota**: La primera compilación puede tardar más tiempo ya que se descargan y construyen las dependencias (Catch2, nlohmann/json).

#### Usando CMake directamente

Windows (PowerShell):

```powershell
cmake -S . -B build
cmake --build build
```

Linux/macOS:

```bash
cmake -S . -B build
cmake --build build
```

#### Usando Just (recomendado)

```bash
just build
```

El ejecutable se generará en `build/bin/os_simulator`.

---

### 2.4. Ejecución del simulador y del visualizador

#### Ejecutar el simulador

Windows:

```powershell
.\build\bin\os_simulator.exe
```

Linux/macOS:

```bash
./build/bin/os_simulator
```

#### Configurar el entorno Python para el visualizador

Windows (PowerShell):

```powershell
python -m venv venv
.\venv\Scripts\Activate.ps1
pip install -r visualization\requirements.txt
```

Linux/macOS:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r visualization/requirements.txt
```

#### Ejecutar el visualizador

Windows:

```powershell
python -m visualization
```

Linux/macOS:

```bash
python3 -m visualization
```

---

## 3. Uso

### 3.1. Simulador

```
NOMBRE
    os_simulator - Simulador de planificación de procesos y memoria virtual

SINOPSIS
    os_simulator [OPCIONES]

DESCRIPCIÓN
    Simula la ejecución de procesos con diferentes algoritmos de planificación
    de CPU y gestión de memoria virtual. Genera métricas para análisis posterior.

OPCIONES
    -f <archivo>
        Especifica el archivo de procesos a cargar.
        Por defecto: data/procesos/procesos.txt

    -c <archivo>
        Especifica el archivo de configuración del simulador.
        Por defecto: data/procesos/config.txt

    -m <archivo>
        Especifica el archivo donde se guardarán las métricas.
        Por defecto: data/resultados/metrics.jsonl

    -h, --help
        Muestra esta ayuda.

EJEMPLOS
    # Ejecutar con configuración por defecto y métricas habilitadas
    ./build/bin/os_simulator

    # Usar archivos personalizados
    ./build/bin/os_simulator -f mis_procesos.txt -c mi_config.txt

    # Especificar archivo de métricas personalizado
    ./build/bin/os_simulator -m resultados/test.jsonl

ARCHIVOS DE ENTRADA
    Archivo de procesos (formato):
        PID tiempo_llegada CPU(x),E/S(y),CPU(z) prioridad paginas

        Ejemplo:
        P1 0 CPU(4),E/S(3),CPU(5) 1 4
        P2 2 CPU(6) 2 5

    Archivo de configuración (formato):
        total_memory_frames=64
        frame_size=4096
        scheduling_algorithm=RoundRobin
        page_replacement_algorithm=LRU
        io_scheduling_algorithm=FCFS
        quantum=4
        io_quantum=4

    Algoritmos de planificación disponibles:
        - FCFS
        - SJF
        - RoundRobin
        - Priority

    Algoritmos de reemplazo de páginas:
        - FIFO
        - LRU
        - Optimal
        - NRU

    Algoritmos de planificación de E/S:
        - FCFS
        - RoundRobin
```

---

### 3.2. Visualizador

```
NOMBRE
    visualization - Generador de diagramas para métricas del simulador

SINOPSIS
    python -m visualization [archivo_metricas] [directorio_salida]
    python -m visualization --batch <directorio_entrada> [directorio_salida]

DESCRIPCIÓN
    Lee archivos de métricas en formato JSONL y genera diagramas de visualización.

MODOS DE OPERACIÓN
    Modo individual (por defecto):
        Procesa un único archivo de métricas.

    Modo por lotes (--batch):
        Procesa todos los archivos JSONL en un directorio y sus subdirectorios.

ARGUMENTOS
    archivo_metricas
        Ruta al archivo de métricas JSONL.
        Por defecto: data/resultados/metrics.jsonl

    directorio_salida
        Directorio donde se guardarán los diagramas generados.
        Por defecto: data/diagramas

OPCIONES
    --batch <directorio_entrada>
        Activa el modo por lotes. Procesa todos los archivos .jsonl
        encontrados en el directorio de entrada.

    -h, --help
        Muestra la ayuda.

EJEMPLOS
    # Modo individual con configuración por defecto
    python -m visualization

    # Modo individual con archivo específico
    python -m visualization resultados/custom.jsonl

    # Modo individual con archivo y directorio de salida
    python -m visualization data/resultados/metrics.jsonl output/graficos

    # Modo por lotes - procesa todos los JSONL en un directorio
    python -m visualization --batch data/resultados/combinations/

    # Modo por lotes con directorio de salida personalizado
    python -m visualization --batch data/resultados/ output/diagramas_batch/
```

---

### 3.3. Generación de documentación con Doxygen

#### Generar documentación HTML

Usando Doxygen:

```bash
doxygen
```

O usando Just:

```bash
just build-docs
```

La documentación se genera en `docs/`.

#### Servir documentación localmente

Windows (PowerShell):

```powershell
cd docs/html
python -m http.server 8000
```

Linux/macOS:

```bash
cd docs/html && python3 -m http.server 8000
```

O usando Just:

```bash
just docs-serve
```

Acceder a `http://localhost:8000` en el navegador.

#### Generar documentación en PDF

Windows (requiere MiKTeX o TeX Live):

```powershell
cd docs/latex
pdflatex refman.tex
makeindex refman.idx
pdflatex refman.tex
```

Linux/macOS:

```bash
cd docs/latex && make
```

O usando Just:

```bash
just docs-pdf
```

El PDF se genera en `docs/latex/refman.pdf`.

---

### 3.4. Ejecución de pruebas unitarias con Catch2

#### Ejecutar pruebas

Windows:

```powershell
.\build\bin\tests.exe
```

Linux/macOS:

```bash
./build/bin/tests
```

---

### 3.5. Carpetas de resultados

| Carpeta            | Contenido                                             |
| ------------------ | ----------------------------------------------------- |
| `data/resultados/` | Archivos de métricas JSONL generados por el simulador |
| `data/diagramas/`  | Diagramas PNG generados por el visualizador           |
| `docs/html/`       | Documentación HTML generada por Doxygen               |
| `docs/latex/`      | Documentación LaTeX y PDF generados por Doxygen       |
