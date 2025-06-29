This is a very simple tester for null0 tiled map API.

I create a game-map style `adventure_map.h`, and a little user-code to define what happens, which could be lightly modified to do some other style (like SMB) then things are defined as objects in the [Tiled](https://www.mapeditor.org/) maps.

![screenshot](screenshot.png)

```sh
# native
cmake -B build
cmake --build build

# web
emcmake cmake -B wbuild
cmake --build wbuild
npx -y live-server docs
```