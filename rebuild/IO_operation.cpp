//
// 只包括直接io部分，都是工具函数
//
#include "IO_operation.h"
#include "table.h"
#include "float_coding.h"

bool ap_Getfromfile(int64_t buffer[], int32_t& ptr, char*& bufin)
{
    /*从文件流file中读入数据到bufin中,并将字符串编码后存入buffer
    直到buffer存满或者文件读完
    参数设置：输入缓冲区、指针、输入容器
    使用全局变量：apread，inputfile ap_inbuffer_max
    */
    ptr = 0;
    memset(buffer, 0, 8 * ap_inbuffer_max);
    memset(bufin, 0, charin_max);
    {
        lock_guard<mutex> guard(apread);
        if (!inputfile)
            return false;
        if (!feof(inputfile))
        {
            while (ptr < ap_inbuffer_max - 1)   //一直读入数据直到输入缓冲区写满
            {
                int64_t pos = _ftelli64(inputfile); //存储读之前的位置
                fread(bufin, 1, charin_max - 100, inputfile);
                char temp[99] = { 0 };
                fgets(temp, 99, inputfile);
                memcpy(bufin + strlen(bufin), temp, strlen(temp));
                int64_t len = strlen(bufin);
                if (len == 0)
                {
                    legal_count += ptr;
                    fclose(inputfile);
                    inputfile = nullptr;
                    break;   //文件读完结束循环
                }
                else
                {
                    int64_t gap = ap_change(len, buffer, ptr, bufin);
                    if (gap != len + 1)  //若读入的字符串没有全部处理完，需要移动文件指针
                    {
                        if (feof(inputfile))
                            clearerr(inputfile);
                        _fseeki64(inputfile, gap + pos, 0);//从pos开始向后移gap
                        legal_count += ap_inbuffer_max;
                        break; //输入缓冲区读满，结束循环
                    }
                }
                memset(bufin, 0, charin_max);
            }
        }
        else  //若文件已经读完
        {
            fclose(inputfile);
            inputfile = nullptr;
        }
    }
    return ptr;
}

bool msort_Getfromfile(int32_t i, int64_t buffer[], int32_t& ptr, FILE* file[])
{
    /* 多路归并部分 从临时文件中读取归并段内容
     * 将file中的浮点数存入buffer中，个数存入ptr
     * 未用到全局变量
     * 未调用用到全局变量的函数*/
    ptr = 0;
    //读取文件整体大小
    int64_t start = _ftelli64(file[i]);
    _fseeki64(file[i], 0, SEEK_END);
    int64_t end = _ftelli64(file[i]);
    _fseeki64(file[i], start, 0);
    start = (end - start) / 8;
    ptr = start > msort_inbuffer_max ? msort_inbuffer_max : start;  //存读入的浮点数个数
    fread(buffer, 8, ptr, file[i]);
    return ptr;
}

bool test_read_end(FILE* file[])
{
    /*判断是否所有的文件都已经读完
     * 使用全局变量：cntall*/
    for (int32_t i = 0; i < cntall; i++)
    {
        if (file[i])
            return false;
    }
    return true;
}

bool test_if_inemp(const bool* ifin1, const bool* ifin2)
{
    /*若缓冲区有空，返回true
     * 使用全局变量：cntall*/
    for (int32_t i = 0; i < cntall; i++)
    {
        if (!ifin1[i] || !ifin2[i])
            return true;
    }
    return false;
}

void msort_read(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max], int32_t* inptr2, bool* ifin2)
{
    /*从临时文件（初始归并段）读入输入缓冲区
    参数设置：输入文件数组；输入缓冲区1，尾指针，状态标记；输入缓冲区2，尾指针，状态标记
     使用全局变量：cmsortread mlock msortread cntall msort_loser_condition1
     调用使用全局变量的函数：test_read_end */
    while (true)
    {
        unique_lock<mutex> mlock(cmsortread);
        msort_read_condition.wait(mlock, [ifin1, ifin2] {return test_if_inemp(ifin1, ifin2);}); //当输入缓冲区有空时，结束挂起状态
        {
            lock_guard<mutex> guard(msortread);  //对输入缓冲区1进行读入
            for (int32_t i = 0; i < cntall; i++)
            {
                if (file[i] && !ifin1[i]) //i输入缓冲区1为空
                {
                    if (msort_Getfromfile(i, inbuffer1[i], inptr1[i], file))
                    {
                        ifin1[i] = true;
                        msort_loser_condition1.notify_one(); //唤醒归并线程
                    }
                    else
                    {
                        fclose(file[i]);
                        file[i] = nullptr;
                        msort_loser_condition1.notify_one(); //唤醒归并线程
                        if (test_read_end(file)) //若所有文件已经全部读完，退出读线程
                            return;
                    }
                }
                if (file[i] && !ifin2[i]) //i输入缓冲区2为空
                {
                    if (msort_Getfromfile(i, inbuffer2[i], inptr2[i], file))
                    {
                        ifin2[i] = true;
                        msort_loser_condition1.notify_one(); //唤醒归并线程
                    }
                    else
                    {
                        fclose(file[i]);
                        file[i] = nullptr;
                        msort_loser_condition1.notify_one(); //唤醒归并线程
                        if (test_read_end(file)) //若所有文件已经全部读完，退出读线程
                            return;
                    }
                }
            }
        }
    }
}

void msort_write1(int64_t* outbuffer1, int32_t& outptr1, char* bufout)
{
    /* 多路归并部分
     * 将输出缓冲区内容解码后写出文件
    参数设置：输出缓冲区1，指针；输出容器；输出缓冲区1标志， 是否全部写完标志
     使用全局变量：cmsortwrite mlock1 ifout1 ifend msortwrite msort_loser_condition2 msort_write_conditon1*/
    while (1)
    {
        //当输出缓冲区1满时结束挂起状态
        {
            unique_lock<mutex> mlock1(cmsortwrite1);
            msort_write_conditon1.wait(mlock1, [] {return ifout1 || ifend;});
            {
                lock_guard<mutex> guard(msortwrite);
                decode(outbuffer1, outptr1, bufout);
                ifout1 = false;
                outptr1 = 0;
                msort_loser_condition2.notify_one();
                if (ifend)
                    return;
            }
        }
    }
}

void msort_write2(int64_t* outbuffer2, int32_t& outptr2, char* bufout)
{
    /* 多路归并部分
     * 将输出缓冲区内容解码后写出文件
    参数设置：输出缓冲区2，指针；输出容器；输出缓冲区2标志， 是否全部写完标志
    使用全局变量：cmsortwrite2 mlock1 ifout2 ifend msortwrite msort_loser_condition2 msort_write_conditon2*/
    while (1)
    {
        //当输出缓冲区1满时结束挂起状态
        {
            unique_lock<mutex> mlock1(cmsortwrite2);
            msort_write_conditon2.wait(mlock1, [] {return ifout2 || ifend;});
            {
                lock_guard<mutex> guard(msortwrite);
                decode(outbuffer2, outptr2, bufout);
                ifout2 = false;
                outptr2 = 0;
                msort_loser_condition2.notify_one();
                if (ifend)
                    return;
            }
        }
    }
}