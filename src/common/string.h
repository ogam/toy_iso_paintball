#ifndef STRING_H
#define STRING_H

char string_get_index(fixed char* str, s32 i);
bool string_is_space(s32 c) { return c == ' ' || c == '\t' || c == '\n'; }
f32 string_get_width(fixed char* str, s32 length, s32 index) ;
s32 string_delete_range(fixed char* str, s32 index, s32 count);
s32 string_insert_range(fixed char* str, s32 index, s32* c, s32 count);
char* string_slice(fixed char* str, s32 start, s32 end);
const char* string_clone(const char* str);
void string_set(fixed char* a, const char* b);

#endif //STRING_H
