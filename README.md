This is a very simple tester for null0 tiled map API.

```sh
# native
cmake -B build
cmake --build build

# web
emcmake cmake -B wbuild
cmake --build wbuild
npx -y live-server docs
```