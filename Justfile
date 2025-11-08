build:
  cmake -S . -B build
  cmake --build build

test: build
  ./build/bin/tests

run: build
  ./build/bin/os_simulator

clean: 
  rm -rf build