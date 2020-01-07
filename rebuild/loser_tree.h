//
// Created by user on 2020/1/7.
//

#ifndef REBUILD_LOSER_TREE_H
#define REBUILD_LOSER_TREE_H
#include "table.h"
string get_name(int32_t cc);
bool test_if_infull(bool* ifin1, bool* ifin2);
void SelectMin(int64_t ap_value[], int32_t rn[], int32_t ap_loser[], int32_t q, int32_t& rq);
void Creat_section(char pre);
void generate(char inputname[], int32_t threadnum, int32_t charin, int32_t kk);
void ajust(int64_t value[], int32_t loser[], int32_t q);
int mergesort(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max],
              int32_t* inptr2, bool* ifin2, int64_t* outbuffer1, int32_t& outptr1, int64_t* outbuffer2, int32_t& outptr2);
void sort(char outputname[], int32_t kk);
#endif //REBUILD_LOSER_TREE_H
