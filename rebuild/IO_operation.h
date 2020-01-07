//
// Created by user on 2020/1/7.
//
#include <cstdint>
#include "table.h"
#ifndef REBUILD_IO_OPERATION_H
#define REBUILD_IO_OPERATION_H
bool ap_Getfromfile(int64_t buffer[], int32_t& ptr, char*& bufin);
bool msort_Getfromfile(int32_t i, int64_t buffer[], int32_t& ptr, FILE* file[]);
bool test_read_end(FILE* file[]);
bool test_if_inemp(const bool* ifin1, const bool* ifin2);
void msort_read(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max], int32_t* inptr2, bool* ifin2);
void msort_write1(int64_t* outbuffer1, int32_t& outptr1, char* bufout);
void msort_write2(int64_t* outbuffer2, int32_t& outptr2, char* bufout);
#endif //REBUILD_IO_OPERATION_H
