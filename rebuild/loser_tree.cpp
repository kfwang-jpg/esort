//
// Created by user on 2020/1/7.
//
//包括生成初始归并段部分和归并排序部分的败者树算法，及相应的多线程实现
#include "loser_tree.h"
#include "table.h"
#include "IO_operation.h"
string get_name(int32_t cc)
{
    /*根据文件次序得出它的命名部分
     * 使用全局变量：cntadd[]*/
    string name = "_";
    int32_t i = 0;
    while (cc < cntadd[i] || cc >= cntadd[i + 1])
        i++;
    char temp[5] = { 0 };
    sprintf(temp, "%d", cc - cntadd[i]);
    name = char(i + '0') + name + temp;
    return name;
}
bool test_if_infull(bool* ifin1, bool* ifin2)
{
    /*判断是否输入缓冲区都满（1/2至少有一个满)
     * 使用全局变量：cntall*/
    for (int32_t i = 0; i < cntall; i++)
    {
        if (!ifin1[i] && !ifin2[i])
            return false;
    }
    return true;
}

void SelectMin(int64_t ap_value[], int32_t rn[], int32_t ap_loser[], int32_t q, int32_t& rq)
{
    /* 生成初始归并段部分
     * 从q开始向上到根结点调整败者树段号存入rq*/
    int32_t temp = 0;
    for (int32_t t = (q + ap_k) >> 1; t > 0; t = t >> 1)
    {
        if (rn[ap_loser[t]] < rn[q] || (rn[ap_loser[t]] == rn[q] && ap_value[ap_loser[t]] < ap_value[q]))
        {//胜者：段号小／段号相同但值小
            temp = ap_loser[t];
            ap_loser[t] = q;
            q = temp;   //q一直储存的是胜者
            rq = rn[q]; //rq存储胜者段号
        }
    }
    ap_loser[0] = q;
}

void Creat_section(char pre) {
    /*生成初始归并段，返回归并段的数量
     使用全局变量：cntall
     调用的使用全局变量的函数：ap_Getfromfile*/
    auto* ap_inbuffer = new int64_t[ap_inbuffer_max]; //输入缓冲区
    int64_t* ap_outbuffer = new int64_t[ap_outbuffer_max];//输出缓冲区
    int64_t* ap_value = new int64_t[ap_k + 1];    //败者树叶结点的值
    int32_t input1 = 0, input2 = 0, p1 = 0;    //输入缓冲区的首尾指针/输出缓冲区尾指针
    int32_t* rn = new int32_t[ap_k + 1];   //叶结点(r[i])所属归并段号
    int32_t* ap_loser = new int32_t[ap_k];  //非叶结点（ap_loser[0]存最终胜者序号，其余存败者序号)
    char* bufin = new char[charin_max];
    int32_t q = 0, rq = 0, rc = -1, rmax = 0, p = 0;
    int64_t Lastap_value;
    memset(ap_loser, 0, sizeof(int) * ap_k);
    memset(rn, 0, sizeof(int) * (ap_k + 1));
    memset(bufin, 0, charin_max);
    if (inputfile && ap_Getfromfile(ap_inbuffer, input2, bufin))
    {
        ap_value[ap_k] = themin;   //一定会胜
        for (int32_t i = 0; i < ap_k; i++)
            ap_loser[i] = ap_k;
        for (int32_t i = ap_k - 1; i >= 0; i--)
        {//构造败者树
            if (input1 < input2)
            {
                ap_value[i] = ap_inbuffer[input1++];
                rn[i] = 0;
            }
            else if (inputfile && ap_Getfromfile(ap_inbuffer, input2, bufin))
            {
                //输入缓冲区为空，尝试读入文件
                ap_value[i] = ap_inbuffer[0];
                input1 = 1;
                rn[i] = 0;
            }
            else
                rn[i] = 1;
            SelectMin(ap_value, rn, ap_loser, i, rq);
        }
        //当前胜者序号/上一个被选出的序号 lastkey的归并段段号 当前归并段段号 下一次将要产生的归并段号
        rc = 0;
        q = ap_loser[0]; //胜者序号
        Lastap_value = themax;  //门槛，最近选出的记录的值（大于他的进本归并段，小于的进下一归并段）
        FILE* ofile = fopen((path + pre + '_' + '0' + suffix).c_str(), "wb");  //第一个临时输出文件
        while (1)
        {
            if (rq != rc)    //选出的最小记录段号不是当前归并段号，说明是下一段
            {
                if (cntall++ % 10 == 0)
                    printf("The #%d temperary file has been generated!\n", cntall);
                fwrite(ap_outbuffer, 8, p1, ofile);    //这段结束写出输出缓冲区中残留的内容
                fclose(ofile);
                p1 = 0;    //清空缓冲区
                if (rq > rmax)
                    break;
                else
                {
                    char temp[10] = { 0 };		//打开下一个临时写入文件
                    sprintf(temp, "%d", rq);
                    ofile = fopen((path + pre + '_' + temp + suffix).c_str(), "wb");
                    rc = rq;
                }
            }
            //将元素q输出到缓冲区
            if (p1 < ap_outbuffer_max)
                ap_outbuffer[p1++] = ap_value[q];
            else
            {
                fwrite(ap_outbuffer, 8, p1, ofile);
                p1 = 1;
                ap_outbuffer[0] = ap_value[q];
            }
            Lastap_value = ap_value[q];   //记忆新的门槛
            //在q位置读入新的元素
            if (input1 < input2)
            {
                ap_value[q] = ap_inbuffer[input1++];
                rn[q] = ap_value[q] < Lastap_value ? rmax = rq + 1 : rc;
            }
            else if (inputfile && ap_Getfromfile(ap_inbuffer, input2, bufin))
            {
                ap_value[q] = ap_inbuffer[0];
                input1 = 1;
                rn[q] = ap_value[q] < Lastap_value ? rmax = rq + 1 : rc;
            }
            else
                rn[q] = rmax + 1;
            rq = rn[q];
            SelectMin(ap_value, rn, ap_loser, q, rq);  //从新进来的结点q开始对树进行调整
            q = ap_loser[0];   //下一个被选结点（最终胜者）
        }
    }
    cnt[pre - '0'] = rc + 1;
    delete[]ap_value;
    delete[]rn;
    delete[]ap_loser;
    delete[]ap_inbuffer;
    delete[]ap_outbuffer;
    delete[]bufin;
}

