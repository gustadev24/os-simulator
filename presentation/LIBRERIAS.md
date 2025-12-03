# LIBRERIAS

Copiado del archivo README.md

Algunas librerias son descargables desde internet, asi que las instrucciones se adjuntan en este documento también.

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
