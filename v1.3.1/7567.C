/*
version:7567 V1.1
date:
description:
	此版本已将行为库转化为双位存储方式以将行为库的内存占用减少至25%。
*/

/*
version:7567 V1.2
date:	
	20240406(start)
	20240414(finish)
description:
	该版本添加了基础的用户交互界面，目前已完成主选单、游戏选择、系统介绍（第一页）。
	未来目标：优化交互系统、优化按键动画、提高系统模块化程度、制作文本自动换行器、制作通用的翻页系统。
*/


/*
version:7567 V1.3
date:
	20240510(start)
	20240619(finish)
description:
	该版本添加基于贪吃蛇坐标检测与处理架构的俄罗斯方块游戏
	微调了部分函数功能
*/


/*
[日志]
在掌握可变数组与链表后可尝试升级为动态行为库；
但不建议，因为了解到可变数组只能顺序开辟内存，而链表中每个单元均需一个字节存放地址，在此项目中可能降低内存利用率

4月13日发现不应使用xdata存放code的地址，会无法正确寻址。
wave6000仿真时，存放在xdata和data指针的地址看似一致，但在解引用时xdata的地址无法正确指向，猜测是xdata的数据经过多次解引用，出现引用错误

6月9日
更正之前使用xdata寻址错误的问题，实际上是在定义变量时应当正确指定
例如	uint xdata* data p		表示在data区定义一个指向xdata的指针

5月30日
未调用的函数会被编译器识别为中断函数，导致其中的变量被作为全局变量编译，不使用的函数可以注释掉或者在主函数中写出但通过条件使其不调用

6月19日
编译器检查函数声明与定义的时候如果出现定义在xdata的变量，则声明与定义不一致不会报错，但检查函数传入值时仍以声明时传入值的类型检查，导致编译时报错提示传入值类型不匹配
修改函数定义时应注意同步修改函数声明！
新的FPC转接板今日已发货，后续将使用新板。
对write_command()仿真发现使用if判断比使用多次运算速度更快。
*/



/*
[ST7567通讯指南]

[4-lines SPI]
需要CSB（片选）、SDA（数据）、SCL（时钟）、A0（地址选择）
	CSB：数据发送前使SCB=0，发送完8位数据后使SCB=1并保持
	SDA：数据传输，在CSB=0，SCL=0后开始按位发送
	SCL：数据在上升沿锁存，故应先使SCL=0，待SDA确定后使SCL=1，锁存成功后再次使SCL=0，重复进行
	A0：当A0=0时发送为指令，当A0=1时发送为显示数据，与第8位数据同时锁存，建议在发送开始前确定

[3-lines SPI]
需要CSB（片选）、SDA（数据）、SCL（时钟）
	与4-lines SPI类似，减少了A0线，转而将数据线的第1位作为A0地址选择，相应地SDA现在一次发送9位数据

[I2C serial]
建议阅读ST7567A技术手册关于此部分的内容
需要SDA（数据）、SCL（时钟）
	无数据发送时SDA及SCL均应置高
	在数据传输过程中，时钟高电平时数据正在被转化，此时应保持SDA电平稳定，否则可能被判断为起止标志

	一、起止方式：
	当SCL=1时，SDA=1 => SDA=0 标志数据发送开始；当SCL=1时，SDA=0 => SDA=1 标志数据发送结束

	二、校验位：
	master每发送1字节数据后使SDA=1，此时slave必需使SDA保持稳定低电平以完成校验，否则数据无效
	（由此可知使用I2C时应使用IO口开漏输出模式？）

	三、交互协议/数据发送：
	在进行数据发送前，必须先给总线上的设备分配地址，0111100,011101,011110,0111111为7567保留的地址
	将SA0和SA1置为高电平或低电平以设置从地址
	开始标志后master将首先发送从设备地址，只有与发送的地址相匹配的设备会接收随后的数据
	从地址选定后，将首先发送一个Control Byte以设定从设备的工作状态，
	首先是Control Byte,字节的剩余7位为其他的指令，如A0
	完成该字节校验后发送Data Byte并校验，随后重复先发送control byte再发送data byte的操作直至终止标志u




*/

#include <reg51.h>
//#include <stdio.h>
#define  uint  unsigned int
#define  uchar unsigned char
#include "chinese_cha.h"
#include "icon_lib.h"
#include "assii.h"

#define data_port P1

//sbit 	cs1 = P3^4;
//sbit  rs = P3^2;
//sbit	rw =P3^6;
//sbit	rd =P3^7;
//sbit	sclk =P3^1;
//sbit	sda =P3^0;
//sbit	res =P3^3;

//ZHANGRU 20160909
sbit 	cs1 = P3^0;
sbit	res =P3^1;
sbit    rs = P3^2;
sbit	rw =P3^3;
sbit	rd =P3^4;


sbit	sclk =P3^3;
sbit	sda =P3^4;
sbit	sclk1 =P1^0;
sbit	sda1 =P1^1;
sbit	sclk2 =P1^6;
sbit	sda2 =P1^7;

//LCD控制器为ST7567A，使用SPI接口进行通信
//sclk 时钟信号，由主控制器提供
//sdin 数据输入信号，由主控制器提供
//cs 片选信号，由主控制器提供，用于选择ST7567A进行通信
//RES 复位信号，由主控制器提供，用于复位
//A0 命令/数据选择信号，由主控制器提供

//控制板上还有P3^0,P3^1,P3^2,P3^3,P3^4待拓展

//当前LCD12048有效位为:cs1(P3^0)、rs(P3^2)、sda2(P1^7)、sclk2(P1^6);
//以下IO为其他硬件使用:sclk =P3^3,sda =P3^4,sclk1 =P1^0,sda1 =P1^1;
//LCD复位控制:res=P3^1

/*
IO使用状态:
P1^0	P1^1	P1^2	P1^3	P1^4	P1^5	P1^6	P1^7
sclk	sda1									sclk2	sda2
(J1)	(J1)	(J1)	(J1)	(J1)	(J1)	(J1)	(J1)



P2^0	P2^1	P2^2	P2^3	P2^4	P2^5	P2^6	P2^7
->key3	->key2	->key1
(con2)	(J20)	(J20)	(J20)	(J20)	(J20)	(J20)	(J20)
P2未焊接排针，暂时不可用


P3^0	P3^1	P3^2	P3^3	P3^4	P3^5	P3^6	P3^7
cs1		res		rs		sclk	sda		
RXD		TXD				
(J1)	(J1)	(J1)	(J1)	(J1)	(J1)	(J1)

*/

//#######################################################################################################
//结构体列表：


	//按键类型---------------------------------
	typedef struct ui_type_1
	{
		//显示内容
		uchar writing_width;
		uchar writing_height;
		uchar *writing_data;
	}
	key_type1;	//定义按键图标显示结构

	//光标类型----------------------------------
	typedef struct cursor_type_1
	{
		uchar cursor_point;
		uchar cursor_max;
	}
	cursor_type1;

	//方块结构类型，如何确保结构体存在xdata区？
	typedef struct square_type_1
	{
		uint coordx[4];
		uint coordy[4];
	}
	square_type1;


//######################################################################################################
//函数列表：

	void lcd_init(void);
	void lcd_rest(void);

	void n_ms(uchar x);
	void delay_n_s(uchar x);
	void delay_n_100ms(uchar x);			//延时N-S子程序

	void key_scan(void);
	void key_scan1(uint dtime);//可修改等待的按键
	uchar key_scan2(uint dtime);//限时按键检测功能，使用对P2完整设定实现，建议开发对P2位检测的方法
	uint key_scan3(uint dtime);//单键检测，返回相应时间（循环次数）
	uint key_scan4();//多键检测，不限时

	void write_command(uchar command);//发送指令
	void write_data(uchar dis_data);//发送数据
	void set_page_address(uchar x);//设定page
	void set_column_address(uchar x);//设定column

	//这4个函数无实际功能，推测为早期开发使用
	/*
	void prt_one_char_sub(uchar start_column,start_page,uchar *i);
	void disp_timer(uchar start_column,start_page,uchar second);
	void prt_one_char(uchar start_column,start_page,uchar *char_pointer);
	void read_data_tab(uchar *pp);
	*/

	void lcd_disp_tab(void);//写四边框，需在子程序中手动设定边界
	void lcd_disp_full(uchar x);//写点阵，实际常用于清屏
	void lcd_disp_test_icon_2(void);//写测试图标子程序
	void lcd_disp_test_icon(void);//写测试图标子程序

	void disp_single_char(uchar column,page,uchar *text_pointer);//写单字
	void disp_full_picture(uchar column,page,uchar *pic_pointer);//写全屏图片
	void disp_multi_cha_16(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width);//由disp_full_picture()修改而来，原用于写96*16的图片，后也用于写多个高为16的字
	void disp_arb_pic1(uchar column,page,uchar *pic_pointer,uchar pic_height,uchar pic_width,uchar space_content);//写任意图片

	void disp_single_square1(uchar column,page,uchar data_write_in);//写4*4方格（早期开发版本）
	void disp_single_square2(uchar head_column,uchar head_line);//写4*4方格，可自动识别上半page或下半page

	//贪吃蛇写入4*4方格，与disp_89()配合
	void disp_single_area(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width);

		//对disp_single_area的控制
		void switch_case0(uchar icolumn_t,uchar typ_t);
		void switch_case1(uchar icolumn_t,uchar typ_t);
		void switch_case2(uchar icolumn_t,uchar typ_t);
		//8page的9行式坐标的带边框显示功能
		void disp_89(uint icolumn_temp2,uchar area,uchar typ);

	//双位行为库的写入与读取
	void act_rec_write(uchar num,uchar direction,uchar *act_rec_adr);
	uchar act_rec_read(uchar num,uchar *act_rec_adr);

	//俄罗斯方块功能----------------------
	uchar square_rotate(square_type1 xdata *p_square,uint xdata *coord_lib);//旋转
	void square_origin_set(square_type1 xdata *p_square);//重设旋心
	uchar square_edge_detect(square_type1 xdata *p_square,uchar direction);//边缘检测
	uchar square_move(square_type1 xdata *p_square,uint xdata *coord_lib,uchar direction);//移动俄罗斯方块坐标
	void square_wipe(uint xdata * xdata coord_lib,uint xdata * top_coordx);//检测清行，含刷新显示

	//坐标缓存修改，未完成
	void coordgroup_single_change(uint *p_coordgroup,uint x,uint x_max,uint y,uchar content);
	
	void pin_detect_write(uint icolumn_temp,uint loc1_temp);//3位探针检测与显示刷新

	//贪吃蛇检测蛇头碰撞的对象（食物or蛇身）
	//icolumn_temp为传入列位置，loc1_temp为传入坐标列
	uchar pin_detect_crash(uint icolumn_temp,uint loc1_temp,uint iline_temp,uchar *act_rec_adr,uchar length_temp,uchar game_status_temp);

	uint loc_process1(uint iline_t);//目标修改坐标组的预处理

	uchar pin_read_bit(uint xdata data_temp,uint xdata column);//双字节单数据位读取器
	uchar num_set(char change,uchar max_num,uchar pre_num);//超限自动归零计算器

	uchar cursor_left(cursor_type1 *cp);//光标左移一位
	uchar cursor_right(cursor_type1 *cp);//光标右移一位
	void key_content_change(key_type1 *key_p,uchar *content);//按键显示内容变更
	void cha_width_change(key_type1 *cha_p,uchar content);//按键字幕宽度变更

	//线性同余算法，文心一言提供，人工修改适配
	unsigned int prng(uint xdata prng_seed_temp);
	uchar prng_in_range(uint xdata prng_seed_temp,uchar xdata prng_max_temp,uchar xdata prng_min_temp);

	void count_write(uchar length_temp);//显示当前贪吃蛇长度，x显示测试功能，仅可显示0-9;

	void rotation_coord_90(uint xdata origin_x,uint xdata origin_y,uint *target_x,uint *target_y);//单坐标顺时针90度旋转

	//交互层切换函数
	//后续升级中应尝试修改为 链表+状态机 模式
	uchar Interface_Main();//主界面
	uchar Interface_GameSelect();//游戏选择界面
	uchar Interface_SystemDescribe();//系统介绍界面
	void Interface_Core();//界面跳转系统

	void snake_program(uint prng_seed);//贪吃蛇游戏
	void rusq_program(uint prng_seed);//俄罗斯方块游戏