void generate(char inputname[], int32_t threadnum, int32_t charin, int32_t kk)
{
    /*生成初始归并段部分 多线程
     * 使用全局变量：cntall illegal_count legal_count
     * 调用使用全局变量的函数Creat_section*/
    system("md temp");   //创建存放临时文件的文件夹
    inputfile = fopen(inputname, "rb");
    if (!inputfile)
    {
        printf("Failed to open : %s\n", inputname);
        return;
    }
    printf("Producing merge stage!\n");
    numthread = threadnum;
    ap_k = kk;
    charin_max = charin;
    ap_inbuffer_max = charin;
    char pre[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    thread Cthread[10];
    for (int32_t i = 0; i < threadnum; i++)    //创建线程
        Cthread[i] = thread(Creat_section, pre[i]);
    for (int32_t i = 0; i < threadnum; i++)
        Cthread[i].join();
    printf("The division is over.\nThere are %d illegal inputs and %d legal inputs.\n", illegal_count, legal_count);
    printf("Generated %d partition files.\n", cntall);
}

void ajust(int64_t value[], int32_t loser[], int32_t q)
{
    /*自叶结点q到败者树根结点进行调整*/
    int32_t temp = 0;
    for (int32_t t = (msort_k + q) >> 1; t > 0; t = t >> 1) //t初始化为q的父结点
    {
        if (value[loser[t]] < value[q])
        {
            //败者计入loser[t], 胜者计入q
            temp = q;
            q = loser[t];
            loser[t] = temp;
        }
    }
    loser[0] = q;   //在最后一步将胜者写入根结点
    //只能在最后一步，否则初始化的值一定会赢
}

int mergesort(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max],
              int32_t* inptr2, bool* ifin2, int64_t* outbuffer1, int32_t& outptr1, int64_t* outbuffer2, int32_t& outptr2)
{
    /*多路归并部分
     *实现了生产者消费者模型的多线程
    参数设置：输入文件数组，输入缓冲区1，指针，状态标志；输入缓冲区2，指针，状态标志；输出缓冲区1，指针；输出缓冲区2，指针
     全局变量：cmsortloser1 msort_loser_condition1 msort_write_conditon2 msort_write_conditon1 cmsortloser2 msort_loser_condition2
     msortread msort_read_condition cmsortloser3 msortwrite
     调用用全局变量的函数 test_if_infull*/
    int64_t** inbuffer = new int64_t * [msort_k];     //当前输入缓冲区
    int64_t* outbuffer;        //当前输出缓冲区
    bool nowout = true;    //判断当前输出缓冲区是否为1
    bool* nowin = new bool[msort_k + 1]; //判断当前输入缓冲区是否为1
    int32_t outptr = 0;	   //输出缓冲区尾指针
    int32_t* inptrt = new int[msort_k]; //当前输入缓冲区尾指针
    int32_t* inptrh = new int[msort_k]; //当前输入缓冲区头指针
    memset(inptrh, 0, msort_k * 4);
    memset(inptrt, 0, msort_k * 4);
    memset(nowin, 1, msort_k + 1);
    int64_t* msort_value = new int64_t[msort_k + 1]; //r[i]的排序码（用作比较的值）
    int32_t* msort_loser = new int32_t[msort_k]; //非叶子结点，loser[0]存最终胜者，其他存败者
    char* bufout = new char[charout_max]; //输出字符串
    int32_t cc = 0, count = 0, count2 = 0; //已处理归并段个数
    msort_value[msort_k] = themin;  //初始化为最小值（用于初始化败者树）
    //当输入缓冲区1/2至少有一个满时结束挂起状态
    unique_lock<mutex> mlock(cmsortloser1);
    msort_loser_condition1.wait(mlock, [ifin1, ifin2] {return test_if_infull(ifin1, ifin2);});
    //初始化输入缓冲区
    for (int i = 0; i < cntall; i++)
    {
        if (ifin1[i])
        {
            inbuffer[i] = inbuffer1[i];
            inptrt[i] = inptr1[i];
        }
        else
        {
            inbuffer[i] = inbuffer2[i];
            inptrt[i] = inptr2[i];
            nowin[i] = 0;
        }
    }
    outbuffer = outbuffer1;  //当前输出缓冲区设置为输出缓冲区1
    for (int i = 0; i < msort_k; i++)   //初始化败者树
    {
        msort_loser[i] = msort_k; //败者树所有结点都赋值为kk(对应value[kk],一定会胜
        if (inptrt[i])  //若有数据读入
        {
            msort_value[i] = inbuffer[i][0];
            inptrh[i]++;
        }
        else
            msort_value[i] = themax;
    }
    for (int i = msort_k - 1; i >= 0; i--)
        ajust(msort_value, msort_loser, i);  //从key[k-1]到key[0]调整形成败者树
    int32_t q = msort_loser[0]; //胜者
    while (msort_value[q] != themax)
    {   //当max升到loser[0]时，归并完毕
        if (outptr < msort_outbuffer_max)
            outbuffer[outptr++] = msort_value[q];   //写入输出缓冲区
        else
        {//如果输出缓冲区已满,唤醒写线程
            if (nowout)
            {
                while (ifout2)
                    msort_write_conditon2.notify_one();  //唤醒输出缓冲区2写线程
                {
                    lock_guard<mutex> guard(msortwrite);
                    ifout1 = true;
                    outptr1 = outptr;
                }
                msort_write_conditon1.notify_one();  //唤醒输出缓冲区1写线程
            }
            else
            {
                while (ifout1)
                    msort_write_conditon1.notify_one();  //唤醒输出缓冲区1写线程
                {
                    lock_guard<mutex> guard(msortwrite);
                    ifout2 = true;
                    outptr2 = outptr;
                }
                msort_write_conditon2.notify_one();  //唤醒输出缓冲区2写线程
            }
            //当输出缓冲区有空时才能进入下一阶段
            unique_lock<mutex> mlock(cmsortloser2);
            msort_loser_condition2.wait(mlock, [] {return !ifout1 || !ifout2;});
            if (!ifout1)  //输出缓冲区1为空
            {
                outbuffer = outbuffer1;
                nowout = true;
            }
            else  //输出缓冲区2为空
            {
                outbuffer = outbuffer2;
                nowout = false;
            }
            outptr = 1;  //清空缓冲区
            outbuffer[0] = msort_value[q];
        }
        //从第q个归并段再读入下一个记录存入value[q]
        if (inptrh[q] < inptrt[q])
            msort_value[q] = inbuffer[q][inptrh[q]++];
        else   //当前第q输入缓冲区已空
        {
            {
                lock_guard<mutex> guard(msortread);
                if (nowin[q])
                    ifin1[q] = false;  //将输入缓冲区标记为空
                else
                    ifin2[q] = false;
                msort_read_condition.notify_one(); //唤醒读线程
            }
            //只有q输入缓冲区满或q文件读完才进入下一阶段
            unique_lock<mutex> mlock(cmsortloser3);
            msort_loser_condition1.wait(mlock, [q, ifin1, ifin2, file] {return ifin1[q] || ifin2[q] || !file[q];});
            if (ifin1[q])  //q输入缓冲区1满
            {
                nowin[q] = true;
                inbuffer[q] = inbuffer1[q];
                inptrt[q] = inptr1[q];
                msort_value[q] = inbuffer[q][0];
                inptrh[q] = 1;
            }
            else if (ifin2[q]) //q输入缓冲区2满
            {
                nowin[q] = false;
                inbuffer[q] = inbuffer2[q];
                inptrt[q] = inptr2[q];
                msort_value[q] = inbuffer[q][0];
                inptrh[q] = 1;
            }
            else
                msort_value[q] = themax;
        }
        ajust(msort_value, msort_loser, q); //从q开始调整
        q = msort_loser[0];   //取当前最小记录所在归并段号
    }
    //将输出缓冲区中的残留内容全部写出
    {
        lock_guard<mutex> guard(msortwrite);
        ifend = true;
    }
    if (nowout)
    {
        while (ifout2)
            msort_write_conditon2.notify_one();  //唤醒缓冲区2写线程
        {
            lock_guard<mutex> guard(msortwrite);
            ifout1 = true;  //将输出缓冲区1标记为已满
            outptr1 = outptr;
        }
        msort_write_conditon1.notify_one();  //唤醒缓冲区1写线程
        msort_write_conditon2.notify_one();  //唤醒缓冲区1写线程
    }
    else
    {
        while (ifout1)
            msort_write_conditon1.notify_one();  //唤醒缓冲区1写线程
        {
            lock_guard<mutex> guard(msortwrite);
            ifout2 = true;  //将输出缓冲区2标记为已满
            outptr2 = outptr;
        }
        msort_write_conditon2.notify_one();  //唤醒缓冲区2写线程
        msort_write_conditon1.notify_one();  //唤醒缓冲区1写线程
    }
    return 0;
}

