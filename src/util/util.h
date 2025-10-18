#ifndef UTIL_H
#define UTIL_H

typedef struct RLE
{
    V2i position;
    s32 value;
    s32 length;
} RLE;

fixed RLE* grid_to_rle(V2i size, s32* data, fixed RLE* buffer_opt);
void rle_to_grid(V2i size, s32* data, fixed RLE* lines);

fixed struct Switch_Link_Packed* pack_switch_links(dyna Switch_Link* switch_links);
fixed struct Switch_Link* unpack_switch_links(dyna Switch_Link_Packed* packed_list);

#endif //UTIL_H
