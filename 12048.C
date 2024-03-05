


#include <reg51.h>
#define  uint  unsigned int
#define  uchar unsigned char
#include "chinese_cha.h"
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



void lcd_init(void);
void lcd_rest(void);

void n_ms(uchar x);
void delay_n_s(uchar x);
void delay_n_100ms(uchar x);			//延时N-S子程序

void key_scan();
void key_scan1(uint dtime);//可修改等待的按键
uint key_scan2(uint dtime);//限时按键检测功能，使用对P2完整设定实现，建议开发对P2位检测的方法

void write_command(uchar command);
void write_data(uchar dis_data);
void set_page_address(uchar x);
void set_column_address(uchar x);

//这4个函数无实际功能，推测为早期开发使用
void prt_one_char_sub(uchar start_column,start_page,uchar *i);
void disp_timer(uchar start_column,start_page,uchar second);
void prt_one_char(uchar start_column,start_page,uchar *char_pointer);
void read_data_tab(uchar *pp);

void lcd_disp_tab(void);//写四边框，需手动设定边界
void lcd_disp_full(uchar x);//写点阵，实际常用于清屏
void lcd_disp_test_icon_2();
void lcd_disp_test_icon(void);

void disp_single_char(uchar column,page,uchar *text_pointer);//写单字
void disp_full_picture(uchar column,page,uchar *pic_pointer);//写全屏图片
void disp_full_picture_9616(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width);//写96*16的图片

void disp_single_square1(uchar column,page,uchar data_write_in);//写4*4方格（早期开发版本）
void disp_single_square2(uchar head_column,uchar head_line);//写4*4方格，可自动识别上半page或下半page
	//贪吃蛇写入4*4方格，已disp_89()配合
	void disp_single_area(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width);

	//对disp_single_area的控制
	void switch_case0(uchar icolumn_t,uchar typ_t);
	void switch_case1(uchar icolumn_t,uchar typ_t);
	void switch_case2(uchar icolumn_t,uchar typ_t);
	void disp_89(uchar icolumn_temp2,uchar area,uchar typ);

void pin_detect_write(uchar icolumn_temp,uint *loc_d);//探针检测与坐标修改功能
uint loc_process1(uchar iline_t);//目标修改坐标的预处理
void snake_program();//贪吃蛇主程序











//LCD初始化子程序*********************************************************************
void lcd_init(void)
{
        write_command(0xe2);//软复位
        n_ms(5);
        write_command(0x2f);//电源开
        write_command(0xaf);//显示开
        write_command(0x23);//粗条对比度，0x21-0x27
        write_command(0x81);//双字节指令，微调对比度，
        write_command(45);       //38=9.0v  30=8.5v  //对比度调节，0-63
        write_command(0xc8);	 //上下,c0,c8
        write_command(0xa0);	 //左右,a0,a1

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
void delay_n_s(uchar x)			//延时N-S子程序
{
        uint i;
        uchar y;
        for(y=0;y<x;y++)
        {
        	for(i=0;i<33000;i++);
        }
}

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

void key_scan(void)						//按键处理
{
	uchar	i,h;
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

//编写于2024年寒假，当时不知道IO口可以位操作，结果就照着之前的按键检测用字节操作了，难受
uint key_scan2(uint dtime)
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
		default:
			goto scan_start;//检测确认完成，再次进入检测按键按下
	}
scan_finish://结束检测，返回检测结果
	P1=0x00;
	return pin_address;//无操作时为0，第0脚时为1，第1脚时为2，第2脚时为3
}




// 写命令子程序*********************************************************************
void write_command(uchar command)  //写命令子程序
{
    uchar x;                //定义暂存器
    rs=0;                   //rs = P3^2;     //RS=0
    cs1=0;          		//cs1 = P3^0;    //cs1=0
    for(x=0;x<8;x++)        	//循环8次
    {
        sclk=0;				//sclk =P3^3;
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
    	sclk=1;                    //SCKL=1
        sclk1=1;
        sclk2=1;
        }
    }
    cs1=1;                      //cs1=1
}


