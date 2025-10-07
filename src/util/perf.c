#include "util/perf.h"

void perf_begin(const char* tag)
{
    tag = cf_sintern(tag);
    if (!cf_hashtable_has(s_app->stopwatches, tag))
    {
        CF_Stopwatch new_stopwatch = {0};
        cf_hashtable_set(s_app->stopwatches, tag, new_stopwatch);
    }
    CF_Stopwatch* stopwatch = cf_hashtable_get_ptr(s_app->stopwatches, tag);
    *stopwatch = cf_make_stopwatch();
}

double perf_end(const char* tag)
{
    tag = cf_sintern(tag);
    CF_Stopwatch* stopwatch = NULL;
    if (cf_hashtable_has(s_app->stopwatches, tag))
    {
        stopwatch = cf_hashtable_get_ptr(s_app->stopwatches, tag);
    }
    if (stopwatch)
    {
        double duration_us = cf_stopwatch_microseconds(*stopwatch);
        cf_hashtable_set(s_app->perfs, tag, duration_us);
        return duration_us;
    }
    return 0;
}