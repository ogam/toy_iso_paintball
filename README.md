# toy_iso_paintball

Isometric demo game using [cute framework](https://github.com/RandyGaul/cute_framework)  
This is very bare bones game at the moment, it has an editor to create levels.  
Majority of the game's data is setup in `/data/meta/*.json` from entity components to campaigns.  

<img src="resources/demo.gif"></img>

## Project
Project uses ECS and deserializes json to attach components to entities, check under `src/assets/assets.c` to adjust any deserialization.  

## Sprites  
You can change out the sprites for tiles and units, tile size is calculated at start up time to walk through all the tiles to find out how big they are.  
Tiles need to all be the same size otherwise you'll get the wrong draw offsets between rows/columns.  
Current tiles are 32x32 but this should work with any other tile size, this has been tested with [Kenney Sketch Town Expansion Set](https://kenney.nl/assets/sketch-town-expansion) which has tiles at 256x352.  
Units if they are not correctly placed on top of the tile, you will need to create a aseprite slice called `origin` with the offset changed, check `data/sprites/cleric.ase` as an example of this.  

## Editor
Game includes an editor, to create new brushes you'll need to create a new `json` file.  
Check under `data/meta/`, examples of units are `baldy.json`, `enemy_range.json`, `enemy_charger.json`  
Anything with an editor block that includes a category of `tile`, `unit` or `object` will be added to the editor brush list.  
There isn't an editor for campaigns, this must be typed out in json, check example under `data/meta/campaign_0.json`.  
Every campaign includes a credits block so you can reference creators and any art and music used for your campaign.  

[![Quick Editor Example](https://img.youtube.com/vi/tgmGdtFuQWA/0.jpg)](https://www.youtube.com/watch?v=tgmGdtFuQWA)

## Build
run `build.bat`

## Notes  
Currently only tested on windows with msvc.  
If there are any problems with other platforms, it might be due to using `minicoro.h` and not having a stackful coroutine supported.  
