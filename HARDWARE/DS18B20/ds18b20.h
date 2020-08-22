#ifndef __DS18B20_H
#define __DS18B20_H 
#include "sys.h"   

//DS18B20驱动代码	   


//IO方向设置
#define DS18B20_IO_IN()  {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=8<<12;}
#define DS18B20_IO_OUT() {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=3<<12;}
////IO操作函数											   
#define	DS18B20_DQ_OUT PGout(11) //数据端口	PA0 
#define	DS18B20_DQ_IN  PGin(11)  //数据端口	PA0 
   	
u8 DS18B20_Init(void);//初始化DS18B20
short DS18B20_Get_Temp(void);//获取温度
void DS18B20_Start(void);//开始温度转换
void DS18B20_Write_Byte(u8 dat);//写入一个字节
u8 DS18B20_Read_Byte(void);//读出一个字节
u8 DS18B20_Read_Bit(void);//读出一个位
u8 DS18B20_Check(void);//检测是否存在DS18B20
void DS18B20_Rst(void);//复位DS18B20    
#endif















