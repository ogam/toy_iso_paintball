## Drawing

There are 3 spaces we're dealing with for a 2d renderer
- grid is a flat 2 axes 
- isometric 3 axes space (same as grid with elevation)
- world space this is a 2 axes space used to draw to the screen.

Main drawing for isometric tiles is to use a 2x2 matrix, this is used in `src/common/math.h` and varying `v2i_*` functions to go from grid/isometric space to world space along with `v2_*` to go from world to isometric space.  

```
CF_M2x2 calc_iso_m2()
{
    CF_V2 tile_size = assets_get_tile_size();
    CF_M2x2 m = 
    {
        .x = cf_v2(0.5f * tile_size.x, 0.25f * tile_size.y),
        .y = cf_v2(-0.5f * tile_size.x, 0.25f * tile_size.y),
    };
    
    return m;
}

CF_M2x2 calc_iso_inv_m2()
{
    CF_V2 tile_size = assets_get_tile_size();
    f32 a = 0.5f * tile_size.x;
    f32 b = -0.5f * tile_size.x;
    f32 c = 0.25f * tile_size.y;
    f32 d = 0.25f * tile_size.y;
    f32 det_denom = a * d - b * c;
    f32 det = det_denom == 0.0f ? 0.0f : 1.0f / det_denom;
    
    CF_M2x2 m = 
    {
        .x = cf_v2(d, -c),
        .y = cf_v2(-b, a),
    };
    
    m = cf_mul_m2_f(m, det);
    
    return m;
}
```

From there you can use this 2x2 matrix to calculate from a grid -> isometric space and back, for a flat world this is pretty much all you really need along with a known size for all of your tiles, tile size in this project is calculated at run time depending on the tile sprites that are used but ideally it should be known during development time to know what your tile size should be.  

For this project tiles can be stacked to show some elvation, all tiles are setup in `src/game/entity.h` under the `World` struct, these are all packed into a `Tile` struct for the base tile information such as `walkable` `hazard` `slope` `elevation` `tiling` etc as either bit fields or s8 for `elevation`, current elevation in this tile struct is every additional value raises the tile by half an elevation, every 2 is a full tile elevation.  So you could end up with 63.5 elevation (127) before overflowing, this can be a u8 so you would end up with 127.5 elevation total. So essentially `Tiles* tiles` is mainly a height map. To make tiles move up and down there is additional float height map called `tile_elevation_offsets`, this is use to have some varying elevation differences between tiles to make them seem like they're floating and an additional optional float height map `tile_elevation_velocity_offsets` which is to keep track of all velocities of each tile over time to make it seem like they have some weight for wave effects and bounciness.  

Draw order to avoid tiles clipping each other is done from left to right (-X -> +X), back to front (+Y -> -Y), bottom to top (-Z -> +Z) for tiles.  

```
// sort key is setup to be drawn back to front, left to right, bottom to top
// left to right high bit range (x)
// back to front      mid range (y)
// elevation          low range
typedef union Draw_Sort_Key
{
    struct
    {
        // lowest bit range should be based on types to allow a prop to be drawn first before
        // a unit does
        u64 type : 8;
        // elevation here is fairly large compared to other parts of this key
        // mainly because total elevation is a f32 which means it can vary a lot
        // we have subtle elvation differences when there's any velocity applied
        // to tiles so these need to be rendered in the correct order otherwise
        // it'll be misleading which one is in front
        u64 elevation : 32;
        u64 y : 8;
        u64 x : 8;
        // not really used for now, should really use it for the background
        u64 layer : 8;
    };
    u64 value;
} Draw_Sort_Key;
```

## Tile Picking

Due to having elevation here each tile query from grid -> isometric -> world space needs to include an elevation parameter since this will affect where to draw and how to see if your mouse is hovered over a tile's surface.  

Going backwards from world -> isometric is a bit different since we don't know what the elevation value is there are a few options to handle this
- You could draw each tile with a different color to a texture and sample off of that to figure out which tile was hovered over, this is probably the most accuracte since it is a WYSIWYG (what you see is what you get), it's currently not doing this but most likely it should be since it gives a very clear representation of what you should be mousing over and more pixel perfect.  
- The other way is sampling against the Y world space, this mostly works if all elevation is the same for the level but not when there's varying elevation.  
- Current way is to create a set of tile surface vertices then do a quickhull to check if your mouse position is within or outside of that polygon, due to depth (elevation) causing tiles closer to the bottom of the screen to draw on top of the ones further back, it also has to do this N times walking direction <-1, -1> every iteration to figure out if you're currently hovering over any tiles below the current sampled tile. Not ideal but works for now since we do have top surface tiles being drawn separately from the stack of tiles below due to any offsets being applied.  


## Auto Tiling
This is an optional mode that's only available to the editor, this allows you to specify if you want to auto tile a portion or the level or to stick with default `tall_c` / `short_c` animations for each tile. Currently the tileset that's being used doesn't cover many of the edge cases. At the moment the suffix is in the following order for each auto tile side, `n` -> `s` -> `w` -> `e`. So whenver there's an open tile connection to the NE / EN the suffix will always be `ne`, if there's a 3 connections say NES / NSE / SEN / SNE this is all the same and suffix will be `nse`. Failure to find the animation for these tile connections will default back to `c` or the center animation. If all sides are open then a suffix of `o` will be added to the animation so something like `tall_o` or `short_o` again defaulting back to `c` if it's not available.  
Auto tiling is also based on elevation and tile id, lower elevation will close connections against higher elevation tiles whereas higher elevation tiles will leave the side with lower elevtion open. 
```
...
.sS
...
```
Example here being `s` will be closed to the right facing `S`, and `S` will have it's left open to `s`.  