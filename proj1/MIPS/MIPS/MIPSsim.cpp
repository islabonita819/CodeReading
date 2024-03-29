// MIPS.cpp: 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include<iostream>
#include<fstream>
#include<vector>
#include<map>
#include<string>
#include<sstream>
using namespace std;

//指令的数据结构：将01串转换为汇编指令
struct disassembled
{
	//指令地址
	int address;
	//指令种类，进行后续的条件判断
	int id;
	//指令操作码
	string operate;
	//操作数t1, t2, t3
	int t1;
	int t2;
	int t3;
	//输出的汇编指令:最后在文件中指令的输出格式
	string assembly;
};

//数据的数据结构：将01串转变为十进制数值
struct Data {
	//数据地址
	int address;
	//数据值
	int num;
};

//种类1中所包含的指令枚举
enum opcodes1 {
	J, JR, BEQ, BLTZ, BGTZ, ADD1, SUB1, MUL1, BREAK, SW, LW, SLL, SRL, SRA, NOP, AND1, NOR1, SLT1
};

//种类2中所包含的指令枚举
enum opcodes2 {
	ADD2, SUB2, MUL2, AND, NOR, SLT, NUM
};

//将输入文件保存在s中
vector<string> s;
//保存指令
vector<disassembled> answers;
//保存数据
vector<Data> datas;
//指令的起始地址
int currentAddress = 64;
//输出文件里面寄存器的排列形式
int registers[2][16] = { 0 };
//方便进行条件判断，代码可读性
static map<string, opcodes1> mapStringValues1;
static map<string, opcodes2> mapStringValues2;

//将二进制数据转换为十进制数据，不考虑符号
int strToIntNoSn(string in)
{
	int i = 0;
	while (in[i] == '0') { i++; };
	//获取从非零位置开始的字符串
	in = in.substr(i);
	int num = in.size(), mid = 0, tt = 1;
	for (i = 0; i < num; i++)
	{
		mid += (in[num - 1 - i] - '0') * tt;
		tt *= 2;
	}
	return mid;
}

//将二进制数据转化为十进制数据，考虑符号位
int strToIntWithSn(string in)
{
	int mid = 0, num = in.size(), tt = 1, i = 0;
	if (in[0] == '0')
	{
		mid = strToIntNoSn(in.substr(1));
	}
	else
	{
		bool flag = false;
		for (i = num - 1; i >= 0; i--)
		{
			if (in[i] == '0')
			{
				in[i] = '1';
			}
			else
			{
				if (i != 0)
				{
					in[i] = '0';
					break;
				}
				else
				{
					flag = true;
				}
			}
		}
		if (flag == true)
		{
			mid = strToIntNoSn(in.substr(1))*-1;
		}
		else
		{
			for (i = 1; i < num; i++)
				if (in[i] == '0')
					in[i] = '1';
				else
					in[i] = '0';
			mid = strToIntNoSn(in.substr(1))*-1;
		}
	}
	return mid;
}

//Disassembly和Simlution文件里面指令的输出格式不一样，进行相应的转化
string transformInst(disassembled instr)
{
	//待分割的字符串，含有很多空格
	string assembly = instr.assembly;
	//存储返回结果
	string resultStr;
	//用于存放分割后的字符串 
	vector<string> res;
	//暂存从word中读取的字符串 
	string result;
	//将字符串读到input中 
	stringstream input(assembly);
	//依次输出到result中，并存入res中 
	while (input >> result)
		res.push_back(result);
	int count = res.size();
	for (unsigned int i = 0; i < res.size(); i++) {
		if (count == 1)
		{
			resultStr = res[i];
		}
		else
		{
			if (i == 0)
			{
				resultStr = resultStr + res[i] + '\t';
			}
			else if (i == res.size() - 1)
			{
				resultStr = resultStr + res[i];
			}
			else
			{
				resultStr = resultStr + res[i] + ' ';
			}
		}
	}
	return resultStr;
}

//读取输入文件，写入变量s
void read(string inputFile)
{
	ifstream infile;
	//将文件流对象与文件连接起来 
	infile.open(inputFile.data());
	string input;
	while (getline(infile, input))
	{
		s.push_back(input.substr(0, 32));
	}
	infile.close();
}

