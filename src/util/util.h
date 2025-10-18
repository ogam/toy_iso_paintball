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
    // incase some cpu doesn't support _mm_popcnt_u32
    // https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    x = x - ((x >> 1) & 0x55555555);                    // reuse input as temporary
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);     // temp
    int count = ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
    return count;
#endif
}

#endif //UTIL_H
