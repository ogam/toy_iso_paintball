// doesn't include sanity checks or check for memory corruption
// @reference: https://github.com/RandyGaul/cute_framework/blob/master/include/cute_array.h
// @reference: https://github.com/nothings/stb
// @reference: https://github.com/mattiasgustavsson/libs

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

// priority queues here defaults as ascending low->high, make sure to call pq_set_descending() if your
// algorithm assumes high->low

#include <stdbool.h>
#include <stdint.h>

#ifndef PQ_ALLOC
#include <stdlib.h>
#define PQ_ALLOC malloc
#endif

#ifndef PQ_FREE
#include <stdlib.h>
#define PQ_FREE free
#endif

#ifndef PQ_MEMCPY
#include <string.h>
#define PQ_MEMCPY memcpy
#endif

#ifndef PQ_MEMCMP
#include <string.h>
#define PQ_MEMCMP memcmp
#endif


#ifndef PQ_ASSERT
#define PQ_ASSERT(EXP) { if (!(EXP)) *(int*)0 = 0; }
#endif

#define pq

// example usage
#if 0
void ints()
{
    pq int* values = NULL;
    pq_add(values, 1, 0.0f);
    pq_add(values, 2, 1.0f);
    pq_add(values, 3, 3.0f);
    pq_add(values, 4, 2.0f);
    float* weights = pq_weights(values);
    
    // order should be [1, 2, 4, 3]
    for (int index = 0; index < pq_count(values); ++index)
    {
        printf("[%d] - %d, %.2f\n", index, values[index], weights[index]);
    }
    pq_free(values);
}

void deal_damage(pq int64_t* threat_list, int64_t handle, float damage)
{
    bool damage_dealt = false;
    for (int index = 0; index < pq_count(threat_list); ++index)
    {
        if (threat_list[index] == handle)
        {
            pq_add_weight(threat_list, index, damage);
            damage_dealt = true;
            break;
        }
    }
    if (!damage_dealt)
    {
        pq_add(threat_list, handle, damage);
    }
}

void threat()
{
    int64_t handles[] = 
    {
        1, 2, 3
    };
    pq int64_t* threat_list = NULL;
    // make sure to initialize `threat_list`, passing in NULL to deal_damage() you'll end up
    // leaking memory
    pq_fit(threat_list, 8);
    pq_set_descending(threat_list);
    deal_damage(threat_list, handles[0], 100.0f);
    deal_damage(threat_list, handles[1], 50.0f);
    deal_damage(threat_list, handles[2], 300.0f);
    deal_damage(threat_list, handles[0], 100.0f);
    
    // [3] - 300.0
    // [1] - 200.0
    // [2] - 50.0
    printf("threat list:\n");
    for (int index = 0; index < pq_count(threat_list); ++index)
    {
        printf("\t[%" PRId64 "]: %.2f\n", threat_list[index], pq_weights(threat_list)[index]);
    }
    
    pq_set_ascending(threat_list);
    // [2] - 50.0
    // [1] - 200.0
    // [3] - 300.0
    printf("threat list:\n");
    for (int index = 0; index < pq_count(threat_list); ++index)
    {
        printf("\t[%" PRId64 "]: %.2f\n", threat_list[index], pq_weights(threat_list)[index]);
    }
    
    pq_free(threat_list);
}

#endif

#define pq_header_size() (sizeof(PQHeader))
#define pq_header(ARR) ((PQHeader*)ARR - 1)
// convenience macro for allocating for static priority queues
//   size_t alloc_size = pq_alloc_size(34, sizeof(int);
//   void* buffer = malloc(alloc_size);
//   int* list = NULL;
//   pq_static(list, buffer, alloc_size);
#define pq_alloc_size(COUNT, SIZE) (pq_header_size() + (SIZE + sizeof(float)) * COUNT + SIZE)
#define pq_count(ARR) ((ARR) ? pq_header(ARR)->count : 0)
//  @note:  this can crash if you are trying to grab pq_len() on a NULL pq, use pq_count() if you want
//          to be safe, pq_len() if you want to be faster / want to set the length
#define pq_len(ARR) pq_header(ARR)->count
#define pq_last(ARR) ((ARR)[pq_header(ARR)->count - 1])
#define pq_clear(ARR) { if (ARR) pq_header(ARR)->count = 0; }
#define pq_capacity(ARR) ((ARR) ? pq_header(ARR)->capacity : 0)
#define pq_is_static(ARR) ((ARR) ? pq_header(ARR)->is_static : false)
#define pq_weights(ARR) ((ARR) ? pq_header(ARR)->weights : NULL)
#define pq_temp_index(ARR) ((ARR) ? pq_header(ARR)->capacity : 0)
#define pq_set_ascending(ARR) { if (ARR) pq__set_ascending(pq_header(ARR), true); }
#define pq_set_descending(ARR) { if (ARR) pq__set_ascending(pq_header(ARR), false); }

