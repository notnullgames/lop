This is a very simple tester for null0 tiled map API.

I create a game-map style `adventure_map.h`, and a little user-code to define what happens, which could be lightly modified to do some other style (like SMB) then things are defined as objects in the [Tiled](https://www.mapeditor.org/) maps.

![screenshot](screenshot.png)

```sh
# build native
cmake -B build
cmake --build build

# build web
emcmake cmake -B wbuild
cmake --build wbuild

# web-server
npx -y live-server docs

# auto-reload build (in another terminal)
watch 'cmake --build wbuild
```