//Disassembly:将数据按照格式输出到文件
void writeDisassembly(string outputFile)
{
	ofstream outf;
	outf.open(outputFile);
	for (unsigned int i = 0; i < s.size(); i++)
	{
		if (i < answers.size())
			outf << s.at(i).substr(0, 6) << " "
				<< s.at(i).substr(6, 5) << " " 
				<< s.at(i).substr(11, 5) << " " 
				<< s.at(i).substr(16, 5) << " " 
				<< s.at(i).substr(21, 5) << " " 
				<< s.at(i).substr(26, 6) << "\t" << answers.at(i).address << "\t" << answers.at(i).assembly << endl;
		else
			outf << s.at(i) + "\t" << datas.at(i - answers.size()).address << "\t" << datas.at(i - answers.size()).num << endl;
	}
	outf.close();
}

//处理输入文件，进行指令类型与功能的判定，并存入到数据结构
void disassembly()
{
	//判断是否遇到了BREAK指令，如果是接下来所有的输入都是32位字符串的数值
	bool flag = false;
	for (unsigned int i = 0; i < s.size(); i++)
	{
		
		
		int t1, t2, t3;
		string line = s.at(i);
		//取前六位，后六位一起判断指令类型
		string fsix = line.substr(0, 6);
		string lsix = line.substr(26, 6);
		stringstream t11, t22, t33;
		disassembled answer;
		Data data;
		if (flag == false)
		{
			//如果前六位是"000000"，就要根据后六位判断是哪一个指令了
			if (fsix.compare("000000") == 0)
			{
				switch (mapStringValues1[fsix.append(lsix)])
				{
				case JR:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t11 << t1;
					answer.assembly = "JR R" + t11.str();
					answer.address = currentAddress;
					answer.operate = "000000001000";
					answer.t1 = t1;
					currentAddress += 4;
					answers.push_back(answer);
					break;
				case ADD1:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t2 = strToIntNoSn(line.substr(11, 5));
					t3 = strToIntNoSn(line.substr(16, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "ADD R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
					answer.address = currentAddress;
					answer.operate = "000000100000";
					answer.t1 = t3;
					answer.t2 = t1;
					answer.t3 = t2;
					currentAddress += 4;
					answers.push_back(answer);
					break;
				case SUB1:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t2 = strToIntNoSn(line.substr(11, 5));
					t3 = strToIntNoSn(line.substr(16, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "SUB R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
					answer.address = currentAddress;
					answer.operate = "000000100010";
					answer.t1 = t3;
					answer.t2 = t1;
					answer.t3 = t2;
					currentAddress += 4;
					answers.push_back(answer);
					break;
				case BREAK:
					answer.id = 1;
					answer.assembly = "BREAK";
					answer.address = currentAddress;
					currentAddress += 4;
					answer.operate = "000000001101";
					answers.push_back(answer);
					flag = true;
					break;
				case SLL:
					//全为零的时候是NOP
					if (line.compare("00000000000000000000000000000000") == 0)
					{
						answer.id = 1;
						answer.assembly = " ";
						answer.address = currentAddress;
						currentAddress += 4;
						//暂时没有指令码与“000000000001”冲突，就用它表示NOP
						answer.operate = "000000000001";
						answers.push_back(answer);
						break;
					}
					//不全为零的时候指令是SLL
					else
					{
						answer.id = 1;
						t1 = strToIntNoSn(line.substr(11, 5));
						t2 = strToIntNoSn(line.substr(16, 5));
						t3 = strToIntNoSn(line.substr(21, 5));
						t11 << t1;
						t22 << t2;
						t33 << t3;
						answer.assembly = "SLL R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
						answer.address = currentAddress;
						answer.operate = "000000000000";
						answer.t1 = t2;
						answer.t2 = t1;
						answer.t3 = t3;
						currentAddress += 4;
						answers.push_back(answer);
						break;
					}
				case SRL:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(11, 5));
					t2 = strToIntNoSn(line.substr(16, 5));
					t3 = strToIntNoSn(line.substr(21, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "SRL R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
					answer.address = currentAddress;
					answer.operate = "000000000010";
					answer.t1 = t2;
					answer.t2 = t1;
					answer.t3 = t3;
					currentAddress += 4;
					answers.push_back(answer);
					break;
				case SRA:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(11, 5));
					t2 = strToIntNoSn(line.substr(16, 5));
					t3 = strToIntNoSn(line.substr(21, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "SRA R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
					answer.address = currentAddress;
					answer.operate = "000000000011";
					answer.t1 = t2;
					answer.t2 = t1;
					answer.t3 = t3;
					currentAddress += 4;
					answers.push_back(answer);
					break;
					//NOP在SLL里面单独判断单独判断了，这一个分支不会走进去
				case NOP:
					answer.id = 1;
					answer.assembly = " ";
					answer.address = currentAddress;
					currentAddress += 4;
					answer.operate = "000000000001";
					answers.push_back(answer);
					break;
				case AND1:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t2 = strToIntNoSn(line.substr(11, 5));
					t3 = strToIntNoSn(line.substr(16, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "AND R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
					answer.address = currentAddress;
					answer.operate = "000000100100";
					answer.t1 = t3;
					answer.t2 = t1;
					answer.t3 = t2;
					currentAddress += 4;
					answers.push_back(answer);
				case NOR1:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t2 = strToIntNoSn(line.substr(11, 5));
					t3 = strToIntNoSn(line.substr(16, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "NOR R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
					answer.address = currentAddress;
					answer.operate = "000000100111";
					answer.t1 = t3;
					answer.t2 = t1;
					answer.t3 = t2;
					currentAddress += 4;
					answers.push_back(answer);
				case SLT1:
					answer.id = 1;
					t1 = strToIntNoSn(line.substr(6, 5));
					t2 = strToIntNoSn(line.substr(11, 5));
					t3 = strToIntNoSn(line.substr(16, 5));
					t11 << t1;
					t22 << t2;
					t33 << t3;
					answer.assembly = "SLT R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
					answer.address = currentAddress;
					answer.operate = "000000101010";
					answer.t1 = t3;
					answer.t2 = t1;
					answer.t3 = t2;
					currentAddress += 4;
					answers.push_back(answer);
				default:
					break;
				}
			}
			else if (fsix == "000010")
			{
				t1 = strToIntNoSn(line.substr(6).append("00"));
				t11 << t1;
				answer.id = 1;
				answer.assembly = "J #" + t11.str();
				answer.address = currentAddress;
				answer.operate = "000010";
				answer.t1 = t1;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "000100")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16).append("00"));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "BEQ R" + t11.str() + ", R" + t22.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "000100";
				answer.t1 = t1;
				answer.t2 = t2;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "000001")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(16, 16).append("00"));
				t11 << t1;
				t22 << t2;
				answer.assembly = "BLTZ R" + t11.str() + ", #" + t22.str();
				answer.address = currentAddress;
				answer.operate = "000001";
				answer.t1 = t1;
				answer.t2 = t2;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "000111")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(16, 16).append("00"));
				t11 << t1;
				t22 << t2;
				answer.assembly = "BGTZ R" + t11.str() + ", #" + t22.str();
				answer.address = currentAddress;
				answer.operate = "000111";
				answer.t1 = t1;
				answer.t2 = t2;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "011100")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 5));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "MUL R" + t33.str() + ", R" + t11.str() + ", R" + t22.str();
				answer.address = currentAddress;
				answer.operate = "011100";
				answer.t1 = t3;
				answer.t2 = t1;
				answer.t3 = t2;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "101011")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "SW R" + t22.str() + ", " + t33.str() + "(R" + t11.str() + ")";
				answer.address = currentAddress;
				answer.operate = "101011";
				answer.t1 = t2;
				answer.t2 = t3;
				answer.t3 = t1;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "100011")
			{
				answer.id = 1;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "LW R" + t22.str() + ", " + t33.str() + "(R" + t11.str() + ")";
				answer.address = currentAddress;
				answer.operate = "100011";
				answer.t1 = t2;
				answer.t2 = t3;
				answer.t3 = t1;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "110000")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "ADD R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "110000";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "110001")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "SUB R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "110001";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "100001")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "MUL R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "100001";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "110010")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "AND R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "110010";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "110011")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "NOR R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "110011";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else if (fsix == "110101")
			{
				answer.id = 2;
				t1 = strToIntNoSn(line.substr(6, 5));
				t2 = strToIntNoSn(line.substr(11, 5));
				t3 = strToIntNoSn(line.substr(16, 16));
				t11 << t1;
				t22 << t2;
				t33 << t3;
				answer.assembly = "SLT R" + t22.str() + ", R" + t11.str() + ", #" + t33.str();
				answer.address = currentAddress;
				answer.operate = "110101";
				answer.t1 = t2;
				answer.t2 = t1;
				answer.t3 = t3;
				currentAddress += 4;
				answers.push_back(answer);
			}
			else
			{
				data.address = currentAddress;
				data.num = strToIntWithSn(line);
				currentAddress += 4;
				datas.push_back(data);
			}
		}
		else
		{
			data.address = currentAddress;
			data.num = strToIntWithSn(line);
			cout << data.num << endl;
			currentAddress += 4;
			datas.push_back(data);
		}
	}
	writeDisassembly("disassembly.txt");
}