#define pq_grow(ARR, CAPACITY) \
{ \
PQHeader* hdr = pq_header(ARR); \
pq__grow(&hdr, CAPACITY); \
*(void**)&(ARR)= hdr->data; \
} 

#define pq_fit(ARR, CAPACITY) \
{ \
if ((ARR) == NULL) \
{ \
PQHeader* new_hdr = pq__make((int)sizeof((ARR)[0])); \
*(void**)&(ARR)= new_hdr->data; \
} \
pq_grow(ARR, CAPACITY); \
}

#define pq_add(ARR, DATA, WEIGHT) \
{ \
if ((ARR) == NULL) \
{ \
PQHeader* new_hdr = pq__make((int)sizeof((ARR)[0])); \
*(void**)&(ARR) = new_hdr->data; \
} \
pq_fit(ARR, pq_header(ARR)->count + 1); \
(ARR)[pq_temp_index(ARR)] = DATA; \
pq__add(pq_header(ARR), &(ARR)[pq_temp_index(ARR)], WEIGHT); \
}

#define pq_remove_at(ARR, INDEX) pq__remove(pq_header(ARR), INDEX)
#define pq_remove(ARR, ITEM) \
{ \
int __pq_item_index = pq_index_of(ARR, ITEM); \
pq_remove_at(ARR, __pq_item_index); \
}

#define pq_add_weight_at(ARR, INDEX, WEIGHT) \
{ \
if (INDEX >= 0 && INDEX < pq_count(ARR)) \
{ \
float __pq_weight = pq_weights(ARR)[INDEX] + WEIGHT; \
(ARR)[pq_temp_index(ARR)] = (ARR)[INDEX]; \
pq_remove_at((ARR), INDEX); \
pq__add(pq_header(ARR), &(ARR)[pq_temp_index(ARR)], __pq_weight); \
} \
}

#define pq_sub_weight_at(ARR, INDEX, WEIGHT) pq_add_weight_at(ARR, INDEX, -WEIGHT)

#define pq_set_weight_at(ARR, INDEX, WEIGHT) \
{ \
if (INDEX >= 0 && INDEX < pq_count(ARR)) \
{ \
float __pq_weight = WEIGHT; \
(ARR)[pq_temp_index(ARR)] = (ARR)[INDEX]; \
pq_remove_at(ARR, INDEX); \
pq__add(pq_header(ARR), &(ARR)[pq_temp_index(ARR)], __pq_weight); \
} \
}

#define pq_add_weight(ARR, ITEM, WEIGHT) \
{ \
int __pq_item_index = __pq_item_index = pq_index_of(ARR, ITEM); \
if (__pq_item_index >= 0) \
{ \
pq_add_weight_at(ARR, __pq_item_index, WEIGHT); \
} \
else \
{ \
pq_add(ARR, ITEM, WEIGHT); \
} \
}

#define pq_sub_weight(ARR, ITEM, WEIGHT) pq_add_weight(ARR, ITEM, -WEIGHT)

#define pq_set_weight(ARR, ITEM, WEIGHT) \
{ \
int __pq_item_index = __pq_item_index = pq_index_of(ARR, ITEM); \
if (__pq_item_index >= 0) \
{ \
pq_set_weight_at(ARR, __pq_item_index, WEIGHT); \
} \
else \
{ \
pq_add(ARR, ITEM, WEIGHT); \
} \
}

#define pq_index_of(ARR, ITEM) ( (ARR) ? (ARR)[pq_temp_index(ARR)] = ITEM, pq__index_of(pq_header(ARR), pq_header(ARR)->temp) : -1 )
#define pq_weight(ARR, ITEM) ( pq_index_of(ARR, ITEM) >= 0 ? pq_header(ARR)->weights[pq_index_of(ARR, ITEM)] : 0 )
#define pq_weight_at(ARR, INDEX) pq_header(ARR)->weights[INDEX]

#define pq_pop(ARR) ( pq_header(ARR)->count--, (ARR)[pq_count(ARR)] )
#define pq_pop_front(ARR) ( (ARR)[pq_temp_index(ARR)] = (ARR)[0], pq_remove_at(ARR, 0), (ARR)[pq_temp_index(ARR)] )

