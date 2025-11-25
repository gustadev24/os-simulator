set shell := ["bash", "-cu"]

build:
  cmake -S . -B build
  cmake --build build

test: build
  ./build/bin/tests

run: build
  ./build/bin/os_simulator

demo-io:
  g++ -std=c++17 -o build/bin/demo_io examples/demo_io.cpp src/core/process.cpp src/io/io_device.cpp src/io/io_manager.cpp -I./include -pthread
  ./build/bin/demo_io

clean:
  rm -rf build

build-docs:
  doxygen

docs-serve: build-docs
  pushd docs/html && python3 -m http.server 8000 && popd

docs-pdf: build-docs
  pushd docs/latex && make && popd
