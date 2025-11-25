set shell := ["bash", "-cu"]

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

docs-serve: build-docs
    pushd docs/html && python3 -m http.server 8000 && popd

docs-pdf: build-docs
    pushd docs/latex && make && popd
