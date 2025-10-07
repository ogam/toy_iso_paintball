#include "common/string.h"

fixed char* make_scratch_string(s32 length)
{
    fixed char* buf = NULL;
    s32 size = length + sizeof(CF_Ahdr);
    cf_string_static(buf, scratch_alloc(size), size);
    return buf;
}

char string_get_index(fixed char* str, s32 i)
{
    const char* s = str;
    s32 codepoint;
    while (*s && i--)
    {
        s = cf_decode_UTF8(s, &codepoint);
    }
    return *s;
}

//  CF ime sample
//  @credits:  bullno1
static inline s32 utf8_bytes_required(s32 codepos32) 
{
	if (codepos32 <= 0x7F) 
    {
		return 1;
	} 
    else if (codepos32 <= 0x7FF) 
    {
		return 2;
	} 
    else if (codepos32 <= 0xFFFF) 
    {
		return 3;
	} 
    else if (codepos32 <= 0x10FFFF) 
    {
		return 4;
	} 
    else 
    {
		return 0;
	}
}

f32 string_get_width(fixed char* str, s32 length, s32 index) 
{ 
    const char* s = str;
    s32 glyph_index = 0;
    
    s32 codepoint;
    
    while (*s)
    {
        if (glyph_index == index)
        {
            break;
        }
        s = cf_decode_UTF8(s, &codepoint);
        glyph_index++;
    }
    
    //  @todo:  fix space advance not being accounted for
    return cf_text_width(s, length); 
}

s32 string_delete_range(fixed char* str, s32 index, s32 count)
{
    s32 copy_count = cf_string_len(str) - index - count + 1;
    CF_MEMCPY(str + index, str + index + count, copy_count);
    cf_array_len(str) = cf_max(cf_array_len(str) - count, 0);
    str[cf_array_len(str)] = '\0';
    return cf_string_count(str);
}

s32 string_insert_range(fixed char* str, s32 index, s32* c, s32 count)
{
    fixed char* buf = make_scratch_string(sizeof(s32) * count);
    const char* s = str;
    for (s32 index = 0; index < count; ++index)
    {
        cf_string_append_UTF8(buf, c[index]);
    }
    s32 codepoint;
    while (*s && index--)
    {
        s = cf_decode_UTF8(s, &codepoint);
    }
    s32 byte_index = (s32)(s - str);
    
    s32 copy_count = cf_array_count(str) - byte_index;
    CF_MEMCPY(str + byte_index + count, s, copy_count);
    CF_MEMCPY((char*)s, buf, count);
    cf_array_len(str) += count;
    str[cf_array_len(str)] = '\0';
    return cf_string_count(str);
}

char* string_slice(fixed char* str, s32 start, s32 end)
{
    s32 length = end - start + 1;
    char* buf = scratch_alloc(length);
    CF_MEMCPY(buf, str + start, length);
    buf[length] = '\0';
    
    return buf;
}

const char* string_clone(const char* str)
{
    s32 length = (s32)CF_STRLEN(str) + 1;
    char* clone = scratch_alloc(length);
    CF_MEMCPY(clone, str, length);
    clone[length] = '\0';
    return clone;
}

void string_set(fixed char* a, const char* b)
{
    if (!b)
    {
        return;
    }
    if (CF_STRLEN(b) == 0)
    {
        cf_string_clear(a);
    }
    else
    {
        cf_string_set(a, b);
    }
}