[unix]
build:
    ./scripts/build.sh

[windows]
build:
    ./scripts/build.ps1

test: build
    ./build/bin/tests

test-combinations: build
    ./build/bin/tests "[combinations]"

run: build
    ./build/bin/os_simulator

clean:
    rm -rf build

build-docs:
    doxygen

docs-serve: build-docs
    cd docs/html && python3 -m http.server 8000

docs-pdf: build-docs
    cd docs/latex && make

report:
    mkdir -p report/build
    pushd report/src && pdflatex -output-directory=../build informe.tex && popd

report-bib:
    mkdir -p report/build
    pushd report/src && latexmk -pdf -output-directory=../build informe.tex && popd

report-clean:
    rm -rf report/build

visualize:
    python3 -m visualization

visualize-batch:
    python3 -m visualization --batch data/resultados/ data/diagramas/batch/