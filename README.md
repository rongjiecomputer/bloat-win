# Bloat for Windows

Visualize binary size data by [SymbolSort](https://github.com/adrianstone55/SymbolSort)
with [webtreemap](https://github.com/evmar/webtreemap).

Inspired by https://github.com/evmar/bloat (for Linux and Mac).

## Getting started

1. Build your binary with `/Zi` get `.pdb` debug symbol file.

3. Download SymbolSort and run `SymbolSort -in input.pdb -out out.txt`.

4. Set up Visual C++ build environment with `vcvarsall.bat` and build `dump.cpp`
with `cl /O2 dump.cpp /link /out:dump.exe`.

5. Run `dump -in out.txt -out dump.json`.

6. Open `index.html` for visualization.

## Demo

https://rongjiecomputer.github.io/bloat-win/v8.html

## Tips

If you want to share the generated JSON on Internet, it is good to minify it with http://closure-compiler.appspot.com/home

Warning: do not use "Advanced" optimization mode!

## Similar project you might be interested

- https://github.com/google/bloaty
