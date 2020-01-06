#include "IO_operation.h"
//
//  IO_operation.cpp
//  alldesign
//
//  Created by kaifei wang on 2019/9/3.
//  Copyright © 2019年 kaifei wang. All rights reserved.
/*改进：为了减少内存的浪费，在生成初始归并段时，多次读入，直到输入缓冲区满才进行下一步操作
这样输入文件操作必须加锁，ap_change部分不能进行并行操作*/

using namespace std;

//与condition配对、对ifin1，ifin2改变的锁，对ifout1,ifout2改变的锁
mutex apread, msortread, cmsortread, cmsortwrite1, cmsortwrite2, msortwrite, cmsortloser1, illegal,
cmsortloser2, cmsortloser3;
condition_variable msort_read_condition, msort_write_conditon1, msort_write_conditon2, msort_loser_condition1, msort_loser_condition2;
//最后两个分别是等输入缓冲区为满和等输出缓冲区为空
//生成文件数量,非法输入条目数量,划分归并段部分败者树大小，多路归并部分败者树大小
int32_t cnt[10] = { 0 }, cntall = 0, illegal_count = 0, legal_count = 0, ap_k = 0, msort_k = 0;
int32_t cntadd[11] = { 0 }, numthread = 0;  //文件数量之和记录，用于生成文件名
int64_t charin_max = 0, ap_inbuffer_max = 0;  //输入字符串开辟的空间 划分归并段部分输入缓冲区大小
FILE* endout; //最终输出文件
FILE* inputfile; //输入文件
bool ifout1 = false, ifout2 = false, ifend = false; //输出缓冲区1、2、是否写完标志，
string path = "temp\\", suffix = ".DATA";    //输出临时文件的路径及后缀
void Illegal_pro(char*& p, char* bufin)
{
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
void show(int64_t a)
{
	int res[65];
	for (int i = 1; i < 65; i++)
	{
		res[i] = a & 1;
		a = a >> 1;
	}
	for (int i = 64; i > 0; i--)
	{
		if (i % 4 == 0)
			cout << " ";
		cout << res[i];
	}
	cout << endl;
}
int64_t ap_change(int64_t len, int64_t buffer[], int32_t& ptr, char*& bufin)
{
	/*将读入bufin中的字符串 按照编码规则
	 存入buffer中，返回数量
	 配有非法字符处理*/
	bool flag = 0, flag2 = 0, negi = 0, negd = 0;  //判断是否进入指数部分,是否出现过小数点,判断指数和小数是否为负
	int32_t temp_i = 0, temp_d = 0, move_i = 0; //计数器,分别存储现在写入到了指数和小数的第几位
	int32_t pre_ptr = ptr;
	char* p = bufin;
	if (*bufin == 0)
		return 0;
	if (*p == '-')  //先处理第一个字符
	{
		p++;
		negd = 1;
	}
	else if (*p == '+')
		p++;
	else if (*p > 0x39 || *p < 0x30)
		Illegal_pro(p, bufin);
	while (*p && p - bufin < len)
	{
		//可能出现符号的部分只有E后或\n后，已全部被特殊处理
		if (*p == '\r')
		{
			p++;
			continue;
		}
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
						flag2 = 1; //小数点出现标记
						while (*p == 0x30)  //过滤掉无效0位
						{
							p++;
							move_i++;
						}
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
		if (*p == 0x65 || *p == 0x45)	//e/E
		{
			flag = 1;   //进入指数部分
			p++;
			if (*p == 0x2D) //指数为负
			{
				negi = 1;
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
			flag2 = 1;
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
bool ap_Getfromfile(int64_t buffer[], int32_t& ptr, char*& bufin)
{
	/*从文件流file中读入数据到bufin中,并将字符串编码后存入buffer
	直到buffer存满或者文件读完
	参数设置：输入缓冲区、指针、输入容器*/
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
				char temp[90] = { 0 };
				fgets(temp, 90, inputfile);
				memcpy(bufin + strlen(bufin), temp, strlen(temp));
				int64_t len = strlen(bufin);
				if (len == 0)
				{
					legal_count += ptr;
					fclose(inputfile);
					inputfile = 0;
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
			inputfile = 0;
		}
	}
	return ptr;
}
void SelectMin(int64_t ap_value[], int32_t rn[], int32_t ap_loser[], int32_t q, int32_t& rq)
{
	/*从q开始向上到根结点调整败者树
	 胜者段号存入rq*/
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
	/*生成初始归并段，返回归并段的数量input为输入文件地址*/
	int64_t* ap_inbuffer = new int64_t[ap_inbuffer_max]; //输入缓冲区
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

bool msort_Getfromfile(int32_t i, int64_t buffer[], int32_t& ptr, FILE* file[])
{
	/*将file中的浮点数存入buffer中，个数存入ptr*/
	ptr = 0;
	int64_t start = _ftelli64(file[i]);
	_fseeki64(file[i], 0, SEEK_END);
	int64_t end = _ftelli64(file[i]);
	_fseeki64(file[i], start, 0);
	start = (end - start) / 8;
	ptr = start > msort_inbuffer_max ? msort_inbuffer_max : start;  //存读入的浮点数个数
	fread(buffer, 8, ptr, file[i]);
	return ptr;
}

void decode(int64_t buffer[], int32_t ptr, char*& bufout)
{
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
		if ((buffer[i] & 0x00f0000000000000) == 0x0020000000000000)	//写指数符号位
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
		bufout[p + 16] = '\r';
		bufout[p + 17] = '\n';
		p += 18;
		neg = 0; decimal = 10; index = 15; //准备解码下一个
	}
	fwrite(bufout, 1, p, endout);
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
bool test_if_infull(bool* ifin1, bool* ifin2)
{
	/*判断是否输入缓冲区都满（1/2至少有一个满)*/
	for (int32_t i = 0; i < cntall; i++)
	{
		if (!ifin1[i] && !ifin2[i])
			return false;
	}
	return true;
}
bool test_read_end(FILE* file[])
{
	/*判断是否所有的文件都已经读完*/
	for (int32_t i = 0; i < cntall; i++)
	{
		if (file[i])
			return false;
	}
	return true;
}
bool test_if_inemp(bool* ifin1, bool* ifin2)
{
	/*若缓冲区有空，返回true*/
	for (int32_t i = 0; i < cntall; i++)
	{
		if (!ifin1[i] || !ifin2[i])
			return true;
	}
	return false;
}
string get_name(int32_t cc)
{
	/*根据文件次序得出它的命名部分*/
	string name = "_";
	int32_t i = 0;
	while (cc < cntadd[i] || cc >= cntadd[i + 1])
		i++;
	char temp[5] = { 0 };
	sprintf(temp, "%d", cc - cntadd[i]);
	name = char(i + '0') + name + temp;
	return name;
}
void msort_read(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max], int32_t* inptr2, bool* ifin2)
{
	/*从临时文件（初始归并段）读入输入缓冲区
	参数设置：输入文件数组；输入缓冲区1，尾指针，状态标记；输入缓冲区2，尾指针，状态标记*/
	while (1)
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
						file[i] = 0;
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
						file[i] = 0;
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
	/*将输出缓冲区内容解码后写出文件
	参数设置：输出缓冲区1，指针；输出容器；输出缓冲区1标志， 是否全部写完标志*/
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
	/*将输出缓冲区内容解码后写出文件
	参数设置：输出缓冲区1，指针；输出容器；输出缓冲区2标志， 是否全部写完标志*/
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
int mergesort(FILE* file[], int64_t(*inbuffer1)[msort_inbuffer_max], int32_t* inptr1, bool* ifin1, int64_t(*inbuffer2)[msort_inbuffer_max],
	int32_t* inptr2, bool* ifin2, int64_t* outbuffer1, int32_t& outptr1, int64_t* outbuffer2, int32_t& outptr2)
{
	/*对归并段进行多路归并。
	参数设置：输入文件数组，输入缓冲区1，指针，状态标志；输入缓冲区2，指针，状态标志；输出缓冲区1，指针；输出缓冲区2，指针*/
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
	printf("Merge sorting!\n");
	msort_k = kk;
	FILE* (*file) = new FILE * [msort_k]; //为每个归并段生成输入文件
	bool* ifin1 = new bool[msort_k + 1];  //每个输入缓冲区1的标志
	bool* ifin2 = new bool[msort_k + 1];  //每个输入缓冲区1的标志
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
		printf("No number has been sorted.\nPlease cheack the input file!\n");
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
	delete[] * inbuffer1;
	delete[] * inbuffer2;
	delete[] outbuffer1;
	delete[] outbuffer2;
	delete[] inptr1;
	delete[] bufout;
	delete[] inptr2;
}