void sort(char outputname[], int32_t kk)
{
    /* 多路归并
     * 开启读入、写出和归并线程
     * 使用全局变量：endout msort_k cntall numthread
     * 调用使用全局变量的函数：msort_read mergesort msort_write1 msort_write2*/
    printf("Merge sorting!\n");
    msort_k = kk;
    FILE* (*file) = new FILE * [msort_k]; //为每个归并段生成输入文件流
    bool* ifin1 = new bool[msort_k + 1];  //每个输入缓冲区1的标志
    bool* ifin2 = new bool[msort_k + 1];  //每个输入缓冲区2的标志
    if (!(endout = fopen(outputname, "wb")))
    {
        printf("Failed to generate output file\n");
        return;
    }
    memset(file, 0, sizeof(FILE*) * kk);
    memset(ifin1, 0, kk + 1);
    memset(ifin2, 0, kk + 1);
    for (int32_t i = 1; i < numthread + 1; i++)
        cntadd[i] = cntadd[i - 1] + cnt[i - 1];
    if (!cntall)
    {
        printf("No number has been sorted.\nPlease check the input file!\n");
        return;
    }
    if (cntall > msort_k)
    {
        printf("There are too many temporary files.\nPlease increase the value of \"ap_k\"!\n");
        return;
    }
    int64_t(*inbuffer1)[msort_inbuffer_max] = new int64_t[cntall][msort_inbuffer_max];  //每个文件开设输入缓冲区1
    int64_t(*inbuffer2)[msort_inbuffer_max] = new int64_t[cntall][msort_inbuffer_max];  //每个文件开设输入缓冲区2
    int64_t* outbuffer1 = new int64_t[msort_outbuffer_max];   //每个文件开设输出缓冲区1
    int64_t* outbuffer2 = new int64_t[msort_outbuffer_max];   //每个文件开设输出缓冲区2
    char* bufout = new char[charout_max];      //输出容器
    int32_t* inptr1 = new int32_t[cntall];  //输入缓冲区1尾指针
    int32_t* inptr2 = new int32_t[cntall];  //输入缓冲区2尾指针
    int32_t outptr1 = 0, outptr2 = 0;  //输出缓冲区12尾指针
    for (int32_t i = 0; i < cntall; i++)
        file[i] = fopen((path + get_name(i) + suffix).c_str(), "rb");
    thread read{ msort_read, file, inbuffer1, inptr1, ifin1, inbuffer2, inptr2, ifin2 };
    thread msort{ mergesort, file, inbuffer1, inptr1, ifin1, inbuffer2, inptr2, ifin2, outbuffer1, ref(outptr1), outbuffer2, ref(outptr2) };
    thread write1{ msort_write1, outbuffer1, ref(outptr1), bufout };
    thread write2{ msort_write2, outbuffer2, ref(outptr2), bufout };
    read.join();
    msort.join();
    write1.join();
    write2.join();
    fclose(endout);
    delete[] * file;
    delete[] ifin2;
    delete[] ifin1;
    delete[] inbuffer1;
    delete[] inbuffer2;
    delete[] outbuffer1;
    delete[] outbuffer2;
    delete[] inptr1;
    delete[] bufout;
    delete[] inptr2;
}
