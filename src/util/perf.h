#ifndef PERF_H
#define PERF_H

//  @todo:  add in frames, ring buffer

void perf_begin(const char* tag);
double perf_end(const char* tag);

#endif //PERF_H
