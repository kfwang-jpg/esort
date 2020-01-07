//
// Created by user on 2020/1/7.
//

#ifndef REBUILD_TABLE_H
#define REBUILD_TABLE_H
#ifndef IO_OPERATION_H
#define IO_OPERATION_H
#define _CRT_SECURE_NO_WARNINGS
#include <memory.h>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <cmath>
#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;
/*const int32_t ap_k = 10;//1200000;    //划分归并段部分k路归并
const int32_t ap_outbuffer_max = 5;//225280;   //划分归并段部分输出缓冲区大小
const int32_t ap_inbuffer_max = 106;//5280000;   //划分归并段部分输入缓冲区大小
const int32_t charin_max = 105;//84480000; //输入字符串开辟的空间
const int32_t charout_max = 105;//341939200; //输出字符串开辟的空间
const int32_t msort_outbuffer_max = 3;//17996800;   //多路归并部分输出缓冲区大小
const int32_t msort_inbuffer_max = 3;//22528;   //多路归并部分输入缓冲区大小*/
//const int32_t kk = 256;
//const int32_t ap_k = 1200000;    //划分归并段部分k路归并
const int64_t ap_outbuffer_max = 4000000;   //划分归并段部分输出缓冲区大小
//const int32_t ap_inbuffer_max = 11000005;   //划分归并段部分输入缓冲区大小
//const int32_t charin_max = 11000000; //输入字符串开辟的空间
const int64_t charout_max = 5400000; //输出字符串开辟的空间
const int64_t msort_outbuffer_max = 300000;   //多路归并部分输出缓冲区大小
const int64_t msort_inbuffer_max = 296960;   //多路归并部分输入缓冲区大小
const int64_t themax = 0x004fffffffffffff;   //最大值
const int64_t themin = -0x004fffffffffffff;   //最小值
//用于给负空指数位置f
const int64_t setz[4] = { 0, 0x000f000000000000, 0x000ff00000000000, 0x000fff0000000000 };
//保证只对该指数位取反
const int64_t setx[3] = { 0x000f000000000000, 0x0000f00000000000, 0x00000f0000000000 };
const int64_t flagbyte1 = 0x20000000000000;  //指数为正符号
const int64_t flagbyte2 = 0x10000000000000;  //指数为负符号
const int64_t decimal_table[10][10] = {
        0, 0x1000000000, 0x2000000000, 0x3000000000, 0x4000000000, 0x5000000000, 0x6000000000, 0x7000000000, 0x8000000000, 0x9000000000,
        0, 0x100000000, 0x200000000, 0x300000000, 0x400000000, 0x500000000, 0x600000000, 0x700000000, 0x800000000, 0x900000000,
        0, 0x10000000, 0x20000000, 0x30000000, 0x40000000, 0x50000000, 0x60000000, 0x70000000, 0x80000000, 0x90000000,
        0, 0x1000000, 0x2000000, 0x3000000, 0x4000000, 0x5000000, 0x6000000, 0x7000000, 0x8000000, 0x9000000,
        0, 0x100000, 0x200000, 0x300000, 0x400000, 0x500000, 0x600000, 0x700000, 0x800000, 0x900000,
        0, 0x10000, 0x20000, 0x30000, 0x40000, 0x50000, 0x60000, 0x70000, 0x80000, 0x90000,
        0, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, 0x8000, 0x9000,
        0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0x900,
        0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90,
        0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9 };
const int64_t index_table[3][10] = {
        0, 0x1000000000000, 0x2000000000000, 0x3000000000000, 0x4000000000000, 0x5000000000000, 0x6000000000000, 0x7000000000000, 0x8000000000000, 0x9000000000000,
        0, 0x100000000000, 0x200000000000, 0x300000000000, 0x400000000000, 0x500000000000, 0x600000000000, 0x700000000000, 0x800000000000, 0x900000000000,
        0, 0x10000000000, 0x20000000000, 0x30000000000, 0x40000000000, 0x50000000000, 0x60000000000, 0x70000000000, 0x80000000000, 0x90000000000,
};
const int64_t pave[11] = { static_cast<int64_t>(0xffffffffffffffff), static_cast<int64_t>(0xfffffffffffffff0), static_cast<int64_t>(0xffffffffffffff00), static_cast<int64_t>(0xfffffffffffff000), static_cast<int64_t>(0xffffffffffff0000), static_cast<int64_t>(0xfffffffffff00000),
                           static_cast<int64_t>(0xffffffffff000000), static_cast<int64_t>(0xfffffffff0000000), static_cast<int64_t>(0xffffffff00000000), static_cast<int64_t>(0xfffffff000000000), static_cast<int64_t>(0xffffff1000000000) };
extern mutex apread, msortread, cmsortread, cmsortwrite1, cmsortwrite2, msortwrite, cmsortloser1, illegal,
        cmsortloser2, cmsortloser3;
extern condition_variable msort_read_condition, msort_write_conditon1, msort_write_conditon2, msort_loser_condition1, msort_loser_condition2;
extern int32_t cnt[10], cntall, illegal_count, legal_count, ap_k , msort_k;
extern int32_t cntadd[11], numthread;  //文件数量之和记录，用于生成文件名
extern int64_t charin_max, ap_inbuffer_max;  //输入字符串开辟的空间 划分归并段部分输入缓冲区大小
extern FILE* endout; //最终输出文件
extern FILE* inputfile; //输入文件
extern bool ifout1, ifout2, ifend; //输出缓冲区1、2、是否写完标志，
extern string path, suffix;    //输出临时文件的路径及后缀
#endif
#endif //REBUILD_TABLE_H