// 写数据子程序*********************************************************************
void write_data(uchar dis_data)  //写数据子程序
{
    uchar x;                   //定义暂存器
    rs=1;                      //RS=1
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
	uchar		x,k;
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





// 写显示四边框子程序*********************************************************************
//该程序当前设定值为120*48
void lcd_disp_tab(void)
{
	uchar	i,page_address_temp=0;

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
	uchar	a,b,i,t,page_address_temp=0;

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
void lcd_disp_test_icon_2()
{
	uchar *tab_pointer ;

	uchar	temp,i;
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
	uchar	i;

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
        uchar x,y,z;
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
        uchar x,y,z;
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
//使用方法为在字符点阵库中直接拼接多个单字
//column=起始写入列，page=起始写入page，*pic_pointer=点阵集名称，cha_um=拼接的字符个数，cha_width=单个字符宽度

void disp_full_picture_9616(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
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




//写单个4*4色块子程序**************************************************************************
//disp_single_square()的早期学习版本

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

//写单个贪吃蛇area子程序,修改自写多个文字子程序
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
					disp_single_area(5*icolumn_t-2,0,s10000,1,4);
					break;
				case 1: //001=1
					disp_single_area(5*icolumn_t-2,0,s12001,1,4);
					break;
					//(uchar column,page,uchar *pic_pointer,uchar cha_num,uchar cha_width)
				case 2://010=2
					disp_single_area(5*icolumn_t-2,0,s12010,1,4);
					break;
				case 3://011=3
					disp_single_area(5*icolumn_t-2,0,s12011,1,4);
					break;
				case 4://100=4
					disp_single_area(5*icolumn_t-2,0,s12100,1,4);
					break;
				case 5://101=5
					disp_single_area(5*icolumn_t-2,0,s12101,1,4);
					break;
				case 6://110=6
					disp_single_area(5*icolumn_t-2,0,s12110,1,4);
					break;
				case 7://111=7
					disp_single_area(5*icolumn_t-2,0,s12111,1,4);
					break;
			}
}

//case1表示area为1
void switch_case1(uchar icolumn_t,uchar typ_t)
{
	switch(typ_t)
			{
				case 0:
					disp_single_area(5*icolumn_t-2,2,s00000,1,4);
					break;
				case 1: //001=1
					disp_single_area(5*icolumn_t-2,2,s34001,1,4);
					break;
				case 2://010=2
					disp_single_area(5*icolumn_t-2,2,s34010,1,4);
					break;
				case 3://011=3
					disp_single_area(5*icolumn_t-2,2,s34011,1,4);
					break;
				case 4://10=4
					disp_single_area(5*icolumn_t-2,2,s34100,1,4);
					break;
				case 5://101=5
					disp_single_area(5*icolumn_t-2,2,s34101,1,4);
					break;
				case 6://110=6
					disp_single_area(5*icolumn_t-2,2,s34110,1,4);
					break;
				case 7://111=7
					disp_single_area(5*icolumn_t-2,2,s34111,1,4);
					break;
			}
}

//case2表示area2
void switch_case2(uchar icolumn_t,uchar typ_t)
{
	switch(typ_t)
			{
				case 0:
					disp_single_area(5*icolumn_t-2,4,s00001,1,4);
					break;
				case 1: //001=1
					disp_single_area(5*icolumn_t-2,4,s56001,1,4);
					break;
				case 2://010=2
					disp_single_area(5*icolumn_t-2,4,s56010,1,4);
					break;
				case 3://011=3
					disp_single_area(5*icolumn_t-2,4,s56011,1,4);
					break;
				case 4://100=4
					disp_single_area(5*icolumn_t-2,4,s56100,1,4);
					break;
				case 5://101=5
					disp_single_area(5*icolumn_t-2,4,s56101,1,4);
					break;
				case 6://110=6
					disp_single_area(5*icolumn_t-2,4,s56110,1,4);
					break;
				case 7://111=7
					disp_single_area(5*icolumn_t-2,4,s56111,1,4);
					break;
			}
}





//8page的9行式坐标带边框显示功能***************************************************************
//由于无法嵌套switch，故用函数引出第二层switch

void disp_89(uchar icolumn_temp2,uchar area,uchar typ)
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


//探针检测并调用显示写入（擦除）*******************************************************************
//该程序探针为3位111型探针，用于对坐标库中每个area的状态的检测

void pin_detect_write(uchar icolumn_temp,uint *loc_d)//icolumn为传入列位置，loc_d为传入坐标列
{	
	uint loc_point=0xE000;//初始化探针
	uint loc_point_low=*loc_d;//复制坐标列
	uchar i;
	for(i=0;i<3;i++)
	{
		if((loc_point&loc_point_low)>0)
		{
			loc_point_low=loc_point_low<<3*i;
			loc_point_low=loc_point_low>>13;//移动坐标列至低位
			//loc_point=loc_point>>3;
			disp_89(icolumn_temp,i,loc_point_low);
			loc_point_low=*loc_d;
		}
		else
		{
			disp_89(icolumn_temp,i,0);
		}
		loc_point=loc_point>>3;
	}
}


//坐标库写入预处理
//用于单个坐标写入时待替换位设置
uint loc_process1(uchar iline_t)
{
	uint iloc=0x8000;
	uchar i;
	for(i=0;i<iline_t-1;i++)
	{
		iloc=iloc>>1;
	}
	return iloc;
}






//贪吃蛇主程序****************************

//注意显示应从物理的 第4列 开始，以保证显示左右对称

void snake_program()
{
	
	uchar length=3;//初始化长度,1字节
	
	uchar direction=1;//方向,1为右、2为上、3为左、4为下，1字节

	uchar xdata act_rec[20];//行为库，20字节，可尝试使用xdata
	uchar rec_temp;//行为缓存，1字节
		
	uint xdata loc1[23];//1号坐标库，46字节，可尝试使用xdata
	uint loc1_temp;//坐标缓存，2字节
	uint loc_point=0xE000;//坐标探针1110000000000000，2字节
	uint loc_point_low;//低位探针结果，2字节

	uchar iline=4;//iline=1~9
	uchar icolumn=11;//icolumn=0~22,初始头坐标，2字节
	uchar iline_temp,icolumn_temp;//坐标缓存，2字节

	uchar i;//循环暂存器，1字节，目前已使用80字节
	
	for(i=0;i<23;i++)//初始化坐标库
	{
		loc1[i]=0x0000;
	}

	for(i=0;i<20;i++)//初始化行为库
	{
		act_rec[i]=0;
	}

	for(i=0;i<length;i++)//初始行动记录存入
	{
		act_rec[i]=direction;
	}
	for(i=0;i<3;i++)//初始坐标存入
	{
		loc1[icolumn-i]=0x1000;
	}
	for(i=0;i<3;i++)//初始显示
	{
		disp_89(icolumn-i,1,0x0004);//disp_89(uchar icolumn_temp2,uchar area,uint typ)
	}


	while(1)
	{

	//获取按键结果
	switch(key_scan2(1000)-1)//uint key_scan2(uint dtime),返回值为1~3,1为ok键
	{
		case 1:
			if(direction>=4)
			{
				direction=0;
			}
			direction++;
			break;
		case 2:
			if(direction<=1)
			{
				direction=5;
			}
			direction--;
			break;
		default:
			break;

	}

	//将新行动压入行为库
	for(i=0;i<length;i++)
	{
		act_rec[i]=act_rec[i+1];
	}
	act_rec[length-1]=direction;

	//覆写新的头坐标
	switch(direction)
	{
		case 1://向右
			icolumn++;
			break;
		case 2://向上
			iline--;
			break;
		case 3://向左
			icolumn--;
			break;
		case 4://向下
			iline++;
			break;
	}


	
	
	
	//写入新坐标
	loc1_temp = loc1[icolumn];
	loc1[icolumn]=loc1_temp | loc_process1(iline);
	
	pin_detect_write(icolumn,&loc1[icolumn]);



	//探针检测与显示写入,void pin_detect_write(uint *loc_d)的源
	
	/*
	loc_point=0xE000;
	for(i=0;i<3;i++)
	{
		if(loc_point&loc1[icolumn]>0)
		{
			loc_point_low=loc_point;
				loc_point_low=loc_point_low>>13-3*i;
			disp_89(i,loc_point_low);
		}
		loc_point=loc_point>>3;
	}
	*/


	//探针检测与显示擦除（写入）
	//1、运算尾坐标

	iline_temp=iline;
	icolumn_temp=icolumn;
	for(i=0;i<20 && act_rec[i]!=0;i++)
	{
		switch(act_rec[i])
		{
			case 1:
				icolumn_temp--;
				break;
			case 2:
				iline_temp++;
				break;
			case 3:
				icolumn_temp++;
				break;
			case 4:
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

	pin_detect_write(icolumn_temp,&loc1[icolumn_temp]);
	//key_scan();

	}



	//loc1_temp = loc1[icolumn_process(icolumn)];
	//loc1[icolumn_process(icolumn)]=loc_process2(iline_process(iline));//擦除旧坐标

}






// 主程序****************************************************************************
//key_scan1()单键延时检测
//key_scan2()三键延时检测
void main(void)
{
	uint g;
	uint i_temp,g_temp;
	while(1)
	{
		lcd_rest();
   		lcd_init();

		lcd_disp_full(0x00);
		delay_n_100ms(20);
		lcd_disp_tab();//写四边框



		key_scan1(500);


//lcd_disp_full(0xaa);
//delay_n_100ms(20);
////key_scan();
//lcd_disp_full(0x00);
//delay_n_100ms(20);
//	//key_scan();
////lcd_disp_test_icon_2();
////	key_scan();
//lcd_disp_full(0xFF);
//delay_n_100ms(20);
// // key_scan();
////lcd_disp_full(0x00);
//

//delay_n_100ms(10);
//		key_scan();


//显示多个单字功能测试

//lcd_disp_full(0x00);
//disp_single_char(1,1,CHA_CU_1);
//disp_single_char(17,1,CHA_CU_2);
//disp_single_char(33,1,CHA_CU_3);
//key_scan();
//disp_89(11,1,0004);
//key_scan();
//lcd_disp_full(0x00);
//disp_full_picture_9616(1,1,CHA_MU_3,9,8);
//key_scan();

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



disp_full_picture_9616(1,1,CHA_MU_3,9,8);

key_scan1(1000);
lcd_disp_full(0x00);
lcd_disp_tab();
snake_program();

		//snake_program();



//lcd_disp_full(0xaa);
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