//Simlution:将数据按照格式输出到文件
void writeSimlution(ofstream &outf, int time, disassembled answer)
{
	outf << "--------------------" << endl;
	outf << "Cycle:" << time << "\t" << answer.address << "\t" << transformInst(answer) << endl;
	outf << endl;

	outf << "Registers" << endl;
	outf << "R00:";
	for (int k = 0; k < 16; k++)
	{
		if (k == 10)
		{
			outf << "\t" << registers[0][k] << endl;
		}
		else
		{
			outf << "\t" << registers[0][k];
		}
	}
	outf << endl;
	outf << "R16:";
	for (int k = 0; k < 16; k++)
	{
		if (k == 10)
		{
			outf << "\t" << registers[1][k] << endl;
		}
		else
		{
			outf << "\t" << registers[1][k];
		}
	}
	outf << endl;
	outf << endl;

	//将BREAK后面的数据按每行8个进行输出
	outf << "Data" << endl;
	int row = datas.size() / 8;
	if (datas.size() % 8 > 0)
		row++;
	for (unsigned int j = 0; j < datas.size(); )
	{
		outf << datas.at(j).address << ":";
		for (int k = 0; k < 8; k++)
		{
			outf << "\t" << datas.at(j).num;
			j++;
		}
		outf << endl;
	}
	outf << endl;
}

