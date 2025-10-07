#ifndef MATH_H
#define MATH_H

typedef struct V2i
{
    s32 x;
    s32 y;
} V2i;

#define v2i(...) ((V2i){.x = 0, .y = 0, __VA_ARGS__})

typedef struct Aabbi
{
    V2i min;
    V2i max;
} Aabbi;

CF_M2x2 calc_iso_m2();
CF_M2x2 calc_iso_inv_m2();
CF_V2 v2i_to_v2(V2i tile, f32 elevation);
CF_V2 v2i_to_v2_iso(V2i tile, f32 elevation);
CF_V2 v2i_to_v2_iso_center(V2i tile, f32 elevation);
V2i v2_to_v2i(CF_V2 p);
V2i screen_v2_to_v2i(CF_V2 p);

V2i v2i_mul_i(V2i a, s32 v);
V2i v2i_add(V2i a, V2i b);
V2i v2i_sub(V2i a, V2i b);
V2i v2i_neg(V2i v);
V2i v2i_perp(V2i v);
V2i v2i_sign(V2i v);
V2i v2i_norm(V2i v);
s32 v2i_len_sq(V2i v);
s32 v2i_distance(V2i a, V2i b);
s32 v2i_size(V2i v);
V2i v2i_min(V2i a, V2i b);
V2i v2i_max(V2i a, V2i b);
V2i v2i_clamp(V2i v, V2i min, V2i max);

Aabbi make_aabbi(V2i min, V2i max);
b32 aabbi_contains(Aabbi aabb, V2i v);

b32 f32_is_zero(f32 v);
s32 get_gcd(s32 a, s32 b);

#endif //MATH_H
