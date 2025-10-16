#include "util/util.h"

fixed RLE* grid_to_rle(V2i size, s32* data, fixed RLE* buffer_opt)
{
    fixed RLE* lines = NULL;
    if (buffer_opt)
    {
        lines = buffer_opt;
        cf_array_clear(lines);
    }
    else
    {
        MAKE_SCRATCH_ARRAY(lines, size.y);
    }
    
    for (s32 y = 0; y < size.y; ++y)
    {
        s32 x = 0;
        while (x < size.x)
        {
            V2i position = v2i(.x = x, .y = y);
            s32 index = x + y * size.x;
            s32 id = data[index];
            s32 rle_count = 0;
            
            while (x < size.x)
            {
                if (data[index + rle_count] == id)
                {
                    ++rle_count;
                    ++x;
                }
                else
                {
                    break;
                }
            }
            
            if (id)
            {
                RLE line = 
                {
                    .position = position,
                    .value = id,
                    .length = rle_count,
                };
                
                if (cf_array_count(lines) >= cf_array_capacity(lines))
                {
                    GROW_SCRATCH_ARRAY(lines);
                }
                
                cf_array_push(lines, line);
            }
        }
    }
    
    return lines;
}

void rle_to_grid(V2i size, s32* data, fixed RLE* lines)
{
    for (s32 line_index = 0; line_index < cf_array_count(lines); ++line_index)
    {
        RLE rle = lines[line_index];
        s32 index = rle.position.x + rle.position.y * size.x;
        s32 copy_offset = 0;
        
        while (copy_offset < rle.length)
        {
            data[index + copy_offset++] = rle.value;
        }
    }
}