//初始化操作码与操作符
void initialize()
{
	mapStringValues1["000010"] = J;
	mapStringValues1["000000001000"] = JR;
	mapStringValues1["000100"] = BEQ;
	/*暂时没有冲突，决定BLTZ的不是前六位（前六位决定SPECLAI2），只是
	这里面涉及到的指令跟“000001”不冲突，就这么设计了*/
	mapStringValues1["000001"] = BLTZ;
	mapStringValues1["000111"] = BGTZ;
	mapStringValues1["000000100000"] = ADD1;
	mapStringValues1["000000100010"] = SUB1;
	mapStringValues1["011100"] = MUL1;
	mapStringValues1["000000001101"] = BREAK;
	mapStringValues1["101011"] = SW;
	mapStringValues1["100011"] = LW;
	mapStringValues1["000000000000"] = SLL;
	mapStringValues1["000000000010"] = SRL;
	mapStringValues1["000000000011"] = SRA;
	//应该是“000000000000”，但是避免跟SLL冲突改了一下（设计的不太友好）
	mapStringValues1["000000000001"] = NOP;
	mapStringValues1["000000100100"] = AND1;
	mapStringValues1["000000100111"] = NOR1;
	mapStringValues1["000000101010"] = SLT1;

	mapStringValues2["110000"] = ADD2;
	mapStringValues2["110001"] = SUB2;
	mapStringValues2["100001"] = MUL2;
	mapStringValues2["110010"] = AND;
	mapStringValues2["110011"] = NOR;
	mapStringValues2["110101"] = SLT;
	mapStringValues2["1111"] = NUM;

}

