# Virtual Memory Management

## Descripción

todo

## Desarrollo

### Requisitos

- CMake
- Compilador C++ (GCC, Clang, etc.)
- Just (opcional, para comandos)

### Servidor de lenguaje
Se recomienda usar el servidor de lenguaje clangd que ya se encuentra configurado para este proyecto.

### Comandos utiles

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
