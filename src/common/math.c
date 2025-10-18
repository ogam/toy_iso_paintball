#include "common/math.h"

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

CF_V2 v2i_to_v2(V2i tile, f32 elevation)
{
    f32 x = (f32)tile.x + elevation;
    f32 y = (f32)tile.y + elevation;
    
    return cf_v2(x, y);
}

CF_V2 v2i_to_v2_iso(V2i tile, f32 elevation)
{
    CF_M2x2 m = calc_iso_m2();
    CF_V2 p = v2i_to_v2(tile, elevation);
    return cf_mul_m2_v2(m, p);
}

CF_V2 v2i_to_v2_iso_center(V2i tile, f32 elevation)
{
    CF_M2x2 m = calc_iso_m2();
    CF_V2 p = v2i_to_v2(tile, elevation);
    p.x += 0.5f;
    p.y += 0.5f;
    
    return cf_mul_m2_v2(m, p);
}

V2i v2_to_v2i(CF_V2 p)
{
    V2i v = v2i(.x = (s32)CF_FLOORF(p.x), 
                .y = (s32)CF_FLOORF(p.y));
    return v;
}

V2i screen_v2_to_v2i(CF_V2 p)
{
    CF_M2x2 inv_m = calc_iso_inv_m2();
    
    p = cf_mul_m2_v2(inv_m, p);
    V2i tile = v2i(.x = (s32)CF_FLOORF(p.x), 
                   .y = (s32)CF_FLOORF(p.y));
    return tile;
}

V2i v2i_mul_i(V2i a, s32 v)
{
    return v2i(.x = a.x * v, .y = a.y * v);
}

V2i v2i_add(V2i a, V2i b)
{
    return v2i(.x = a.x + b.x, .y = a.y + b.y);
}

V2i v2i_sub(V2i a, V2i b)
{
    return v2i(.x = a.x - b.x, .y = a.y - b.y);
}

V2i v2i_neg(V2i v)
{
    return v2i(.x = -v.x, .y = -v.y);
}

V2i v2i_perp(V2i v)
{
    return v2i(.x = v.y, .y = -v.x);
}

V2i v2i_sign(V2i v)
{
    return v2i(.x = cf_clamp_int(v.x, -1, 1), .y = cf_clamp_int(v.y, -1, 1));
}

// since these are s32 normalizing this would be to find the smallest
// units for x,y to still make up the same line shape
V2i v2i_norm(V2i v)
{
    s32 gcd = get_gcd(v.x, v.y);
    V2i result = gcd ? v2i(.x = v.x / gcd, .y = v.y / gcd) : v2i();
    return result;
}

s32 v2i_len_sq(V2i v)
{
    return v.x * v.x + v.y * v.y;
}

s32 v2i_distance(V2i a, V2i b)
{
    return cf_abs_int(b.x - a.x) + cf_abs_int(b.y - a.y);
}

s32 v2i_size(V2i v)
{
    return v.x * v.y;
}

V2i v2i_min(V2i a, V2i b)
{
    return v2i(.x = cf_min(a.x, b.x), .y = cf_min(a.y, b.y));
}

V2i v2i_max(V2i a, V2i b)
{
    return v2i(.x = cf_max(a.x, b.x), .y = cf_max(a.y, b.y));
}

V2i v2i_clamp(V2i v, V2i min, V2i max)
{
    return v2i(.x = cf_clamp_int(v.x, min.x, max.x), .y = cf_clamp_int(v.y, min.y, max.y));
}

V2i v2i_abs(V2i v)
{
    return v2i(.x = cf_abs_int(v.x), .y = cf_abs_int(v.y));
}

Aabbi make_aabbi(V2i min, V2i max)
{
    Aabbi aabb = {
        .min = v2i_min(min, max),
        .max = v2i_max(min, max),
    };
    
    return aabb;
}

b32 aabbi_contains(Aabbi aabb, V2i v)
{
    return v.x >= aabb.min.x && v.y >= aabb.min.y && v.x <= aabb.max.x && v.y <= aabb.max.y;
}

V2i aabbi_center(Aabbi aabb)
{
    V2i extents = v2i_sub(aabb.max, aabb.min);
    extents.x /= 2;
    extents.y /= 2;
    return v2i_add(aabb.min, extents);
}

Aabbi aabbi_clamp(Aabbi a, Aabbi b)
{
    V2i min = v2i_clamp(a.min, b.min, b.max);
    V2i max = v2i_clamp(a.max, b.min, b.max);
    return make_aabbi(min, max);
}

inline b32 f32_is_zero(f32 v)
{
    return cf_abs(v) < 1e-7f;
}

s32 get_gcd(s32 a, s32 b)
{
    a = cf_abs_int(a);
    b= cf_abs_int(b);
    s32 divisor = cf_min(a, b);
    s32 v0 = cf_max(a, b);
    s32 v1 = divisor;
    
    while (divisor)
    {
        v0 = v0 % divisor;
        divisor = cf_min(v0, v1);
        v0 ^= v1;
        v1 ^= v0;
        v0 ^= v1;
    }
    
    return v0;
}

s32 next_power_of_two(s32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

// incase some cpu doesn't support _mm_popcnt_u32
// https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
s32 popcnt_s32(s32 v)
{
    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
    s32 c = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
    return c;
}