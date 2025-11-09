# Simulador Integrado de Planificación de Procesos y Gestión de Memoria Virtual

## Descripción

El simulador desarrollado integra módulos de planificación de procesos y gestión de memoria virtual, manteniendo control en la interacción de ambos mediante el uso de políticas establecidas con el objetivo de lograr un rendimiento óptimo del sistema.

## Desarrollo

### Requisitos

- CMake
- Compilador C++ (GCC, Clang, etc.)
- Just (opcional, para comandos)

### Servidor de lenguaje
Se recomienda usar el servidor de lenguaje clangd que ya se encuentra configurado para este proyecto.

### Recomendaciones para el desarrollo

Se usan las siguentes herramientas para el desarrollo del proyecto, se recomienda instalarlas antes de empezar a desarrollar.
* [cmake](https://cmake.org/download/): Para automatización de compilación
* [just](https://github.com/casey/just): Para automatización de scripts

### Comandos útiles

#### Compilación
La primera build puede tardar un poco más ya que se descargan y construyen las dependencias.

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

Usando CMake:

```bash
cmake -S . -B build
./build/bin/os_simulator
```

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