//LCD初始化子程序*********************************************************************
void lcd_init(void)
{
        write_command(0xe2);//软复位	0xe2=11100010
        n_ms(5);
        write_command(0x2f);//电源开	0x2f=00101111
        write_command(0xaf);//显示开	0xaf=10101111
        write_command(0x23);//粗条对比度，0x21-0x27		0x23=00100011
        write_command(0x81);//双字节指令，微调对比度	 0x81=10000001
        write_command(47);       //38=9.0v  30=8.5v  //对比度调节，0-63
        write_command(0xc8);	 //上下,c0,c8		0xc8=11001000
        write_command(0xa0);	 //左右,a0,a1		0xa0=10100000

//write_command(0x60);
}

// LCD复位子程序*********************************************************************
void lcd_rest(void)
{
	res=0;			//res =P3^1;
	n_ms(200);
	res=1;
	n_ms(200);
//	n_ms(200);
}


//延时N-MS子程序********************************************************************
void n_ms(uchar x)			//延时N-MS子程序
{
	uchar    i,d=1;
        for(i=0;i<x;++i)
        {
			for(d=0;d<50;d++);
        }
}
//延时N-S子程序********************************************************************
/*
void delay_n_s(uchar x)			//延时N-S子程序
{
        uint i;
        uchar y;
        for(y=0;y<x;y++)
        {
        	for(i=0;i<33000;i++);
        }
}
*/

//延时N-S子程序********************************************************************
void delay_n_100ms(uchar x)			//延时N-S子程序 (12mhz)
{
        uint i;
        uchar y;
        for(y=0;y<x;y++)
        {
        	for(i=0;i<7690;i++);
        }
}


//处理按键程序*************************************************************************************
//仅检测单键，会等待按键松开
void key_scan(void)						//按键处理
{
	uchar i;
	P2=0Xff;
key_in:
	n_ms(10);
	i=P2;
	i=i&0x01;					//P3.5为按键输入;ok=0x01,down=0x02,up=0x04
	if(i>0)	
	goto key_in;
key_hold:
	n_ms(10);
    i=P2;
	i=i&0x01;
	if(i<1)
	goto key_hold;
}

//延时处理按键程序********************************************************************************
//仅检测单键，等待时间有限
void key_scan1(uint dtime)						//dtime=5000>>>>time<=8s
{
	uchar i;
	uint j;
	P2=0Xff;
	j=0;
key_in:
	n_ms(10);
	i=P2;						//P2.0=ok,P2.1=down,P2.2=up
	i=i&0x01;					//P3.5为按键输入;ok=0x01,down=0x02,up=0x04
	if(i>0)
	{
		if(j<dtime)
		{
			j++;
			goto key_in;
		}
	}
key_hold:
	n_ms(10);
  	i=P2;
	i=i&0x01;
	if(i<1)
	{
		if(j<dtime)
		{
			j++;
			goto key_hold;
		}
	}
}

//延时检测按键程序********************************************************************************
//编写于2024年寒假，当时不知道IO口可以位操作，结果就照着之前的按键检测用字节操作了
//多键检测
//修改于2024年6月19日，暂未验证，如果出问题可以回旧版本复制回来
uchar key_scan2(uint dtime)
{
	uchar i,pin_address;
	uint j;
	j=0;
	pin_address=0;//扫描结果初始化
	P2=0Xff;//端口初始化

scan_start://端口扫描，在任意按键被按下前重复检测，直到延时结束
	n_ms(10);
	i=P2;
	P1=0x0c;//尝试用P1控制LED
	i=i&0x01;
	if(j>dtime)
	{
		goto scan_finish;
	}	
	if (i<1) 
		{
			pin_address=1;
			goto scan_check;
		}
	i=P2;
	i=i&0x02;	
	if (i<1) 
		{
			pin_address=2;
			goto scan_check;
		}
	i=P2;
	i=i&0x04;	
	if (i<1) 
		{
			pin_address=3;
			goto scan_check;
		}
	if (j<dtime) 
		{
			j++;
			goto scan_start;
		}
	goto scan_finish;
scan_check://检测到按键按下后确认按键松开，直到延时结束
	switch (pin_address)
	{
		case 1:
			n_ms(10);
  		i=P2;
			i=i&0x01;
			if(i<1&&j<dtime)
			{
				j++;
				goto scan_check;
			}
			goto scan_start;
			break;
		case 2:
			n_ms(10);
  		i=P2;
			i=i&0x02;
			if(i<1&&j<dtime)
			{
				j++;
				goto scan_check;
			}
			goto scan_start;
			break;
		case 3:
			n_ms(10);
  		i=P2;
			i=i&0x04;
			if(i<1&&j<dtime)
			{
				j++;
				goto scan_check;
			}
			goto scan_start;
			break;
		default://无输入状态
			goto scan_start;//检测确认完成，再次进入检测按键按下
	}
scan_finish://结束检测，返回检测结果
	P1=0x00;
	return pin_address;//无操作时为0，第0脚时为1，第1脚时为2，第2脚时为3
}

//按键检测与反应时间反馈*************************************************************************************
uint key_scan3(uint dtime)						//dtime=5000>>>>time<=8s
{
	uchar i;
	uint j;
	P2=0Xff;
	j=0;
key_in:
	n_ms(10);
	i=P2;						//P2.0=ok,P2.1=down,P2.2=up
	i=i&0x01;					//P3.5为按键输入;ok=0x01,down=0x02,up=0x04
	if(i>0)
	{
		if(j<dtime)
		{
			j++;
			goto key_in;
		}
	}
key_hold:
	n_ms(10);
  	i=P2;
	i=i&0x01;
	if(i<1)
	{
		if(j<dtime)
		{
			j++;
			goto key_hold;
		}
	}
return j%50;//返回范围缩限
}



//按键等待检测***********************************************************************
//多键检测，不限时
uint key_scan4()
{
	uchar i;
	uint j,pin_address;
	j=0;
	pin_address=0;//扫描结果初始化
	P2=0Xff;//端口初始化

scan_start://端口扫描，在任意按键被按下前重复检测，直到延时结束
	n_ms(10);
	i=P2;
	P1=0x0c;//尝试用P1控制LED
	i=i&0x01;	
	if (i<1) 
		{
			pin_address=1;
			goto scan_check;
		}
	i=P2;
	i=i&0x02;	
	if (i<1) 
		{
			pin_address=2;
			goto scan_check;
		}
	i=P2;
	i=i&0x04;	
	if (i<1) 
		{
			pin_address=3;
			goto scan_check;
		}

	j++;
	goto scan_start;
scan_check://检测到按键按下后确认按键松开，直到延时结束
	switch (pin_address)
	{
		case 1:
			n_ms(10);
  		i=P2;
			i=i&0x01;
			if(i<1)
			{
				j++;
				goto scan_check;
			}
			goto scan_finish;
			break;
		case 2:
			n_ms(10);
  		i=P2;
			i=i&0x02;
			if(i<1)
			{
				j++;
				goto scan_check;
			}
			goto scan_finish;
			break;
		case 3:
			n_ms(10);
  		i=P2;
			i=i&0x04;
			if(i<1)
			{
				j++;
				goto scan_check;
			}
			goto scan_finish;
			break;
		default:
			goto scan_start;//检测确认完成，再次进入检测按键按下
	}
scan_finish://结束检测，返回检测结果
	P1=0x00;
	return pin_address;//无操作时为0，第0脚时为1，第1脚时为2，第2脚时为3
}





// 写命令子程序*********************************************************************
//12048_7567A,实际有效位为cs1、rs、sda2和sclk2