//处理输入文件里指令的仿真操作，涉及到指令判断，指令操作，立即数和寄存器数据处理以及下一个PC地址的计算
void simluation()
{
	ofstream outf;
	outf.open("simlution.txt");
	int time = 1;
	disassembled cur = answers.at(0);
	while (true)
	{
		//判断下次是否为BREAK
		if (cur.id == 1)
		{
			if (mapStringValues1[cur.operate] == BREAK)
			{
				writeSimlution(outf, time, cur);
				time++;
				break;
			}
		}
		//如果是第一类指令
		if (cur.id == 1)
		{
			switch (mapStringValues1[cur.operate])
			{
			case J:
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.t1 - 64) / 4);
				time++;
				break;
			case JR:
				writeSimlution(outf, time, cur);
				cur = answers.at((registers[cur.t1 / 16][cur.t1 % 16] - 64) / 4);
				time++;
				break;
			case BEQ:
				writeSimlution(outf, time, cur);
				if (registers[cur.t1 / 16][cur.t1 % 16] - registers[cur.t2 / 16][cur.t2 % 16] == 0)
					cur = answers.at((cur.t3 + cur.address - 64) / 4 + 1);
				else
					cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case BLTZ:
				writeSimlution(outf, time, cur);
				if (registers[cur.t1 / 16][cur.t1 % 16] < 0)
					cur = answers.at((cur.t2 + cur.address - 64) / 4 + 1);
				else
					cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case BGTZ:
				writeSimlution(outf, time, cur);
				if (registers[cur.t1 / 16][cur.t1 % 16] > 0)
					cur = answers.at((cur.t2 + cur.address - 64) / 4 + 1);
				else
					cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case ADD1:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] + registers[cur.t3 / 16][cur.t3 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SUB1:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] - registers[cur.t3 / 16][cur.t3 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case MUL1:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] * registers[cur.t3 / 16][cur.t3 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SW:
				datas.at((registers[cur.t3 / 16][cur.t3 % 16] + cur.t2 - 148) / 4).num = registers[cur.t1 / 16][cur.t1 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case LW:
				registers[cur.t1 / 16][cur.t1 % 16] = datas.at((registers[cur.t3 / 16][cur.t3 % 16] + cur.t2 - 148) / 4).num;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SLL:
				registers[cur.t1 / 16][cur.t1 % 16] = (unsigned)registers[cur.t2 / 16][cur.t2 % 16] << cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SRL:
				registers[cur.t1 / 16][cur.t1 % 16] = (unsigned)registers[cur.t2 / 16][cur.t2 % 16] >> cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SRA:
				registers[cur.t1 / 16][cur.t1 % 16] = (signed)registers[cur.t2 / 16][cur.t2 % 16] >> cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case NOP:
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case AND1:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] & registers[cur.t3 / 16][cur.t3 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case NOR1:
				registers[cur.t1 / 16][cur.t1 % 16] = ~(registers[cur.t2 / 16][cur.t2 % 16] | registers[cur.t3 / 16][cur.t3 % 16]);
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SLT1:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] < registers[cur.t3 / 16][cur.t3 % 16];
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			default:
				break;
			}
		}
		//如果是第二类指令
		else
		{
			switch (mapStringValues2[cur.operate])
			{
			case ADD2:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] + cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SUB2:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] - cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case MUL2:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] * cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case AND:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] & cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case NOR:
				registers[cur.t1 / 16][cur.t1 % 16] = ~(registers[cur.t2 / 16][cur.t2 % 16] | cur.t3);
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case SLT:
				registers[cur.t1 / 16][cur.t1 % 16] = registers[cur.t2 / 16][cur.t2 % 16] < cur.t3;
				writeSimlution(outf, time, cur);
				cur = answers.at((cur.address - 64) / 4 + 1);
				time++;
				break;
			case NUM:
				break;
			default:
				break;
			}
		}
	}
	outf.close();
}

int main()
{
	initialize();
	string filename;
	cin >> filename;
	read(filename);
	disassembly();
	simluation();

	return 0;
}

