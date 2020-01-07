//
// Created by user on 2020/1/7.
//

#include "float_coding.h"
#include "table.h"
/*主要包括编解码部分*/

void Illegal_pro(char*& p, const char* bufin)
{
    /*处理内容：非法输入
     需用到的全局变量：mutex_illegal  illegal_count*/
    {
        lock_guard<mutex> guard(illegal);
        int32_t length = 0;	//非法输入行的长度
        char temp[100] = { 0 }; //临时存储非法输入行
        while (*p != '\n' && p != bufin)
            p--;
        if (*p == '\n')
            ++p;  //移动到非法行行首
        char* temp2 = p;
        while (*p != '\n')   //将指针移动到本行末尾
        {
            p++;
            length++;
        }
        p++;
        if (length > 100)
            length = 99;
        strncpy(temp, temp2, length);
        printf("illegal input : ");
        puts(temp);
        illegal_count++;
    }
}

int64_t ap_change(int64_t len, int64_t buffer[], int32_t& ptr, char*& bufin)
{
    /*将读入bufin中的字符串 按照编码规则
     存入buffer中，返回数量
     配有非法字符处理
     未用到全局变量
     调用的使用全局变量的函数：Illegal_pro*/
    bool flag = false, flag2 = false, negi = false, negd = false;  //判断是否进入指数部分,是否出现过小数点,判断指数和小数是否为负
    int32_t temp_i = 0, temp_d = 0, move_i = 0; //计数器,分别存储现在写入到了指数和小数的第几位
    int32_t pre_ptr = ptr;
    char* p = bufin;
    if (*bufin == 0)
        return 0;
    if (*p == '-')  //先处理第一个字符
    {
        p++;
        negd = true;
    }
    else if (*p == '+')
        p++;
    else if (*p > 0x39 || *p < 0x30)
        Illegal_pro(p, bufin);
    while (*p && p - bufin < len)
    {
        //可能出现符号的部分只有E后或\n后，已全部被特殊处理
        if (*p >= 0x30 && *p <= 0x39) //数字
        {
            if (flag)   //若当前是指数部分
                buffer[ptr] = negi ? buffer[ptr] | (~index_table[temp_i][(*p++ & 0xf)] & setx[temp_i++]) : buffer[ptr] | index_table[temp_i++][(*p++ & 0xf)];
            else    //若当前是小数部分
            {
                if (!flag2 && *p == 0x30)  //处理无效位
                {
                    move_i++;  //记录指数移动位
                    p++;  //过滤掉第一个0位
                    if (*p == 0x2E)  //若下一字符是小数点
                    {
                        p++; //过滤小数点
                        flag2 = true; //小数点出现标记
                        while (*p == 0x30)  //过滤掉无效0位
                        {
                            p++;
                            move_i++;
                        }
                        if (*p == 'e' || *p == 'E' || *p == '\n')  //若小数位全是0
                        {
                            while (*p != '\n')
                                p++;    //过滤掉指数位
                            buffer[ptr] = int64_t(0);
                            if (ptr++ == ap_inbuffer_max - 1)  //此时输出缓冲区已满，返回处理的字符串个数
                                return p - bufin + 1;
                            p++;   //滤掉换行符
                            move_i = negi = negd = flag = flag2 = temp_i = temp_d = 0;
                            continue;
                        }
                    }
                    else if (*p == 0x65 || *p == 0x45 || *p == '\n')  //若小数位全是0
                    {
                        while (*p != '\n')
                            p++;    //过滤掉指数位
                        buffer[ptr] = int64_t(0);
                        if (ptr++ == ap_inbuffer_max - 1)  //此时输出缓冲区已满，返回处理的字符串个数
                            return p - bufin + 1;
                        p++;   //滤掉换行符
                        move_i = negi = negd = flag = flag2 = temp_i = temp_d = 0;
                        continue;
                    }
                    else
                    {
                        move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                        Illegal_pro(p, bufin);
                    }
                }
                else
                {
                    buffer[ptr] = buffer[ptr] | decimal_table[temp_d++][(*p++ & 0xf)];
                    if (temp_d == 10)  //已经填满最后一位了
                    {
                        if (*p > 0x34 && *p <= 0x39)    //四舍五入进位
                        {
                            int64_t temp = buffer[ptr];
                            int32_t i = 0;
                            while ((temp & 0xf) == 9) //直到找到不是9的位再进位
                            {
                                temp = temp >> 4;
                                i++;
                            }
                            buffer[ptr] = (buffer[ptr] & pave[i]) + (int64_t(1) << (i * 4));
                        }
                        while (*p >= 0x30 && *p <= 0x39)
                            p++;    //过滤掉剩余的小数部分
                    }
                }
            }
            continue;
        }
        if (*p == '+')
        {
            if (*(p - 1) == '\n')
                p++;
            else
            {
                move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                Illegal_pro(p, bufin);
            }
            continue;
        }
        if (*p == '-')
        {
            if (*(p - 1) == '\n')
            {
                p++;
                negd = 1;
            }
            else
            {
                move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                Illegal_pro(p, bufin);
            }
            continue;
        }
        if (*p == '\n')  //下一个数字
        {
            if (move_i) // 需要重新编码指数位
            {
                int32_t index_temp = 0;  //存储指数绝对值
                int64_t temp = buffer[ptr] >> 40;  //指数第一位的位置
                buffer[ptr] = buffer[ptr] & 0x000000ffffffffff; //清空指数位
                if (negi)  //若指数是负的
                {
                    temp = ~temp;  //负指数，取反
                    index_temp = (temp & 0xf) + (((temp >> 4) & 0xf) * 10) + (((temp >> 8) & 0xf) * 100);
                }
                else
                    index_temp = (temp & 0xf) + (((temp >> 4) & 0xf) * 10) + (((temp >> 8) & 0xf) * 100);
                if (negi)
                    index_temp = -index_temp;  //负指数符号
                index_temp -= move_i;
                if (index_temp < 0)
                {
                    negi = true;
                    index_temp = -index_temp;
                }
                else
                    negi = false;
                int ii = 2;
                while (index_temp)
                {
                    buffer[ptr] = negi ? buffer[ptr] | (~index_table[ii][((index_temp % 10) & 0xf)] & setx[ii--]) : buffer[ptr] | index_table[ii--][((index_temp % 10) & 0xf)];
                    index_temp /= 10;
                }
                if (negi && ii >= 0)  //负指数需要把空指数位置1
                    buffer[ptr] = buffer[ptr] | setz[ii + 1];
            }
            buffer[ptr] = negi ? buffer[ptr] | flagbyte2 : buffer[ptr] | flagbyte1;
            if (negd)   //确定小数符号位
                buffer[ptr] = -buffer[ptr];
            if (ptr++ == ap_inbuffer_max - 1)  //此时输出缓冲区已满，返回处理的字符串个数
                return p - bufin + 1;
            p++;   //滤掉换行符
            move_i = negi = negd = flag = flag2 = temp_i = temp_d = 0;
            continue;
        }
        if (*p == 'e' || *p == 'E')	//e/E
        {
            flag = true;   //进入指数部分
            p++;
            if (*p == 0x2D) //指数为负
            {
                negi = true;
                p++;
            }
            else if (*p == 0x2B) //指数为正
                p++;
            else if (*p < 0x30 || *p > 0x39)  //指数后非负号或数字
            {
                move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                Illegal_pro(p, bufin);
                continue;
            }
            char* pp = p;
            while (*pp >= 0x30 && *pp <= 0x39)   //求指数位数
            {
                pp++;
                temp_i++;
            }
            if (temp_i > 3)  //若指数大于3位，非法
            {
                move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                Illegal_pro(p, bufin);
                continue;
            }
            temp_i = 3 - temp_i; //空指数位个数
            if (negi)    //负指数需要把空指数位置1
                buffer[ptr] = buffer[ptr] | setz[temp_i];
            continue;
        }
        if (*p == 0x2E && !flag2)    //若为第一次出现的小数点
        {
            flag2 = true;
            p++;
            if (flag || !*(p - 2) || !*p || *p < 0x30 || *p > 0x39 || *(p - 2) < 0x30 || *(p - 2) > 0x39)
                //若进入指数部分后还有小数点 或小数点前后不是数字
            {
                move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
                Illegal_pro(p, bufin);
            }
            continue;
        }
        //其他非法部分
        move_i = buffer[ptr] = temp_i = temp_d = negi = negd = flag = flag2 = 0;
        Illegal_pro(p, bufin);
    }
    return len + 1;  //若字符串全部读入
}

