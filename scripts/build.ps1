if (Get-Command "ninja" -ErrorAction SilentlyContinue) {
  $Generator = "-G Ninja"
} else {
  $Generator = ""
}

cmake -S . -B build $Generator
cmake --build build