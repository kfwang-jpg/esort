//
// Created by user on 2020/1/7.
//
#include "table.h"
mutex apread, msortread, cmsortread, cmsortwrite1, cmsortwrite2, msortwrite, cmsortloser1, illegal,
        cmsortloser2, cmsortloser3;
condition_variable msort_read_condition, msort_write_conditon1, msort_write_conditon2, msort_loser_condition1, msort_loser_condition2;
int32_t cnt[10] = { 0 }, cntall = 0, illegal_count = 0, legal_count = 0, ap_k = 0, msort_k = 0;
int32_t cntadd[11] = { 0 }, numthread = 0;  //文件数量之和记录，用于生成文件名
int64_t charin_max = 0, ap_inbuffer_max = 0;  //输入字符串开辟的空间 划分归并段部分输入缓冲区大小
FILE* endout = nullptr; //最终输出文件
FILE* inputfile = nullptr; //输入文件
bool ifout1 = false, ifout2 = false, ifend = false; //输出缓冲区1、2、是否写完标志，
string path = "temp\\", suffix = ".DATA";    //输出临时文件的路径及后缀 //NOLINT