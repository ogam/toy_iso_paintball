#ifndef MACROS_H
#define MACROS_H

#ifndef UNUSED
#define UNUSED(VAR) (void)VAR
#endif

#ifndef MAKE_SCRATCH_PQ
#define MAKE_SCRATCH_PQ(ARR, COUNT) \
{ \
ARR = NULL; \
int __pq_alloc_size = pq_alloc_size(COUNT, sizeof(ARR[0])); \
void* __pq_buffer = scratch_alloc(__pq_alloc_size); \
pq_static((ARR), __pq_buffer, __pq_alloc_size); \
}
#endif


#ifndef GROW_SCRATCH_PQ
#define GROW_SCRATCH_PQ(ARR) \
{ \
int __old_capacity = pq_capacity(ARR); \
int __capacity = cf_max(__old_capacity * 2, 8); \
int __arr_alloc_size = pq_alloc_size(__capacity, sizeof(ARR[0])); \
void* __arr_buffer = scratch_alloc(__arr_alloc_size); \
void* __old_arr = ARR; \
pq_static((ARR), __arr_buffer, __arr_alloc_size); \
if (__old_capacity > 0) \
{ \
pq_set((ARR), __old_arr); \
} \
}
#endif

#ifndef MAKE_SCRATCH_ARRAY
#define MAKE_SCRATCH_ARRAY(ARR, COUNT) \
{ \
ARR = NULL; \
int __arr_alloc_size = sizeof(CF_Ahdr) +  sizeof(ARR[0]) * COUNT; \
void* __arr_buffer = scratch_alloc(__arr_alloc_size); \
cf_array_static((ARR), __arr_buffer, __arr_alloc_size); \
}
#endif

#ifndef GROW_SCRATCH_ARRAY
#define GROW_SCRATCH_ARRAY(ARR) \
{ \
int __old_capacity = cf_array_capacity(ARR); \
int __capacity = cf_max(__old_capacity * 2, 8); \
int __arr_alloc_size = sizeof(CF_Ahdr) +  sizeof(ARR[0]) * __capacity; \
void* __arr_buffer = scratch_alloc(__arr_alloc_size); \
void* __old_arr = ARR; \
cf_array_static((ARR), __arr_buffer, __arr_alloc_size); \
if (__old_capacity > 0) \
{ \
cf_array_set((ARR), __old_arr); \
} \
}
#endif

#ifndef ARRAY_REMOVE_AT
#define ARRAY_REMOVE_AT(ARR, INDEX) \
{ \
if (INDEX < cf_array_count(ARR)) \
{ \
if (cf_array_count(ARR) == 1) \
{ \
cf_array_pop(ARR); \
} \
else  \
{ \
s32 __count = cf_array_count(ARR) - INDEX - 1; \
void* __dst = (ARR) + INDEX; \
void* __src = (ARR) + INDEX + 1; \
CF_MEMCPY(__dst, __src, sizeof((ARR)[0]) * __count); \
cf_array_len(ARR)--; \
} \
} \
}
#endif

#ifndef ARRAY_INSERT_AT
#define ARRAY_INSERT_AT(ARR, INDEX, DATA) \
{ \
if (cf_array_count(ARR) >= cf_array_capacity(ARR)) \
{ \
if (CF_AHDR(ARR)->is_static) \
{ \
GROW_SCRATCH_ARRAY(ARR); \
} \
else \
{ \
cf_array_fit(ARR, cf_min(cf_array_capacity(ARR) * 2, 8)); \
} \
} \
s32 __index = cf_max(INDEX, 0); \
s32 __count = cf_array_count(ARR) - __index; \
void* __dst = (ARR) + __index + 1; \
void* __src = (ARR) + __index; \
CF_MEMCPY(__dst, __src, sizeof((ARR)[0]) * __count); \
(ARR)[__index] = DATA; \
cf_array_len(ARR)++; \
}
#endif


// statically allocated array hints
#ifndef fixed
#define fixed
#endif

// @reference
// @randy discord
#if 0
// original
#define resume(co, fn) do { if (!(co.id)) { co = make_co(fn, NULL); } cf_coroutine_resume(co); } while (0)
resume(co, my_update_function);
#endif
#define MCO_RESUME(CO, FN) \
{ \
if (mco_status(CO) == MCO_DEAD) \
{ \
mco_desc desc = mco_desc_init(FN, 0); \
mco_result result = mco_init(CO, &desc); \
CF_ASSERT(result == MCO_SUCCESS); \
} \
mco_resume(CO); \
}

#endif //MACROS_H