void write_command(uchar command)  //写命令子程序
{
    uchar x;                //定义暂存器
    rs=0;                   //rs = P3^2;     //RS=0,拉低A0,表示数据为指令
    cs1=0;          		//cs1 = P3^0;    //cs1=0,拉低片选
    for(x=0;x<8;x++)        	//循环8次
    {
        sclk=0;				//sclk =P3^3;		//时钟置低
        sclk1=0;			//sclk1 =P1^0;
       	sclk2=0;			//sclk2 =P1^6;
        if((command&0x80)==0x80)
        	{
           	sda=1;          //sda =P3^4;	//取SDA，即命令4的最高位
            sda1=1;			//sda1 =P1^1;
            sda2=1;			//sda2 =P1^7;
        	}
        else
        	{
            sda=0;
        	sda1=0;
            sda2=0;
            }
        command=command<<1;        //左移一位
        {
    	sclk=1;                    //SCKL=1		//时钟置高，上升沿锁存数据
        sclk1=1;
        sclk2=1;
        }
    }
    cs1=1;                      //cs1=1		//片选置高，数据发送结束
}


// 写数据子程序*********************************************************************
void write_data(uchar dis_data)  //写数据子程序
{
    uchar x;                   //定义暂存器
    rs=1;                      //RS=1		//置高A0，表示数据为显示数据
    cs1=0;                     //cs1=0
    for(x=0;x<8;x++)          		 //循环8次
    {
        sclk=0;
        sclk1=0;
        sclk2=0;
        if ((dis_data&0x80)==0x80)                    //SCLK=0
        	{ 
			sda=1;
           	sda1=1;
           	sda2=1;
           	}
        else
        	{
			sda=0;
        	sda1=0;
            sda2=0;
            }
        dis_data=dis_data<<1;      //左移一位
        {
		sclk=1;                    //SCKL=1
        sclk1=1;
        sclk2=1;
        }
    }
    cs1=1;                      //cs1=1
}




/*
void write_command(uchar command)
{
	rs=0;
	cs1=0;
	rd=1;			//rd =P3^4;
	rw=0;			//rw =P3^3;
	data_port=command;
	rw=1;
}
void write_data(uchar disp_data)
{
	rs=1;
	cs1=0;
	rd=1;
	rw=0;
	data_port=disp_data;
	rw=1;
}
*/



// 写页面地址子程序*********************************************************************
void set_page_address(uchar x)
{
    uchar  page_temp;
    page_temp=x|0xb0;
    write_command(page_temp);
}


// 写列地址子程序*********************************************************************
void set_column_address(uchar x)
{
	uchar  column_temp;

	column_temp=x%16;		//取低位列地址
	write_command(column_temp);

	column_temp=x/16;		//取高位列地址
	column_temp=column_temp|0x10;
	write_command(column_temp);
}

/*
void prt_one_char_sub(uchar start_column,start_page,uchar *i)
{
        uchar   x,l,k;
        x=start_column;
		set_page_address(start_page);
        set_column_address(start_column);

        for(k=0;k<5;k++)
        {
        l=*i;
        write_data(*i);
        *i++;
        }

}

void disp_timer(uchar start_column,start_page,uchar second)
{
	prt_one_char_sub(start_column,start_page,uchr_Number_Set+second*5);
}

void prt_one_char(uchar start_column,start_page,uchar *char_pointer)
{
	uchar x;
      	x=*char_pointer;
        while(*char_pointer!='\0')

        {
        if(*char_pointer>=0x30&&*char_pointer<=0x39)	//数字0-9
	prt_one_char_sub(start_column,start_page,&uchr_Number_Set+(*char_pointer-0x30)*5);

        if(*char_pointer>=65&&*char_pointer<=90)	//大写字母A-Z
	prt_one_char_sub(start_column,start_page,&uchr_Capitalization_Char_Set+(*char_pointer-65)*5);

        if(*char_pointer>=97&&*char_pointer<=122)	//小写字母a-z
	prt_one_char_sub(start_column,start_page,&uchr_lowercase_Char_Set+(*char_pointer-97)*5);


        *char_pointer++;
		start_column+=6;
	}
}

void read_data_tab(uchar *pp)
{
        uchar i,x;
        for(x=0;x<10;x++)
        {
        i=*pp  ;
        *pp++;
        }
}
*/

// 写显示四边框子程序*********************************************************************
//该程序当前设定值为120*48
void lcd_disp_tab(void)
{
	uchar i,page_address_temp=0;

	set_page_address(0);
	set_column_address(0);
	
	//写顶边
	for(i=0;i<132;i++)
	{
	write_data(0x01);
	//page_address_temp++;
	}
	set_page_address(5);
	set_column_address(0);
	
	//写底边
	for(i=0;i<132;i++)
	{
		write_data(0x80);

	//page_address_temp++;
	}
	
	//写左边
	set_page_address(0);
	
	for(page_address_temp=0;page_address_temp<7;page_address_temp++)
	{
		set_column_address(0);
		write_data(0xFF);
		set_page_address(page_address_temp);

	//page_address_temp++;
	}
	
	//写右边
	set_page_address(0);
	
	for(page_address_temp=0;page_address_temp<7;page_address_temp++)
	{
		set_column_address(119);
		write_data(0xFF);
		set_page_address(page_address_temp);

	//page_address_temp++;
	}
}


// 写全屏显示点阵子程序*********************************************************************

//此部分功能实现点阵显示功能
		//a=x<<7;
		//b=x>>1;
		//x=a|b;

void lcd_disp_full(uchar x)
{
	uchar a,b,i,t,page_address_temp=0;

	for(t=0;t<9;t++)
   	{
		set_page_address(page_address_temp);
		set_column_address(0);
		for(i=0;i<64;i++)
		{
			a=x<<7;
			b=x>>1;
			x=a|b;
			write_data(x);
  			P0=x;
  		}
  		x= x>>3;
  		for(i=0;i<64;i++)
		{
			a=x<<7;
			b=x>>1;
  			write_data(x);
  			P0=x;
  		}
  		x = x<<3;
		for(i=0;i<128;i++)
		{
			a=x<<7;
			b=x>>1;
			x=a|b;

			write_data(x);
        	P0=x;
        	x= x>>3;
        	write_data(x);
        	P0=x;
        	x = x<<3;
        }
        page_address_temp++;
   }
}

// 写测试图标子程序*********************************************************************
void lcd_disp_test_icon_2(void)
{
	uchar *tab_pointer ;
	uchar i;
  	tab_pointer=&ICON_TAB;
	set_page_address(8);

	for(i=0;i<28;i++)
	{
	set_page_address(8);
	set_column_address(ICON_TAB[i]);
	write_data(0xff);
	set_column_address(ICON_TAB[i]);
	set_page_address(0);
	write_data(0xff);
	key_scan();
    }
}


// 写测试图标子程序*********************************************************************
void lcd_disp_test_icon(void)
{
	uchar i;
	set_page_address(0x08);
	set_column_address(0);
	for(i=0;i<132;i++)
	{
		write_data(0xff);
		key_scan();
    }
}


// 写单个文字子程序*********************************************************************
void disp_single_char(uchar column,page,uchar *text_pointer)
{
    uchar x,y;
    for(y=0;y<2;y++)
    {
        set_column_address(column);
        set_page_address(page);
        for(x=0;x<16;x++)
        {
			//z=*text_pointer;
			write_data(*text_pointer);
			text_pointer++;
        }
        page++;
    }
}

// 写全屏图片子程序*********************************************************************
void disp_full_picture(uchar column,page,uchar *pic_pointer)
{
    uchar x,y;
    for(y=0;y<2;y++)
    {
        set_column_address(column);
        set_page_address(page);
        for(x=0;x<56;x++)
        {
			//z=*text_pointer;
			write_data(*pic_pointer);
			pic_pointer++;
        }
        page++;
    }
}




// 写多个文字子程序*********************************************************************
//由disp_full_picture()修改而来
//使用方法为在字符点阵库中直接拼接多个单字
//column=起始写入列，page=起始写入page，*pic_pointer=点阵集名称，cha_um=拼接的字符个数，cha_width=单个字符宽度

