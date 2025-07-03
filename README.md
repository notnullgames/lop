This is a very simple tester for null0 tiled map API.

I create a game-map style `adventure_map.h`, and a little user-code to define what happens, which could be lightly modified to do some other style (like SMB) then things are defined as objects in the [Tiled](https://www.mapeditor.org/) maps.

![screenshot](screenshot.png)

I am using [this fork of pntr_tiled](https://github.com/RobLoach/pntr_tiled/pull/24) that has external tilesheets and better object-support.

```shell
# build native
$ cmake -B build
$ cmake --build build

$ ./build/lop
```

for web:


```shell
# build web
$ emcmake cmake -B wbuild
$ cmake --build wbuild

# web-server
$ npx -y live-server docs

# auto-reload build (in another terminal)
$ watch 'cmake --build wbuild'
```

for debugging:

```shell
# for fast-build/debugging
$ cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build

# debugging
$ lldb build/lop
(lldb) r
(lldb) bt
```

## collisions

Add a couple object-layers to your map:

- `objects` - put the player & anything they interact with here. Collision is per-tile-square.
- `collisions` - I also want non-interactive (static geometry) collisions, but there some issues: cute_tiled does not like shapes, etc