void decode(int64_t buffer[], int32_t ptr, char*& bufout)
{
    /*未用到全局变量
     * 未调用使用全局变量的函数*/
    memset(bufout, 0, charout_max);
    //将buffer中的内容解码，并写入file中, ptr 为buffer尾指针
    int32_t p = 0;  //bufin的指针
    bool neg = 0;
    short decimal = 10, index = 15; //小数和指数分别在哪一位进行存储
    for (int32_t i = 0; i < ptr; i++)
    {
        if (buffer[i] < 0)   //负数，进行标记
        {
            bufout[p++] = 0x2D; //-
            buffer[i] = -buffer[i];
        }
        if ((buffer[i] & 0x00f0000000000000) == 0x0020000000000000 || !(buffer[i] & 0x00f0000000000000))	//写指数符号位
            bufout[p + 12] = 0x2B; //+
        else
        {
            bufout[p + 12] = 0x2D; //-
            neg = 1;
        }
        bufout[p + 1] = 0x2E; //.
        bufout[p + 11] = 0x45; //E
        while (decimal > 1)  //写入小数点之后的内容
        {
            bufout[p + decimal--] = (buffer[i] & 0xf) | 0x30; //取最后一位存入
            buffer[i] = buffer[i] >> 4; //向右移4位
        }
        bufout[p] = (buffer[i] & 0xf) | 0x30;
        if (neg)
        {
            while (index > 12)	//写入指数
            {
                buffer[i] = buffer[i] >> 4;
                bufout[p + index--] = ~(buffer[i]) & 0xf | 0x30;
            }
        }
        else
        {
            while (index > 12)	//写入指数
            {
                buffer[i] = buffer[i] >> 4;
                bufout[p + index--] = buffer[i] & 0xf | 0x30;
            }
        }
        bufout[p + 16] = '\n';
        //bufout[p + 17] = '\n';
        p += 17;
        neg = false; decimal = 10; index = 15; //准备解码下一个
    }
    fwrite(bufout, 1, p, endout);
}