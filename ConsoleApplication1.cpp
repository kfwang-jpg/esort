//
//  main.cpp
//  外排
//
//  Created by kaifei wang on 2019/9/1.
//  Copyright © 2019年 kaifei wang. All rights reserved.
#include "IO_operation.h"
#include <cstdlib>
#include <time.h>
using namespace std;

int main() {
	FILE* param;
	if (!(param = fopen("Sort.param", "rb")))
	{
		cout << "fail to open the param file\n";
		return 0;
	}
	char temp[101] = { 0 }, inputname[51] = { 0 }, outputname[51] = { 0 };
	int32_t threadnum = 0, ap_kk = 0, msort_kk = 0, charin_max = 0;  //线程数；败者树大小；输入缓冲区大小
	fgets(temp, 100, param);  //读取输入文件路径
	memcpy(inputname, temp + 11, 50);
	inputname[strlen(inputname) - 2] = 0;
	memset(temp, 0, 100);  //读取输出文件路径
	fgets(temp, 100, param);
	memcpy(outputname, temp + 12, 50);
	outputname[strlen(outputname) - 2] = 0;
	memset(temp, 0, 100);    //读取线程数
	fgets(temp, 100, param);
	threadnum = temp[11] - '0';
	printf("The number of thread in divided part is %d\n", threadnum);
	memset(temp, 0, 100);   //读取输入缓冲区大小
	fgets(temp, 100, param);
	for (int i = 11; temp[i] != ';'; i++)
		charin_max = charin_max * 10 + temp[i] - '0';
	memset(temp, 0, 100);   //读取划分归并段部分败者树大小
	fgets(temp, 100, param);
	for (int i = 5; temp[i] != ';'; i++)
		ap_kk = ap_kk * 10 + temp[i] - '0';
	memset(temp, 0, 100);   //读取多路归并部分败者树大小
	fgets(temp, 100, param);
	for (int i = 8; temp[i] != ';'; i++)
		msort_kk = msort_kk * 10 + temp[i] - '0';
	clock_t start1, end1, start2, end2;
	fclose(param);
	start1 = clock();
	generate(inputname, threadnum, charin_max, ap_kk);  //生成初始归并段
	end1 = clock();
	printf("It consumed %d seconds\n", (end1 - start1) / 1000);
	sort(outputname, msort_kk);   //多路归并
	end2 = clock();
	printf("It consumed %d seconds\n", (end2 - end1) / 1000);
	printf("Finish! All operations taken %d seconds\n", (end2 - start1) / 1000);
	return 0;
}