#define pq_static(ARR, BUFFER, SIZE) \
{ \
PQHeader* new_hdr = pq__make_static(BUFFER, SIZE, (int)sizeof((ARR)[0])); \
*(void**)&(ARR)= new_hdr->data; \
}

#define pq_set(SRC, DST) \
{ \
PQ_ASSERT(pq_header(SRC)->element_size == pq_header(DST)->element_size); \
if (pq_header(DST)->capacity < pq_header(SRC)->count) \
{ \
pq_grow((DST), pq_header(SRC)->count); \
} \
PQ_MEMCPY(pq_header(DST)->data, pq_header(SRC)->data, pq_header(SRC)->element_size * pq_header(SRC)->count); \
PQ_MEMCPY(pq_header(DST)->weights, pq_header(SRC)->weights, sizeof(float) * pq_header(SRC)->count); \
pq_header(DST)->count = pq_header(SRC)->count; \
}

#define pq_free(ARR) \
{ \
if ((ARR) && !pq_header(ARR)->is_static) \
{ \
PQ_FREE(pq_header(ARR)); \
(ARR) = NULL; \
} \
}

typedef struct PQHeader
{
    void* data;
    // used for swaping and adding new items without having to make a temp variable
    void* temp;
    float* weights;
    int count;
    int capacity;
    int element_size;
    bool is_static;
    bool is_ascending;
} PQHeader;

static PQHeader* pq__make_static(void* block, int size, int element_size)
{
    // includes temp slot
    int minimum_size = pq_alloc_size(0, element_size);
    
    PQ_ASSERT(size >= minimum_size);
    int remaining_size = size - minimum_size;
    int total_element_size = element_size + sizeof(float);
    int capacity = remaining_size / total_element_size;
    
    PQHeader* hdr = (PQHeader*)block;
    
    void* data = (uint8_t*)block + pq_header_size();
    void* temp = (uint8_t*)data + (element_size * capacity);
    float* weights = (float*)((uint8_t*)temp + element_size);
    
    hdr->data = data;
    hdr->temp = temp;
    hdr->weights = weights;
    hdr->count = 0;
    hdr->capacity = capacity;
    hdr->element_size = element_size;
    hdr->is_static = true;
    hdr->is_ascending = true;
    return hdr;
}

static PQHeader* pq__make(int element_size)
{
    int capacity = 8;
    int allocation_size = pq_alloc_size(capacity, element_size);
    void* block = PQ_ALLOC(allocation_size);
    
    PQHeader* hdr = (PQHeader*)block;
    void* data = (uint8_t*)block + pq_header_size();
    void* temp = (uint8_t*)data + (element_size * capacity);
    float* weights = (float*)((uint8_t*)temp + element_size);
    
    hdr->data = data;
    hdr->temp = temp;
    hdr->weights = weights;
    hdr->count = 0;
    hdr->capacity = capacity;
    hdr->element_size = element_size;
    hdr->is_static = false;
    hdr->is_ascending = true;
    
    return hdr;
}

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static uint32_t next_power_of_2(uint32_t v)
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

static void pq__grow(PQHeader** queue, int requested_capacity)
{
    if ((*queue)->is_static)
    {
        return;
    }
    if ((*queue)->capacity >= requested_capacity)
    {
        return;
    }
    
    int new_capacity = (*queue)->capacity * 2 > 8 ? (*queue)->capacity * 2 : 8;
    if (requested_capacity > (*queue)->capacity)
    {
        requested_capacity = next_power_of_2(requested_capacity);
        new_capacity = requested_capacity;
    }
    
    int allocation_size = pq_alloc_size(new_capacity, (*queue)->element_size);
    void* block = PQ_ALLOC(allocation_size);
    
    PQHeader* hdr = (PQHeader*)block;
    void* data = (uint8_t*)block + pq_header_size();
    void* temp = ((uint8_t*)data + ((*queue)->element_size * new_capacity));
    float* weights = (float*)((uint8_t*)temp + (*queue)->element_size);
    PQ_MEMCPY(data, (*queue)->data, (*queue)->element_size * (*queue)->capacity);
    PQ_MEMCPY(weights, (*queue)->weights, sizeof(float) * (*queue)->capacity);
    
    hdr->data = data;
    hdr->temp = temp;
    hdr->weights = weights;
    hdr->count = (*queue)->count;
    hdr->capacity = new_capacity;
    hdr->element_size = (*queue)->element_size;
    hdr->is_static = false;
    
    PQ_FREE(*queue);
    *queue = hdr;
}