void disp_multi_cha_16(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
{
        uchar x,y,z;
        uchar page_temp;
        page_temp=page;
        for(z=0;z<cha_num;z++)
        {
		    for(y=0;y<2;y++)
		    {
		        set_column_address(column);
		        set_page_address(page);
			    for(x=0;x<cha_width;x++)
		        {    
		        	write_data(*pic_pointer);
		        	pic_pointer++;
		        }
		        page++;
		    }
		    page=page_temp;
			column=column+cha_width;
		}
}


//任意位置写任意大小图片子程序
/*
由disp_multi_cha_16()修改而来
*/
void disp_arb_pic1(uchar column,page,uchar *pic_pointer,uchar pic_height,uchar pic_width,uchar space_content)
{
    uchar x,y;
    uchar page_temp;
    page_temp=page;
	if(space_content==0x00)
	{
		for(y=0;y<pic_height/8;y++)
		{
			set_page_address(page);
			set_column_address(column);
			for(x=0;x<pic_width;x++)
			{    
				write_data(*pic_pointer);
				pic_pointer++;
			}
			page++;
		}
		page=page_temp;
	}
	else
	{
		for(y=0;y<pic_height/8;y++)
		{
			set_page_address(page);
			set_column_address(column);
			for(x=0;x<pic_width;x++)
			{    
				write_data(~(*pic_pointer));
				pic_pointer++;
			}
			page++;
		}
		page=page_temp;		
	}
}



//任意位置写任意大小字符串子程序
//disp_arb_pic1()的修改版
void disp_arb_pic2(uchar column,page,uchar *pic_pointer,uchar pic_height,uchar pic_width,uchar space_content)
{
    uchar x,y,z;
    uchar page_temp;
    page_temp=page;
	if(space_content==0x00)
	{
		for(z=0;z<pic_width/16;z++)
		{
		for(y=0;y<pic_height/8;y++)
		{
			set_page_address(page);
			set_column_address(column);
			for(x=0;x<pic_width/16;x++)
			{    
				write_data(*pic_pointer);
				pic_pointer++;
			}
			page++;
		}
		page=page_temp;
		}
	}
	else
	{
		for(z=0;z<pic_width/16;z++)
		{
		for(y=0;y<pic_height/8;y++)
		{
			set_page_address(page);
			set_column_address(column);
			for(x=0;x<pic_width/16;x++)
			{
				write_data(~(*pic_pointer));
				pic_pointer++;
			}
			page++;
		}
		page=page_temp;
		}
	}
}




//主界面按键显示***********************************************************************************

//类型1
void disp_key_type1(uchar column,page,area_width,area_height,key_type1 ui_data)
{

}

//类型2，居中显示，选中（加框）
void disp_key_type2()
{

}



//类型3，居中显示，支持反白
//column=显示区域起始列位置，page=显示区域起始page位置，area_width=显示区域宽度，area_height=显示区域高度，ui_data=显示内容
//	结构体参考：
//	typedef struct
//	{
//		uchar writing_width;//内容宽度
//		uchar writing_height;//内容高度
//		uchar *writing_data;//内容指针
//	}
//	key_type1;
//
void disp_key_type3(uchar column,page,area_width,area_height,key_type1 ui_data,uchar space_content)
{
	uchar content_start_column;
	uchar content_start_page;

	uchar x,y,z;
	uchar page_temp;
	//uchar column_temp;
	page_temp=page;

	//运算内容写入位置
		content_start_column=column+(area_width-ui_data.writing_width)/2;
		content_start_page=page+(area_height/8-ui_data.writing_height/8)/2;

	//写入空白区
	if(content_start_page!=page)
	{
		for(y=0;z<(page_temp-content_start_page);z++)//写入顶部空白区
		{
			set_column_address(column);
			set_page_address(page);
			for(x=0;x<(column+area_width);x++)
			{
				write_data(space_content);
			}
			page++;
		}
		//page=page_temp;
		page=page+ui_data.writing_height/8;

		for(y=0;z<(page_temp-content_start_page);z++)//写入底部空白区
		{
			set_column_address(column);
			set_page_address(page);
			for(x=0;x<(column+area_width);x++)
			{
				write_data(space_content);
			}
			page++;
		}
		page=page_temp;
	}

	if(content_start_column!=column)
	{
		for(y=0;y<(ui_data.writing_height/8);y++)//写入左侧空白区
		{
			set_column_address(column);
			set_page_address(page);
			for(x=0;x<(content_start_column-column);x++)
			{
				write_data(space_content);
			}
			page++;
		}
		page=page_temp;
		
		for(y=0;y<(ui_data.writing_height/8);y++)
		{
			set_column_address(content_start_column+ui_data.writing_width);
			set_page_address(page);
			for(x=0;x<(content_start_column-column);x++)
			{    
				write_data(space_content);
			}
			page++;
		}
		page=page_temp;
	}

	//显示内容
	disp_arb_pic1(content_start_column,content_start_page,ui_data.writing_data,ui_data.writing_height,ui_data.writing_width,space_content);
}





//写单个4*4色块子程序**************************************************************************
	//disp_single_square2()的早期学习版本
	void disp_single_square1(uchar column,page,uchar data_write_in)
	{
		uchar x,y;
		set_column_address(column);
		set_page_address(page);
		for(y=0;y<1;y++)
		{
			set_column_address(column);
			set_page_address(page);
			for(x=0;x<4;x++)
			{
				write_data(data_write_in);        
			}
		}
	}


//写单个4*4色块子程序**************************************************************************
	//data_write_in = 0xf0,奇数坐标
	//data_write_in = 0x0f,偶数坐标
	void disp_single_square2(uchar head_column,uchar head_line)
	{
		uchar x,y;
		set_column_address(5*head_column-4);
		set_page_address(head_line/2-1);
		if(head_line%2 == 1)
			{
			for(y=0;y<1;y++)
				{
					set_column_address(5*head_column-4);
					set_page_address(head_line/2-1);
					for(x=0;x<4;x++)
					{
						write_data(0xf0);
					}
				}
			}
		else
			{
				for(y=0;y<1;y++)
				{
					set_column_address(5*head_column-4);
					set_page_address(head_line/2-1);
					for(x=0;x<4;x++)
					{
						write_data(0x0f);
					}
				}
			}
	}

//写单个贪吃蛇area子程序,修改自写多个文字子程序*********************************************************
	void disp_single_area(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
	{
			uchar x,y,z;
			uchar page_temp;
			page_temp=page;
			for(z=0;z<cha_num;z++)
			{
				for(y=0;y<2;y++)
				{
					set_column_address(column);
					set_page_address(page);
					for(x=0;x<cha_width;x++)
					{
						write_data(*pic_pointer);
						pic_pointer++;
					}
					page++;
				}
				page=page_temp;
				column=column+cha_width;
			}
	}


//用于解决disp_89()中switch无法嵌套的问题******************************************************
	//case0表示area为0
	void switch_case0(uchar icolumn_t,uchar typ_t)
	{
		switch(typ_t)
			{
					case 0:
						disp_single_area(5*icolumn_t+3,0,s10000,1,4);
						break;
					case 1: //001=1
						disp_single_area(5*icolumn_t+3,0,s12001,1,4);
						break;
						//(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
					case 2://010=2
						disp_single_area(5*icolumn_t+3,0,s12010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_t+3,0,s12011,1,4);
						break;
					case 4://100=4
						disp_single_area(5*icolumn_t+3,0,s12100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_t+3,0,s12101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_t+3,0,s12110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_t+3,0,s12111,1,4);
						break;
				}
	}

	//case1表示area为1
	void switch_case1(uchar icolumn_t,uchar typ_t)
	{
		switch(typ_t)
				{
					case 0:
						disp_single_area(5*icolumn_t+3,2,s00000,1,4);
						break;
					case 1: //001=1
						disp_single_area(5*icolumn_t+3,2,s34001,1,4);
						break;
					case 2://010=2
						disp_single_area(5*icolumn_t+3,2,s34010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_t+3,2,s34011,1,4);
						break;
					case 4://10=4
						disp_single_area(5*icolumn_t+3,2,s34100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_t+3,2,s34101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_t+3,2,s34110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_t+3,2,s34111,1,4);
						break;
				}
	}

	//case2表示area2
	void switch_case2(uchar icolumn_t,uchar typ_t)
	{
		switch(typ_t)
				{
					case 0:
						disp_single_area(5*icolumn_t+3,4,s00001,1,4);
						break;
					case 1: //001=1
						disp_single_area(5*icolumn_t+3,4,s56001,1,4);
						break;
					case 2://010=2
						disp_single_area(5*icolumn_t+3,4,s56010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_t+3,4,s56011,1,4);
						break;
					case 4://100=4
						disp_single_area(5*icolumn_t+3,4,s56100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_t+3,4,s56101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_t+3,4,s56110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_t+3,4,s56111,1,4);
						break;
				}
	}



//8page的9行式坐标的带边框显示功能***************************************************************
	//由于无法嵌套switch，故用函数引出第二层switch
	void disp_89(uint icolumn_temp2,uchar area,uchar typ)
	{
		switch(area)
		{
			case 0:
			{
				switch_case0(icolumn_temp2,typ);//由于不明原因无法嵌套switch，故改用子程序
				break;
				//switch_case0()的源
				/*
				switch(typ)
				{
					case 1: //001=1
						disp_single_area(5*icolumn_temp2-2,area,s12001,1,4);
						break;
						//(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
					case 2://010=2
						disp_single_area(5*icolumn_temp2-2,area,s12010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_temp2-2,area,s12011,1,4);
						break;
					case 4://100=4
						disp_single_area(5*icolumn_temp2-2,area,s12100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_temp2-2,area,s12101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_temp2-2,area,s12110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_temp2-2,area,s12111,1,4);
						break;
				}
				break;
				*/
			}
			case 1:
			{
				switch_case1(icolumn_temp2,typ);//由于不明原因无法嵌套switch，故改用子程序
				break;
				//switch_case1()的源
				/*
				switch(typ)
				{
					case 1: //001=1
						disp_single_area(5*icolumn_temp2-2,area,s34001,1,4);
						break;
					case 2://010=2
						disp_single_area(5*icolumn_temp2-2,area,s34010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_temp2-2,area,s34011,1,4);
						break;
					case 4://100=4
						disp_single_area(5*icolumn_temp2-2,area,s34100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_temp2-2,area,s34101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_temp2-2,area,s34110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_temp2-2,area,s34111,1,4);
						break;
					default:
						key_scan();
				}
				break;
				*/
			}
			case 2:
			{
				switch_case2(icolumn_temp2,typ);//由于不明原因无法嵌套switch，故改用子程序
				break;
				//switch_case2()的源
				/*
				switch(typ)
				{
					case 1: //001=1
						disp_single_area(5*icolumn_temp2-2,area,s56001,1,4);
						break;
					case 2://010=2
						disp_single_area(5*icolumn_temp2-2,area,s56010,1,4);
						break;
					case 3://011=3
						disp_single_area(5*icolumn_temp2-2,area,s56011,1,4);
						break;
					case 4://100=4
						disp_single_area(5*icolumn_temp2-2,area,s56100,1,4);
						break;
					case 5://101=5
						disp_single_area(5*icolumn_temp2-2,area,s56101,1,4);
						break;
					case 6://110=6
						disp_single_area(5*icolumn_temp2-2,area,s56110,1,4);
						break;
					case 7://111=7
						disp_single_area(5*icolumn_temp2-2,area,s56111,1,4);
						break;
				}
				break;
				*/
			}
		}
	}



//将十进制行为转化为十六进制（二进制）形式并存入行为库，每个行为占用2位******************************************
//num=行动的预期写入位置（0~length），direction=传入的十进制行为，*act_rec_adr=行为库的第一个字节地址
void act_rec_write(uchar num,uchar direction,uchar *act_rec_adr)
{
	uchar act_area=(num/4);//目标写入字节
	uchar act_steep=(num%4);//目标写入对（0~3）
	uchar act_rec_present=*(act_rec_adr+act_area);//目标字节的当前值
	uchar act_pin=0x03;//检测探针0000,0011
	uchar i;

	for(i=0;i<act_steep;i++)//将行为及检测探针对齐写入位置
	{
		direction=direction<<2;
		act_pin=act_pin<<2;
	}

	//擦除目标位置以准备写入
	act_rec_present=act_rec_present|act_pin;
	act_rec_present=~act_rec_present;
	act_rec_present=act_rec_present|act_pin;
	act_rec_present=~act_rec_present;

	//写入
	*(act_rec_adr+act_area)=act_rec_present|direction;
	
}

//读取行为库内容***********************************************************************
uchar act_rec_read(uchar num,uchar *act_rec_adr)
{
	uchar act_area=(num/4);//目标检测字节
	uchar act_steep=(num%4);//目标检测对（0~3）
	uchar act_rec_present=*(act_rec_adr+act_area);//目标字节的当前值
	uchar act_pin=0x03;//检测探针0000,0011
	uchar i;

	for(i=0;i<act_steep;i++)//将检测探针对齐检测位置
	{
		act_pin=act_pin<<2;
	}

	act_rec_present=act_rec_present&act_pin;//检测目标对
	for(i=0;i<act_steep;i++)//将将目标对转化为十进制
	{
		act_rec_present=act_rec_present>>2;
	}

	return act_rec_present;//返回行为的十进制数据
}

//旋转俄罗斯方块**************************************************************************
uchar square_rotate(square_type1 xdata *p_square,uint xdata *coord_lib)
{
	uchar i;
	uchar result;//返回旋转后碰撞的方块数
	square_type1 xdata square_temp;
	square_temp=*p_square;
	result=0;
	for(i=1;i<4;i++)
	{
		rotation_coord_90(p_square->coordx[0],p_square->coordy[0],&(p_square->coordx[i]),&(p_square->coordy[i]));
		if(pin_read_bit(*(coord_lib+p_square->coordx[i]),-1+p_square->coordy[i])||result)//旋转碰撞复位
		{
			result++;
		}
	}
	if(result)
	{
		*p_square=square_temp;
	}
	return result;
}

//刷新方块结构旋转中心，优先比较x，x相同则比较y, 均选择较大值********************************************
void square_origin_set(square_type1 xdata *p_square)
{
	uchar i;
	uchar max_n;//新旋心标号
	int difference;
	max_n=0;
	for(i=1;i<4;i++)
	{
		if(p_square->coordx[max_n]<p_square->coordx[i])//比较x值
		{
			max_n=i;
		}
		else if(p_square->coordx[max_n]==p_square->coordx[i])
		{
			if(p_square->coordy[max_n]<p_square->coordy[i])//x相同时比较y值
			{
				max_n=i;	
			}
		}
	}

	if((p_square->coordx[max_n])>(p_square->coordx[0]))//补偿重设后的旋心x坐标，防止方块一次下移多行
	{
		difference=(p_square->coordx[max_n])-(p_square->coordx[0]);
		for(i=0;i<4;i++)
		{
			p_square->coordx[i]-=difference;
		}
	}

	if((p_square->coordy[max_n])>(p_square->coordy[0]))//补偿重设后的旋心y坐标，防止方块左右移动过多，目前效果有限，还可能导致方块偏出边界
	{
		difference=(p_square->coordy[max_n])-(p_square->coordy[0]);
		if((p_square->coordy[0]-difference)>0)
		{
			for(i=0;i<4;i++)
			{
				p_square->coordy[i]-=difference;
			}			
		}
	}

	//移动旋心坐标至数组第0位
	if(max_n)
	{
		p_square->coordx[max_n]+=p_square->coordx[0];
		p_square->coordy[max_n]+=p_square->coordy[0];
		p_square->coordx[0]=(p_square->coordx[max_n])-(p_square->coordx[0]);
		p_square->coordy[0]=(p_square->coordy[max_n])-(p_square->coordy[0]);
		p_square->coordx[max_n]-=p_square->coordx[0];
		p_square->coordy[max_n]-=p_square->coordy[0];
	}

}


//俄罗斯方块边缘检测*******************************************************************************
//direciton=0下边缘，direction=1左边缘，direciton=3右边缘；注意此处方向是竖屏时的方向
uchar square_edge_detect(square_type1 xdata *p_square,uchar direction)
{
	uchar i;
	uchar max_n;//最大值序号
	max_n=0;
	switch(direction)
	{
		case 0://下边缘
			for(i=1;i<4;i++)
			{
				if((p_square->coordx[max_n])<=(p_square->coordx[i]))
				{
					max_n=i;
				}
			}
			break;
		case 1://左边缘
			for(i=1;i<4;i++)
			{
				if((p_square->coordy[max_n])<=(p_square->coordy[i]))
				{
					max_n=i;
				}
			}
			break;
		case 2://上边缘
			for(i=1;i<4;i++)
			{
				if((p_square->coordx[max_n])>=(p_square->coordx[i]))
				{
					max_n=i;
				}
			}			
		case 3://右边缘
			for(i=1;i<4;i++)
			{
				if((p_square->coordy[max_n])>=(p_square->coordy[i]))
				{
					max_n=i;
				}
			}
			break;
	}
	return max_n;
}



//移动俄罗斯方块，检测是否碰撞或超范围
//direciton=0下移，direction=1左移，direciton=3右移；注意此处方向是竖屏时的方向
uchar square_move(square_type1 xdata *p_square,uint xdata *coord_lib,uchar direction)
{
	uchar i;
	square_type1 xdata square_temp;
	uchar result;
	//uint test;
	square_temp=*p_square;
	result=0;
	switch(direction)
	{
		case 0://下移
			for(i=0;i<4;i++)
			{
				p_square->coordx[i]++;
				if((p_square->coordx[i])>22)
				{
					result++;
				}
				if(pin_read_bit(*(coord_lib+p_square->coordx[i]),-1+p_square->coordy[i])||result)//碰撞复位
				{
					result++;
				}
			}
			break;
		case 1://左移
			for(i=0;i<4;i++)
			{
				p_square->coordy[i]++;
				if((p_square->coordy[i])>9)//超范围复位
				{
					result++;
				}
				
				if(pin_read_bit(*(coord_lib+p_square->coordx[i]),-1+p_square->coordy[i]))//旋转碰撞复位
				{
					result++;
				}
			}
			break;
		case 3://右移
			for(i=0;i<4;i++)
			{
				p_square->coordy[i]--;
				if((p_square->coordy[i])==0)
				{
					result++;
				}
				if(pin_read_bit(*(coord_lib+p_square->coordx[i]),-1+p_square->coordy[i])||result)//旋转碰撞复位
				{
					result++;
				}
			}
			break;
	}
	if(result)
	{
		*p_square=square_temp;
	}
	return result;
}

//俄罗斯方块整行检测与清除
void square_wipe(uint xdata * xdata coord_lib,uint xdata * top_coordx)
{
	uint i,j;
	uint xdata coord_temp;
	uint *temp;
	for(i=*top_coordx;i<=23;i++)
	{
			coord_temp=*(coord_lib+i);
			coord_temp=~coord_temp;
			coord_temp=coord_temp>>7;//坐标组取反、移位，方便判断

			if(coord_temp==0)//判断是否行满
			{
				for(j=0;j<=(i-*top_coordx);j++)//坐标组整体下移
				{
					*(coord_lib+i-j)=*(coord_lib+i-j-1);
					pin_detect_write(i-j,*(coord_lib+i-j));
					//key_scan();
				}
				*top_coordx+=1;
			}
	}
}


/*
//修改uint型坐标集单个数据点
void coordgroup_single_change(uint *p_coordgroup,uint x,uint x_max,uint y,uchar content)
{
	uchar i;
	uint probe;
	uint temp;
	probe=0x8000;
	for(i=1;i<y;i++)
	{
		probe=probe>>1;
	}
	temp=*(p_coordgroup+y/8+(y/8)*x_max);
}
*/


//探针检测并调用显示写入（擦除）*******************************************************************
//该程序探针为3位111型探针，用于对坐标库中每个area的状态的检测
void pin_detect_write(uint icolumn_temp,uint loc1_temp)//icolumn_temp=列坐标，loc_temp=目标坐标组
{	
	uint xdata loc_point=0xE000;//初始化探针
	uint loc2_temp=loc1_temp;
	uchar i;
	for(i=0;i<3;i++)
	{
		if((loc_point&loc1_temp)>0)
		{
			loc1_temp=loc1_temp<<3*i;
			loc1_temp=loc1_temp>>13;//移动坐标列至低位
			//loc_point=loc_point>>3;
			disp_89(icolumn_temp,i,loc1_temp);
			loc1_temp=loc2_temp;
		}
		else
		{
			disp_89(icolumn_temp,i,0);
		}
		loc_point=loc_point>>3;
	}
}


//探针检测是否碰撞
//icolumn_temp=列坐标，loc1_temp=目标坐标列，iline_temp=行坐标，*act_rec_adr=行为库地址，length_temp=长度，game_status_temp=游戏状态
uchar pin_detect_crash(uint icolumn_temp,uint loc1_temp,uint iline_temp,uchar *act_rec_adr,uchar length_temp,uchar game_status_temp)
{	
	uint xdata loc_point=0x8000;//初始化探针
	uchar length_count=length_temp;//复制蛇长以用于行为读取
	uchar xdata result=1;
	uchar icolumn_temp2=icolumn_temp;
	uchar iline_temp2=iline_temp;

	uchar i;
	//检查是否超范围
	if(icolumn_temp>22 || icolumn_temp<0)
	{
		result=0;
	}
	else if(iline_temp>9 || iline_temp<1)
	{
		result=0;
	}

	//以下是基于位坐标库的检测方法，直接使用会与食物显示冲突，在v1.0中已经解决
	//现在的解决思路是在位坐标检测后进行运算检测，还需要获取行动记录进行多次运算，若运算失败，则长度+1，否则gameover
	if(result>0)
	{
		for(i=1;i<(iline_temp);i++)//移动探针
		{
			loc_point=loc_point>>1;
		}

		if((loc_point&loc1_temp)>0)//检查是否碰撞
		{

			//检查碰撞物是否为食物
			for(i=0;i<length_temp;i++)
			{
				if(game_status_temp==2)//判断是否连吃
				{
					result=0;
					break;
				}

				length_count=length_temp-i;
				switch(act_rec_read(length_count-1,act_rec_adr))
				{
					case 0:
						icolumn_temp2--;
						break;
					case 1:
						iline_temp2++;
						break;
					case 2:
						icolumn_temp2++;
						break;
					case 3:
						iline_temp2--;
						break;
				}
				if((icolumn_temp2==icolumn_temp) && (iline_temp2==iline_temp))
				{
					result=0;//检测为碰撞
					break;
				}
				else
				{
					//检测为食物
					result=2;
				}
			}

		}
		else
			result=1;
	}
	return result;
}


//坐标库写入预处理
//用于单个坐标写入时待替换位设置
uint loc_process1(uint iline_t)
{
	uint iloc=0x8000;
	uchar i;
	for(i=0;i<iline_t-1;i++)
	{
		iloc=iloc>>1;
	}
	return iloc;
}

//双字节单数据位读取器******************************************************************************************
//data=传入的待测数据，column=目标数据位置
uchar pin_read_bit(uint xdata data_temp,uint xdata column)
{
	uint xdata probe=0x8000;//初始探针
	uchar result=0;

	//移动探针至指定位
	probe=probe>>column;
	if(data_temp & probe)
	{
		result=1;
	}
	return result;
}


//超限自动归零计算器****************************************************************************
uchar num_set(char change,uchar max_num,uchar pre_num)
{
	if(change>0)
	{
		pre_num+=change;
		pre_num=pre_num%(max_num+1);
	}
	if(change<0)
	{
		if(-change<=pre_num)
		{
			pre_num+=change;
		}
		else
		{
			pre_num=-change-pre_num;
			pre_num=pre_num%(max_num+1);
		}
	}
	return pre_num;
}


//光标左移一位******************************************************************************
uchar cursor_left(cursor_type1 *cp)
{
	uchar change;
	if((cp->cursor_point)>0)
	{
		(cp->cursor_point)--;
		change=1;
	}
	else
	{
		change=0;
	}
	return change;
}

//光标右移一位********************************************************************************
uchar cursor_right(cursor_type1 *cp)
{
	uchar xdata change;
	if((cp->cursor_point)<(cp->cursor_max))
	{
		(cp->cursor_point)++;
		change=2;
	}
	else
	{
		change=0;
	}
	return change;
}

//按键显示内容修改
void key_content_change(key_type1 *key_p,uchar *content)
{
	key_p->writing_data=content;
}

//按键宽度修改
void cha_width_change(key_type1 *cha_p,uchar content)
{
	cha_p->writing_width=content;
}



//线性同余算法*******************************************************************

// 伪随机数生成器  
unsigned int prng(uint xdata prng_seed_temp)
{  
    prng_seed_temp = (1103515245 * prng_seed_temp + 12345) & 0x7FFFFFFF;
    return prng_seed_temp;
}  

uchar prng_in_range(uint xdata prng_seed_temp,uchar xdata prng_max_temp,uchar xdata prng_min_temp)
{
	uchar xdata prng_range=prng_max_temp-prng_min_temp;
	prng_seed_temp = (prng_seed_temp % prng_range)+prng_min_temp;
	return prng_seed_temp;
}

/*
线性同余算法from文心一言------------------
// 线性同余算法的参数  
#define A 1103515245  
#define C 12345  
#define M 0x7FFFFFFF // 2^31 - 1  
  
unsigned int prng_seed = 1; // 初始种子值，可以设置为任意值，最好每次程序启动时都改变  
  
// 伪随机数生成器  
unsigned int prng() {  
    prng_seed = (A * prng_seed + C) & M;  
    return prng_seed;  
}  
  
// 生成指定范围内的随机数  
unsigned int generate_random_in_range(unsigned int min, unsigned int max) 
{  
    if (max <= min) {  
        // 处理无效范围的情况  
        return min;  
    }  
      
    unsigned int range = max - min;  
    if (range >= M) {  
        // 如果范围太大，直接返回原始随机数  
        return prng();  
    }  
      
    // 生成原始随机数并缩放到指定范围  
    unsigned int raw_random = prng();  
    unsigned int scaled_random = (raw_random % range) + min;  
    return scaled_random;  
}  
  
void main() {  
    unsigned int min_value = 10; // 最小值  
    unsigned int max_value = 50; // 最大值  
      
    // 可以根据需要初始化种子，例如使用定时器值或外部输入  
    // prng_seed = ...;  
      
    while (1) {  
        // 生成指定范围内的随机数  
        unsigned int random_in_range = generate_random_in_range(min_value, max_value);  
          
        // 这里添加使用随机数的代码，例如通过串口发送出去  
        // ...  
          
        // 延时或等待下一次循环  
        // ...  
    }  
}
---------------------------------------------
*/

//贪吃蛇测试功能，用于显示当前长度，显示范围仅为0-9
void count_write(uchar length_temp)
{
	switch(length_temp%10)
	{
		case 1:
			disp_multi_cha_16(108,4,NUM_1,1,8);
			break;
		case 2:
			disp_multi_cha_16(108,4,NUM_2,1,8);
			break;
		case 3:
			disp_multi_cha_16(108,4,NUM_3,1,8);
			break;
		case 4:
			disp_multi_cha_16(108,4,NUM_4,1,8);
			break;
		case 5:
			disp_multi_cha_16(108,4,NUM_5,1,8);
			break;
		case 6:
			disp_multi_cha_16(108,4,NUM_6,1,8);
			break;
		case 7:
			disp_multi_cha_16(108,4,NUM_7,1,8);
			break;
		case 8:
			disp_multi_cha_16(108,4,NUM_8,1,8);
			break;
		case 9:
			disp_multi_cha_16(108,4,NUM_9,1,8);
			break;
		default:
			disp_multi_cha_16(108,4,NUM_0,1,8);
			break;		
	}
}


//二维坐标90度顺时针旋转功能*********************************************************************************
//后续建议再开发个任意角度的旋转功能
void rotation_coord_90(uint xdata origin_x,uint xdata origin_y,uint *target_x,uint *target_y)
{
	uint xdata target_xtemp,target_ytemp;
	
	target_xtemp=*target_x;
	target_ytemp=*target_y;

	*target_x=origin_x+target_ytemp-origin_y;
	*target_y=origin_y-target_xtemp+origin_x;
	
}




//交互界面程序******************************************************************************************

//第一层界面，主界面
//代号为1
uchar Interface_Main()
{
	uchar xdata lobby_data=0;//返回值初始化，用于返回交互层变动
	uchar xdata key_pressed;//按键操作
	uchar xdata cursor_change;
	cursor_type1 xdata cursor={0,2};//{cursor_point,cursor_max}

	//初始化key
	key_type1 xdata key_first={64,16,CHA_game_type1};
	key_type1 xdata key_second={64,16,CHA_description_type1};
	key_type1 xdata key_third={64,16,CHA_set_type1};

	lcd_disp_full(0x00);//清屏

	//初始按键显示
	disp_key_type3(1,0,70,16,key_first,0xff);
	disp_key_type3(1,2,70,16,key_second,0x00);
	disp_key_type3(1,4,70,16,key_third,0x00);

	while(lobby_data==0)
	{
		//检测按键结果
		key_pressed=key_scan4();

		if(key_pressed>1)
		{
			//刷新上次按键显示
			switch(cursor.cursor_point)
			{
				case 0:
					disp_key_type3(1,0,70,16,key_first,0x00);
					break;
				case 1:
					disp_key_type3(1,2,70,16,key_second,0x00);
					break;
				case 2:
					disp_key_type3(1,4,70,16,key_third,0x00);
					break;
			}

			//改变光标位置
			switch(key_pressed)
			{
				case 1:
					cursor_change=0;
					break;
				case 2:
					cursor_change=cursor_left(&cursor);			
					break;
				case 3:
					cursor_change=cursor_right(&cursor);			
					break;
			}

			//刷新选中按键显示
			switch(cursor.cursor_point)
			{
				case 0:
					disp_key_type3(1,0,70,16,key_first,0xff);
					break;
				case 1:
					disp_key_type3(1,2,70,16,key_second,0xff);
					break;
				case 2:
					disp_key_type3(1,4,70,16,key_third,0xff);
					break;
			}
		}

		//确定键被按下
		if(key_pressed==1)
		{
			switch(cursor.cursor_point)
			{
				case 0:
					lobby_data=2;//当前为第1层，设置进入第2层游戏选择界面
					break;
				case 1:
					lobby_data=5;//设置进入系统介绍
					break;
				case 2:
					lobby_data=0;//设置为进入系统设置
					break;
			}
		}
	}
	return lobby_data;
}




//第二层界面，游戏选择界面
//代号为2
uchar Interface_GameSelect()
{
	uchar xdata lobby_data=0;//返回值初始化，用于返回交互层变动
	uchar xdata key_pressed=0;//按键操作
	uchar xdata cursor_change;
	cursor_type1 xdata cursor={1,2};//{cursor_point,cursor_max}

	//初始化可变按键的内容空间
	uchar *key_middle_icons[2]={ICON_snake1,ICON_rus1};
	//uchar xdata *key_middle_icons_test[2]={ICON_snake1,ICON_rus1};
	uchar xdata icon_middle_num=0;
	uchar *key_middle_cha[2]={&CHA_snake1,&CHA_rusq1};
	uchar xdata cha_middle_width[2]={48,80};

	//初始化key	
	key_type1 xdata key_left={16,16,ICON_left_arrow2};
	key_type1 xdata key_right={8,16,ICON_right_arrow};
	key_type1 xdata key_middle={32,32,ICON_snake1};
	key_type1 xdata cha_middle={48,16,CHA_snake1};

	lcd_disp_full(0x00);//清屏

	//初始显示，尝试使用通用型按键图标显示，目前功能正常
	disp_key_type3(4,3,16,16,key_left,0x00);
	disp_key_type3(109,3,8,16,key_right,0x00);
	disp_key_type3(40,2,40,32,key_middle,0xff);
	disp_key_type3(40,0,40,16,cha_middle,0x00);

	while(lobby_data==0)
	{
		//检测按键结果
		key_pressed=key_scan4();

		if(key_pressed>1)
		{
			//刷新上次按键显示
			switch(cursor.cursor_point)
			{
				case 0:
					disp_key_type3(4,3,16,16,key_left,0x00);
					break;
				case 1:
					disp_key_type3(40,2,40,32,key_middle,0x00);
					break;
				case 2:
					disp_key_type3(109,3,8,16,key_right,0x00);
					break;
			}

			//改变光标位置
			switch(key_pressed)
			{
				case 1:
					cursor_change=0;
					break;
				case 2:
					cursor_change=cursor_left(&cursor);	
					break;
				case 3:
					cursor_change=cursor_right(&cursor);
					break;
			}
			//刷新选中按键显示
			switch(cursor.cursor_point)
			{
				case 0:
					disp_key_type3(4,3,16,16,key_left,0xff);
					break;
				case 1:
					disp_key_type3(40,2,40,32,key_middle,0xff);
					break;
				case 2:
					disp_key_type3(109,3,8,16,key_right,0xff);
					break;
			}
		}
		//记录按键
		if(key_pressed==1)
		{
			switch(cursor.cursor_point)
			{
				case 0://当前为第2级游戏选择界面，设置返回第1级主界面
					lobby_data=1;
					break;
				case 1://进入选中的游戏
					if(icon_middle_num==0)
					{
						lobby_data=3;//设置进入贪吃蛇
					}
					else if(icon_middle_num==1)
					{
						lobby_data=4;//设置进入俄罗斯方块
					}
					break;
				case 2://右键被按下，改变中部按键显示
					lobby_data=0;

					//改变中部按键显示
					icon_middle_num=num_set(1,1,icon_middle_num);
					key_content_change(&key_middle,key_middle_icons[icon_middle_num]);
					key_content_change(&cha_middle,key_middle_cha[icon_middle_num]);
					cha_width_change(&cha_middle,cha_middle_width[icon_middle_num]);
					disp_key_type3(20,0,80,16,cha_middle,0x00);	
					//disp_multi_cha_16(36,0,key_middle_cha[icon_middle_num],cha_middle_width[icon_middle_num],16);
					disp_key_type3(40,2,40,32,key_middle,0x00);
					//n_ms(500);
					break;
			}
		}
					//key_scan();
					//lcd_disp_full(0x00);
					//disp_multi_cha_16(36,0,key_middle_cha[icon_middle_num],3,16);
					//disp_multi_cha_16(36,0,key_middle_cha[icon_middle_num],3,16);
					//key_scan();
	}
	return lobby_data;
}

//第2层界面，系统介绍界面
//代号为5
uchar Interface_SystemDescribe()
{
	uchar xdata lobby_data=1;
	lcd_disp_full(0x00);//清屏

	//手动显示系统介绍，后续升级为多页文本显示
	disp_multi_cha_16(1,0,SYSTEN_DESCRIBE_cha1,3,16);
	disp_multi_cha_16(49,0,SYSTEN_DESCRIBE_cha2,3,8);
	disp_multi_cha_16(73,0,SYSTEN_DESCRIBE_cha3,3,16);
	disp_multi_cha_16(1,2,SYSTEN_DESCRIBE_cha4,3,8);
	disp_multi_cha_16(25,2,SYSTEN_DESCRIBE_cha5,3,16);
	disp_multi_cha_16(73,2,SYSTEN_DESCRIBE_cha6,4,8);
	disp_multi_cha_16(105,2,SYSTEN_DESCRIBE_cha7,1,16);
	disp_multi_cha_16(1,4,SYSTEN_DESCRIBE_cha8,5,16);
	disp_multi_cha_16(81,4,SYSTEN_DESCRIBE_cha9,5,8);
	key_scan();//按键后退出，上级为主界面
	return lobby_data;
}




//界面跳转系统****************************************************************************************
void Interface_Core()
{
	uchar xdata lobby_data=1;//初始界面
	uint xdata prng_seed;//随机数种子

	lcd_disp_full(0x00);//清屏
	disp_multi_cha_16(30,2,CHA_WELCOME,7,8);//显示WELCOME
	key_scan2(500);
	
	while(1)
	{
	switch(lobby_data)
	{
		case 0:
			break;
		case 1://第一层，主选择界面
			lobby_data=Interface_Main();
			break;
		case 2://第二层，进入游戏选择界面
			lobby_data=Interface_GameSelect();
			break;
		
		case 3://进入贪吃蛇游戏
			lcd_disp_full(0x00);
			lcd_disp_tab();
			disp_multi_cha_16(30,2,CHA_WELCOME,7,8);//显示WELCOME
			prng_seed=key_scan3(1000);
			lcd_disp_full(0x00);
			lcd_disp_tab();
			snake_program(prng_seed);
			lobby_data=2;
			break;

		
		case 4://进入俄罗斯方块
			lcd_disp_full(0x00);
			lcd_disp_tab();
			disp_multi_cha_16(30,2,CHA_WELCOME,7,8);//显示WELCOME
			prng_seed=key_scan3(1000);
			prng_seed=10;
			lcd_disp_full(0x00);
			lcd_disp_tab();
			rusq_program(prng_seed);
			lobby_data=2;
			break;
		case 5://进入系统介绍界面
			lobby_data=Interface_SystemDescribe();
			break;
		case 6://进入系统设置界面，待制作

			lobby_data=1;
			break;
	}
	//key_scan();
	}
	
}






//贪吃蛇主程序*******************************************************************************************

//注意显示应从物理的 第4列 开始，以保证显示左右对称

void snake_program(uint prng_seed)
{
	uchar game_status=1;//初始化游戏状态,0=结束,1=正常,2=加长
	uchar length=5;//初始化长度
	uchar game_time=0;//计步器
	uchar food_time=0;//食物计步器
	uchar generate_delay=8;//食物生成间隔

	uchar direction=0;//方向,0为右、1为上、2为左、3为下

	uchar xdata act_rec[49];//行为库

	uint xdata loc1[23];//1号坐标库
	uint loc1_temp;//位坐标列缓存,使用了地址,不建议存入xdata
	uint xdata loc_point;//位坐标探针

	uchar iline=4;//iline=1~9
	uchar icolumn=11;//icolumn=0~22,初始头坐标
	uint iline_temp,icolumn_temp;//坐标缓存

	uchar i;//循环计数器
	
	//uint prng_seed = 1; // 线性同余算法初始种子值，可以设置为任意值，最好每次程序启动时都改变,现已由按键反应时间决定
	uchar prng_max;
	uchar prng_min; //随机数范围

	for(i=0;i<23;i++)//初始化坐标集

	{
		loc1[i]=0x0000;
	}

	for(i=0;i<50;i++)//初始化行为库
	{
		act_rec[i]=0x00;
	}

	for(i=0;i<length;i++)//初始行动记录存入
	{
		act_rec_write(i,direction,&act_rec[0]);
	}
	for(i=0;i<5;i++)//初始坐标存入
	{
		loc1[icolumn-i]=0x1000;
	}
	for(i=0;i<5;i++)//初始显示
	{
		disp_89(icolumn-i,1,0x0004);//disp_89(uchar icolumn_temp2,uchar area,uint typ)
	}

	//游戏启动
	while(game_status>0)
	{

		//获取按键结果
		switch(key_scan2(240)-1)//uint key_scan2(uint dtime),返回值为1~3,1为ok键，修改传入的dtime以修改每步的间隔时间
		{
			case 1:
				if(direction>=3)
				{
					direction=0;
				}
				else
				{
					direction++;
				}
				break;
			case 2:
				if(direction<=0)
				{
					direction=4;
				}
				direction--;
				break;
			default:
				break;
		}

		//将新行动压入行为库
		if(game_status==1)//在长度增长的一个周期内不用压库，直接写入就行
		{
			for(i=0;i<length;i++)
			{
				act_rec_write(i,act_rec_read(i+1,&act_rec[0]),&act_rec[0]);
			}
		}
		act_rec_write(length-1,direction,&act_rec[0]);
		
		//覆写新的头坐标
		switch(direction)
		{
			case 0://向右
				icolumn++;
				break;
			case 1://向上
				iline--;
				break;
			case 2://向左
				icolumn--;
				break;
			case 3://向下
				iline++;
				break;
		}

		//正在将碰撞检测改为使用位存储的行为库
		//碰撞检测，该检测方法是基于位坐标库的，必须应用于坐标写入之前
		game_status=pin_detect_crash(icolumn,loc1[icolumn],iline,&act_rec[0],length,game_status);//0=over,1=start,2=increase

		//写入新坐标
		loc1_temp = loc1[icolumn];//复制旧坐标
		loc1[icolumn]=loc1_temp | loc_process1(iline);//位坐标列插入
		
		//探针检测与显示刷新
		pin_detect_write(icolumn,loc1[icolumn]);

		if(game_status==2)
		{
			length++;
		}

		//探针检测与显示擦除-------------------------

		if(game_status==1)//length增加的一个周期内不用擦除
		{
		//1、运算尾坐标,正尝试转化为函数
		iline_temp=iline;
		icolumn_temp=icolumn;
		for(i=0;(i<length);i++)
		{
			switch(act_rec_read(i,&act_rec[0]))
			{
				case 0:
					icolumn_temp--;
					break;
				case 1:
					iline_temp++;
					break;
				case 2:
					icolumn_temp++;
					break;
				case 3:
					iline_temp--;
					break;
				default:
					break;
			}
		}

		//2、覆写新坐标
		loc1_temp = ~loc1[icolumn_temp];
		loc1_temp = loc1_temp | loc_process1(iline_temp);
		loc1[icolumn_temp]=~loc1_temp;
		
		//3、开始擦除（写入）
		//探针检测与显示刷新
		pin_detect_write(icolumn_temp,loc1[icolumn_temp]);
		//key_scan();
		}

		game_time++;//计步器+1
		food_time++;

		//食物生成*********************************************
		if(food_time==generate_delay)
		{
			//生成x坐标
			prng_max=23,prng_min=1;
			prng_seed=prng(prng_seed);
			icolumn_temp=prng_in_range(prng_seed,prng_max,prng_min);

			//生成y坐标
			prng_max=9,prng_min=1;
			prng_seed=prng(prng_seed);
			iline_temp=prng_in_range(prng_seed,prng_max,prng_min);

			//写入新坐标
			loc1_temp = loc1[icolumn_temp];//复制旧坐标
			loc1[icolumn_temp]=loc1_temp | loc_process1(iline_temp);//位坐标列插入

			//探针检测与显示刷新
			pin_detect_write(icolumn_temp,loc1[icolumn_temp]);
			food_time=0;
		}

		//count_write(length);
	}
	disp_multi_cha_16(25,1,GAME_OVER,9,8);//结束标语
	key_scan();


}




//俄罗斯方块主程序****************************************************************************************

void rusq_program(uint prng_seed)
{
	uchar i;//循环计数器
	uint xdata temp;
	uint *test;

	uchar xdata game_status=1;//初始化游戏状态,0=结束,1=正常
	uchar xdata block_type=1;//初始化方块类型,1~7
	uchar crash_n;
	uchar key;
	uint xdata top_coordx;//顶层x值

	uint xdata iline_temp;
	uint xdata icolumn_temp;//坐标缓存

	uint xdata loc2[23];//2号坐标库,从高位开始存储
	uint loc2_temp;//位坐标列缓存,使用了地址,不建议存入xdata

	//旋心确定原则：首先icolumn最大，随后iline最小
	uint xdata iline=4;//iline=1~9，旋转中心行坐标
	uint xdata icolumn=3;//icolumn=0~22,旋转中心列坐标

	//uint prng_seed = 1; // 线性同余算法初始种子值
	uchar xdata prng_max=7;
	uchar xdata prng_min=1; //随机数范围

	square_type1 xdata square1={0,0,0,0,0,0,0,0};//定义方块结构体
	top_coordx=23;

	block_type=prng_in_range(prng_seed,prng_max,prng_min);//初始化方块类型
	prng_seed=prng(prng_seed);//刷新随机数

	for(i=0;i<23;i++)//初始化坐标集
	{
		loc2[i]=0x0000;
	}
	//loc2[23]=0xffff;

	while(game_status)
	{
		//刷新随机方块结构
		prng_seed=prng(prng_seed);
		block_type=prng_in_range(prng_seed,prng_max,prng_min);

		//设定方块结构
		switch(block_type)
		{
			case 1://条形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn;
				square1.coordx[3]=icolumn;				

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+2;
				square1.coordy[3]=iline+3;
				break;
			case 2://正方形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn-1;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+1;
				square1.coordy[3]=iline;
				break;
			case 3://左折形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn-1;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline;
				square1.coordy[3]=iline-1;
				break;
			case 4://右折形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn-1;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+1;
				square1.coordy[3]=iline+2;
				break;
			case 5://左L形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+2;
				square1.coordy[3]=iline+2;
				break;
			case 6://右L形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+2;
				square1.coordy[3]=iline;				
				break;
			case 7://倒T形
				square1.coordx[0]=icolumn;
				square1.coordx[1]=icolumn;
				square1.coordx[2]=icolumn;
				square1.coordx[3]=icolumn-1;

				square1.coordy[0]=iline;
				square1.coordy[1]=iline+1;
				square1.coordy[2]=iline+2;
				square1.coordy[3]=iline+1;
				break;
		}

		//初始显示
		for(i=0;i<4;i++)
		{
			//1、写入坐标集
			loc2_temp = loc2[square1.coordx[i]];//复制旧坐标
			loc2[square1.coordx[i]]=loc2_temp | loc_process1(square1.coordy[i]);//位坐标列插入
		
			//2、开始显示
			//探针检测与显示刷新
			pin_detect_write(square1.coordx[i],loc2[square1.coordx[i]]);
		}

		while(block_type)//借方块类型作为方块运动循环控制
		{
			//检测按键
			key=key_scan2(200);//uint key_scan2(uint dtime),返回值为1~3,1为ok键，2为左键，3为右键，修改传入的dtime以修改每步的间隔时间
			
			//清除当前显示，准备更新坐标并显示------------------
			for(i=0;i<4;i++)
			{
				//1、擦除旧坐标
				loc2_temp = ~loc2[square1.coordx[i]];
				loc2_temp = loc2_temp | loc_process1(square1.coordy[i]);
				loc2[square1.coordx[i]]=~loc2_temp;

				//2、开始擦除
				//探针检测与显示刷新
				pin_detect_write(square1.coordx[i],loc2[square1.coordx[i]]);
			}

			//移动方块，修改各坐标----------------------------
			switch(key)
			{
				case 1://旋转
					if(square_rotate(&square1,&loc2)==0)
					{
						//刷新旋心
						square_origin_set(&square1);
					}
					break;
				case 2://左移
					crash_n=square_move(&square1,&loc2,1);
					break;
				case 3://右移
					crash_n=square_move(&square1,&loc2,3);
					break;
			}
			crash_n=square_move(&square1,&loc2,0);//下移

			
			//刷新（恢复）显示----------------------------------------
			for(i=0;i<4;i++)
			{
				//1、写入坐标集
				loc2_temp = loc2[square1.coordx[i]];//复制旧坐标
				loc2[square1.coordx[i]]=loc2_temp | loc_process1(square1.coordy[i]);//位坐标列插入
			
				//2、开始写入
				//探针检测与显示刷新
				pin_detect_write(square1.coordx[i],loc2[square1.coordx[i]]);
			}
			
			//检测终止移动，记录终止时的特征坐标，随后检测是否清行--------------------
			if(crash_n)
			{
				//记录顶层
				temp=square1.coordx[square_edge_detect(&square1,2)];
				if(temp<top_coordx)
				{
					top_coordx=temp;
				}
				test=loc2;

				square_wipe(loc2,&top_coordx);//扫描坐标集，清除满行
				//key_scan();
				
				//判定是否超限
				if(top_coordx<4)
				{
					game_status=0;
				}

				block_type=0;
				crash_n=0;

			}
		}
	}
	disp_multi_cha_16(25,1,GAME_OVER,9,8);//结束标语
	key_scan();
}







// 主程序****************************************************************************
//key_scan1()单键延时检测
//key_scan2()三键延时检测
void main(void)
{
	uchar test;//测试功能开关
	//uint g;
	//uint i_temp;
	//uint g_temp;
	uint prng_seed;
	while(1)
	{
		lcd_rest();
   		lcd_init();

		lcd_disp_full(0x00);
		//delay_n_100ms(20);
		lcd_disp_tab();//写四边框

		//write_command(0xff);

			//俄罗斯方块测试区
			//prng_seed=10;
			//lcd_disp_full(0x00);
			//lcd_disp_tab();
			//rusq_program(prng_seed);

	test=0;
	if(test)//早期功能测试区，为防止部分函数被识别为中断函数故不使用注释方法
	{
lcd_disp_full(0xaa);
delay_n_100ms(20);
key_scan();
lcd_disp_full(0x00);
delay_n_100ms(20);
key_scan();
lcd_disp_test_icon_2();
key_scan();
lcd_disp_full(0xFF);
delay_n_100ms(20);
key_scan();
lcd_disp_full(0x00);

delay_n_100ms(10);
key_scan();


//显示多个单字功能测试

lcd_disp_full(0x00);
disp_single_char(1,1,CHA_CU_1);
disp_single_char(17,1,CHA_CU_2);
disp_single_char(33,1,CHA_CU_3);
key_scan();
disp_89(11,1,0004);
key_scan();
lcd_disp_full(0x00);
disp_multi_cha_16(1,1,CHA_MU_3,9,8);
key_scan();
lcd_disp_test_icon();
disp_single_square1();
disp_single_square2();
pin_read_bit();


//按键检测功能验证，4*4方格显示功能验证
/*
g_temp=key_scan2(2500);
switch (g_temp)
{
	case 1:
		for(g=8;g<=16;g++)
		{
			//lcd_disp_full(0x00);
			disp_single_square2(g,2);
			key_scan1(800);
		}
		break;
	case 2:
		for(g=8;g<=16;g++)
		{
			//lcd_disp_full(0x00);
			disp_single_square2(g,3);
			key_scan1(800);
		}
		break;
	case 3:
		for(g=8;g<=16;g++)
		{
			//lcd_disp_full(0x00);
			disp_single_square2(g,4);
			key_scan1(800);
		}
		break;
	default:
		for(g=8;g<=16;g++)
		{
			//lcd_disp_full(0x00);
			disp_single_square2(g,5);
			key_scan1(800);
		}
		
}

*/		
	}


/*
if (g_temp==2) 
{
	for(g=8;g<=16;g++)
		{
			lcd_disp_full(0x00);
			disp_single_square2(g,2);
			key_scan1(500);
		}


}
*/
//lcd_disp_full(0x00);
//key_scan();


		disp_multi_cha_16(1,1,CHA_MU_3,9,8);
		//key_scan1(1000);
		//prng_seed=3;
		
		//prng_seed=key_scan3(1000);
		
		key_scan2(1000);
		Interface_Core();
		lcd_disp_full(0x00);
		lcd_disp_tab();
		//snake_program(prng_seed);
		key_scan();



//lcd_disp_full(0xaa);//全屏点阵
//key_scan();
//
//    	  write_command(0x81);
//        write_command(33);
//        key_scan();
//
//
//       write_command(0x81);
//        write_command(48);
//        key_scan();


//    	   	disp_full_picture(4,0,PIC_12864_meinu);
//    	   	delay_n_s(1);
//
//    	   	disp_full_picture(4,0,PIC_12864_TV);
//    	   	delay_n_s(1);
//
//    	   	disp_full_picture(4,0,PIC_12864_TIMER);
//    	   	delay_n_s(1);
//
//    	   	disp_full_picture(4,0,PIC_12864_MIQI2);
//    	   	delay_n_s(1);
//
//    	   	disp_full_picture(4,0,PIC_12864_CHINA_MAP);
//   	      delay_n_s(1);
	}
}
