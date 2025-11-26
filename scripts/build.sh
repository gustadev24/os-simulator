#!/usr/bin/env bash

if command -v ninja >/dev/null 2>&1; then
  GENERATOR="-G Ninja"
else
  GENERATOR=""
fi

cmake -S . -B build $GENERATOR
cmake --build build
