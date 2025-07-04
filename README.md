This is a very simple tester for null0 tiled map API.

I create a game-map style `adventure.h`, and a little user-code to define what happens, which could be lightly modified to do some other style (like SMB) then things are defined as objects in the [Tiled](https://www.mapeditor.org/) maps.

![screenshot](screenshot.png)

I am using [this fork of pntr_tiled](https://github.com/RobLoach/pntr_tiled/pull/24) that has external tilesheets and better object-support.


I added some no-install convenience scripts, if you have node installed:

```bash

# watch web for change, rebuild
npm start

# watch native for change, rebuild
npm run native:watch



# just build native (./build/lop)
npm run native

# just build web (docs/)
npm run web

# clean up any built files
npm run clean

```

## collisions

Add a couple object-layers to your map:

- `objects` - put the player & anything they interact with here. Collision is per-tile-square.
- `collisions` - I also want non-interactive (static geometry) collisions, but there some issues: cute_tiled does not like shapes, etc