static void pq__add(PQHeader* queue, void* data, float weight)
{
    if (queue->is_static)
    {
        if (queue->count >= queue->capacity)
        {
            return;
        }
    }
    
    int insert_index = queue->count;
    for (int index = 0; index < queue->count; ++index)
    {
        if (queue->is_ascending)
        {
            if (queue->weights[index] > weight)
            {
                insert_index = index;
                break;
            }
        }
        else
        {
            if (queue->weights[index] < weight)
            {
                insert_index = index;
                break;
            }
        }
    }
    
    int move_count = queue->count - insert_index;
    {
        void* src = (uint8_t*)queue->data + queue->element_size * insert_index;
        void* dst = (uint8_t*)queue->data + queue->element_size * (insert_index + 1);
        PQ_MEMCPY(dst, src, queue->element_size * move_count);
        PQ_MEMCPY(src, data, queue->element_size);
    }
    {
        float* src = queue->weights + insert_index;
        float* dst = queue->weights + (insert_index + 1);
        PQ_MEMCPY(dst, src, sizeof(float) * move_count);
        queue->weights[insert_index] = weight;
    }
    queue->count++;
}

static void pq__remove(PQHeader* queue, int index)
{
    if (!(index >= 0 && index < queue->count))
    {
        return;
    }
    
    int move_count = queue->count - index;
    {
        void* src = (uint8_t*)queue->data + queue->element_size * (index + 1);
        void* dst = (uint8_t*)queue->data + queue->element_size * index;
        PQ_MEMCPY(dst, src, queue->element_size * move_count);
    }
    {
        float* src = queue->weights + (index + 1);
        float* dst = queue->weights + index;
        PQ_MEMCPY(dst, src, sizeof(float) * move_count);
    }
    queue->count--;
}

static void pq__set_ascending(PQHeader* queue, bool is_ascending)
{
    if (queue->is_ascending != is_ascending)
    {
        for (int index = 0; index < queue->count / 2; ++index)
        {
            int lo = index;
            int hi = queue->count - index - 1;
            void* lo_d = (uint8_t*)queue->data + queue->element_size * lo;
            void* hi_d = (uint8_t*)queue->data + queue->element_size * hi;
            PQ_MEMCPY(queue->temp, lo_d, queue->element_size);
            PQ_MEMCPY(lo_d, hi_d, queue->element_size);
            PQ_MEMCPY(hi_d, queue->temp, queue->element_size);
            
            float temp = queue->weights[lo];
            queue->weights[lo] = queue->weights[hi];
            queue->weights[hi] = temp;
        }
    }
    queue->is_ascending = is_ascending;
}

static int pq__index_of(PQHeader* queue, void* item)
{
    int found_index = -1;
    for (int index = 0; index < queue->count; ++index)
    {
        void* data = (uint8_t*)queue->data + queue->element_size * index;
        
        if (PQ_MEMCMP(data, item, queue->element_size) == 0)
        {
            found_index = index;
            break;
        }
    }
    return found_index;
}

