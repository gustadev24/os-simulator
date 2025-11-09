build:
  cmake -S . -B build
  cmake --build build

test: build
  ./build/bin/tests

run: build
  ./build/bin/os_simulator

clean: 
  rm -rf build

build-docs:
  doxygen

build-docs-serve: build-docs
  pushd docs/html && python3 -m http.server 8000 && popd

build-docs-pdf: build-docs
  pushd docs/latex && make && popd