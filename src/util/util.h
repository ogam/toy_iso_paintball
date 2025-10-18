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

static inline int popcount(u32 x) {
#if defined(_MSC_VER) && defined(_M_X64)
    return (int)__popcnt64(x);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    int count = 0;
    while (x) {
        x &= (x - 1);
        ++count;
    }
    return count;
#endif
}

#endif //UTIL_H
