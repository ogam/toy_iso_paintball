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

fixed Switch_Link_Packed* pack_switch_links(dyna Switch_Link* switch_links)
{
    fixed Switch_Link_Packed* packed_list = NULL;
    MAKE_SCRATCH_ARRAY(packed_list, cf_array_count(switch_links));
    
    for (s32 index = 0; index < cf_array_count(switch_links); ++index)
    {
        Switch_Link* switch_link = switch_links + index;
        Switch_Link_Packed packed = 
        {
            .source_x = switch_link->source.x,
            .source_y = switch_link->source.y,
            .region_min_x = switch_link->region.min.x,
            .region_min_y = switch_link->region.min.y,
            .region_max_x = switch_link->region.max.x,
            .region_max_y = switch_link->region.max.y,
            .stairs_top_x = switch_link->stairs_top.x,
            .stairs_top_y = switch_link->stairs_top.y,
            .stairs_step_rate_x = switch_link->stairs_step_rate.x,
            .stairs_step_rate_y = switch_link->stairs_step_rate.y,
            .mod_x = switch_link->mod.x,
            .mod_y = switch_link->mod.y,
            .end_elevation = switch_link->end_elevation,
            .state = switch_link->state,
            .speed = switch_link->speed,
            .cascade_delay = switch_link->cascade_delay,
        };
        
        cf_array_push(packed_list, packed);
    }
    
    return packed_list;
}

fixed Switch_Link* unpack_switch_links(dyna Switch_Link_Packed* packed_list)
{
    fixed Switch_Link* switch_links = NULL;
    MAKE_SCRATCH_ARRAY(switch_links, cf_array_count(packed_list));
    CF_MEMSET(switch_links, 0, sizeof(switch_links[0]) * cf_array_count(packed_list));
    
    for (s32 index = 0; index < cf_array_count(packed_list); ++index)
    {
        Switch_Link_Packed* packed = packed_list + index;
        Switch_Link switch_link = 
        {
            .source = v2i(.x = (s32)packed->source_x, (s32)packed->source_y),
            .region = {
                .min = v2i(.x = (s32)packed->region_min_x, .y = (s32)packed->region_min_y),
                .max = v2i(.x = (s32)packed->region_max_x, .y = (s32)packed->region_max_y),
            },
            .stairs_top = v2i(.x = (s32)packed->stairs_top_x, (s32)packed->stairs_top_y),
            .stairs_step_rate = v2i(.x = (s32)packed->stairs_step_rate_x, .y = (s32)packed->stairs_step_rate_y),
            .mod = v2i(.x = (s32)packed->mod_x, .y = (s32)packed->mod_y),
            .end_elevation = packed->end_elevation,
            .state = packed->state,
            .speed = packed->speed,
            .cascade_delay = packed->cascade_delay,
        };
        
        cf_array_push(switch_links, switch_link);
    }
    
    return switch_links;
}