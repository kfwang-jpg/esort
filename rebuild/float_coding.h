//
// Created by user on 2020/1/7.
//
#include <cstdint>
#include "table.h"
#ifndef REBUILD_FLOAT_CODING_H
#define REBUILD_FLOAT_CODING_H
void Illegal_pro(char*& p, const char* bufin);
int64_t ap_change(int64_t len, int64_t buffer[], int32_t& ptr, char*& bufin);
void decode(int64_t buffer[], int32_t ptr, char*& bufout);
#endif //REBUILD_FLOAT_CODING_H
