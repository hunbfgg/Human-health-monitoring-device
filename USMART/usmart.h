#ifndef __USMART_H
#define __USMART_H	  		  
#include "usmart_str.h"
 
#define MAX_FNAME_LEN 		30	//函数名最大长度，应该设置为不小于最长函数名的长度。											   
#define MAX_PARM 			10	//最大为10个参数 ,修改此参数,必须修改usmart_exe与之对应.
#define PARM_LEN 			200	//所有参数之和的长度不超过PARM_LEN个字节,注意串口接收部分要与之对应(不小于PARM_LEN)


#define USMART_ENTIMX_SCAN 	1	//使用TIM的定时中断来扫描SCAN函数,如果设置为0,需要自己实现隔一段时间扫描一次scan函数.
								//注意:如果要用runtime统计功能,必须设置USMART_ENTIMX_SCAN为1!!!!
								
#define USMART_USE_HELP		1	//使用帮助，该值设为0，可以节省近700个字节，但是将导致无法显示帮助信息。
#define USMART_USE_WRFUNS	1	//使用读写函数,使能这里,可以读取任何地址的值,还可以写寄存器的值.
///////////////////////////////////////////////END///////////////////////////////////////////////////////////

#define USMART_OK 			0  //无错误
#define USMART_FUNCERR 		1  //函数错误
#define USMART_PARMERR 		2  //参数错误
#define USMART_PARMOVER 	3  //参数溢出
#define USMART_NOFUNCFIND 	4  //未找到匹配函数

#define SP_TYPE_DEC      	0  //10进制参数显示
#define SP_TYPE_HEX       	1  //16进制参数显示


 //函数名列表	 
struct _m_usmart_nametab
{
	void* func;			//函数指针
	const u8* name;		//函数名(查找串)	 
};
//usmart控制管理器
struct _m_usmart_dev
{
	struct _m_usmart_nametab *funs;	//函数名指针

	void (*init)(u8);				//初始化
	u8 (*cmd_rec)(u8*str);			//识别函数名及参数
	void (*exe)(void); 				//执行 
	void (*scan)(void);             //扫描
	u8 fnum; 				  		//函数数量
	u8 pnum;                        //参数数量
	u8 id;							//函数id
	u8 sptype;						//参数显示类型(非字符串参数):0,10进制;1,16进制;
	u16 parmtype;					//参数的类型
	u8  plentbl[MAX_PARM];  		//每个参数的长度暂存表
	u8  parm[PARM_LEN];  			//函数的参数
	u8 runtimeflag;					//0,不统计函数执行时间;1,统计函数执行时间,注意:此功能必须在USMART_ENTIMX_SCAN使能的时候,才有用
	u32 runtime;					//运行时间,单位:0.1ms,最大延时时间为定时器CNT值的2倍*0.1ms
};
extern struct _m_usmart_nametab usmart_nametab[];	//在usmart_config.c里面定义
extern struct _m_usmart_dev usmart_dev;				//在usmart_config.c里面定义


void usmart_init(u8 sysclk);//初始化
u8 usmart_cmd_rec(u8*str);	//识别
void usmart_exe(void);		//执行
void usmart_scan(void);     //扫描
u32 read_addr(u32 addr);	//读取指定地址的值
void write_addr(u32 addr,u32 val);//在指定地址写入指定的值
u32 usmart_get_runtime(void);	//获取运行时间
void usmart_reset_runtime(void);//复位运行时间

#endif






