static void pq__tests()
{
    typedef struct Test
    {
        struct
        {
            float x;
            float y;
        } position;
        int handle;
    } Test;
    
    // dynamic_memory_test
    {
        Test* test = NULL;
        
        Test t0 = 
        {
            0.0f,
            0.0f,
            0
        };
        
        pq_add(test, t0, 1.0f);
        PQ_ASSERT((pq_header(test))->count == 1);
        PQ_ASSERT((pq_header(test))->count == pq_count(test));
        
        Test t1 = 
        {
            1.0f,
            1.0f,
            0
        };
        pq_add(test, t1, 2.0f);
        PQ_ASSERT((pq_header(test))->count == 2);
        PQ_ASSERT((pq_header(test))->count == pq_count(test));
        
        PQ_ASSERT(test[0].position.x == 0 && test[0].position.y == 0);
        PQ_ASSERT(test[1].position.x == 1 && test[1].position.y == 1);
        pq_fit(test, 10);
        PQ_ASSERT((pq_header(test))->capacity == 16);
        PQ_ASSERT( (pq_header(test))->capacity == pq_capacity(test));
        pq_add(test, t1, 0.5f);
        PQ_ASSERT(test[0].position.x == 1 && test[0].position.y == 1);
        PQ_ASSERT((pq_header(test))->count == 3);
        
        pq_clear(test);
        PQ_ASSERT((pq_header(test))->count == 0);
        
        void* old_ptr = test;
        for (int index = 0; index < 32; ++index)
        {
            Test t = 
            {
                (float)index,
                (float)index * 2,
                index,
            };
            
            pq_add(test, t, (float)(31 - index));
        }
        PQ_ASSERT((pq_header(test))->capacity == 32);
        PQ_ASSERT(old_ptr != test);
        
        float* weights = pq_weights(test);
        for (int index = 0; index < 32; ++index)
        {
            PQ_ASSERT(test[index].position.x == (float)(31 - index));
            PQ_ASSERT(test[index].position.y == (31 - index) * 2.0f);
            PQ_ASSERT(test[index].handle == 31 - index);
            PQ_ASSERT(weights[index] == (float)index);
        }
        
        pq_free(test);
    }
    
    // preallocated memory
    {
        Test* test = NULL;
        int alloc_size = pq_alloc_size(24, sizeof(Test));
        void* buffer = malloc(alloc_size);
        pq_static(test, buffer, alloc_size);
        PQ_ASSERT(pq_capacity(test) == 24);
        PQ_ASSERT(pq_count(test) == 0);
        
        Test t0 = 
        {
            0.0f,
            0.0f,
            0
        };
        
        pq_add(test, t0, 1.0f);
        PQ_ASSERT((pq_header(test))->count == 1);
        PQ_ASSERT((pq_header(test))->count == pq_count(test));
        
        pq_clear(test);
        PQ_ASSERT((pq_header(test))->count == 0);
        pq_free(test);
        
        free(buffer);
    }
    // minimum size, 0 capacity priority_queue
    {
        int alloc_size = pq_alloc_size(0, sizeof(int));
        void* buffer = malloc(alloc_size);
        int* test = NULL;
        pq_static(test, buffer, alloc_size);
        PQ_ASSERT(pq_count(test) == 0);
        PQ_ASSERT(pq_capacity(test) == 0);
        pq_add(test, 1, 1.0f);
        PQ_ASSERT(pq_count(test) == 0);
        free(buffer);
    }
    // removals
    {
        int* test = NULL;
        pq_add(test, 1, 1.0f);
        pq_add(test, 2, 0.0f);
        PQ_ASSERT(pq_count(test) == 2);
        PQ_ASSERT(test[0] == 2);
        PQ_ASSERT(test[1] == 1);
        pq_remove_at(test, 0);
        PQ_ASSERT(test[0] == 1);
        PQ_ASSERT(pq_count(test) == 1);
        pq_remove_at(test, 0);
        PQ_ASSERT(pq_count(test) == 0);
        pq_free(test);
    }
    // weights
    {
        int* test = NULL;
        pq_add(test, 1, 1.0f);
        pq_add(test, 2, 0.0f);
        PQ_ASSERT(pq_count(test) == 2);
        PQ_ASSERT(test[0] == 2);
        PQ_ASSERT(test[1] == 1);
        PQ_ASSERT(pq_count(test) == 2);
        pq_add_weight_at(test, 0, 2.0f);
        PQ_ASSERT(test[0] == 1);
        PQ_ASSERT(test[1] == 2);
        PQ_ASSERT(pq_count(test) == 2);
        pq_sub_weight_at(test, 1, 3.0f);
        PQ_ASSERT(test[0] == 2);
        PQ_ASSERT(test[1] == 1);
        PQ_ASSERT(pq_count(test) == 2);
        pq_set_weight(test, 2, 20.0f);
        PQ_ASSERT(test[0] == 1);
        PQ_ASSERT(test[1] == 2);
        PQ_ASSERT(pq_weights(test)[1] == 20.0f);
        PQ_ASSERT(pq_count(test) == 2);
        PQ_ASSERT(pq_weight(test, 2) == 20.0f);
        pq_free(test);
    }
    // copy
    {
        int* a = NULL;
        int* b = NULL;
        pq_fit(b, 8);
        
        pq_add(a, 1, 1.0f);
        pq_add(a, 2, 0.0f);
        pq_add(a, 3, 3.0f);
        pq_set(a, b);
        PQ_ASSERT(pq_count(a) == pq_count(b));
        for (int index = 0; index < pq_count(a); ++index)
        {
            PQ_ASSERT(a[index] == b[index]);
            PQ_ASSERT(pq_weights(a)[index] == pq_weights(b)[index]);
        }
        pq_free(a);
        pq_free(b);
    }
}

#endif //PRIORITY_QUEUE_H
