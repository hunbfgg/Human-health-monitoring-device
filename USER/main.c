#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"
#include "mpu6050.h"  
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 
#include "ds18b20.h"
#include "timer.h"
#include "sim900a.h"
#include "beep.h"
#include "usart2.h"
#include "text.h"

u8 BPM;
short temperature;
/************************************************
 STM32开发板
 人体健康监测系统
///////////////////////////////////////////////
所用硬件模块及其对应引脚：
温度传感器 DS18B20 DQ->PG11  紫色
										VCC->5V  褐色
										GND->GND 棕色
心率传感器  pulse sensor  S->PA0  灰色
													VCC->3.3V  黑色
													GND->GND  褐色
加速度传感器 mpu6050  SCL->PB10  蓝色
											SDA->PB11  白色
											VCC->5V  橘色
											GND->GND 蓝色
短信收发模块  sim900a 5VR->PA2  灰色
											5VT->PA3	黄色
											GND->GND	蓝色
											VCC_mcu->5V(可不接)
	另外还需要5V 2A电源供电
32单片机 stm32f103zet6
					5V 1A 供电
///////////////////////////////////////////////
************************************************/


//串口1发送1个字符 
//c:要发送的字符
void usart1_send_char(u8 c)
{   	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //循环发送,直到发送完毕   
	USART_SendData(USART1,c);  
} 

 	
 int main(void)
 {	 
	 
	float pitch,roll,yaw; 		//欧拉角
	short aacx,aacy,aacz;			//加速度传感器原始数据
	short gyrox,gyroy,gyroz;	//陀螺仪原始数据
	short temp,temp_1,temp_2;	//温度及角度值	
 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口1初始化为115200
	usart2_init(115200);	//串口2初始化为115200
	delay_init();					//延时初始化 
	
	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();						//初始化按键
	LCD_Init();			   		//初始化LCD  
	MPU_Init();						//初始化MPU6050
	Adc_Init();
	BEEP_Init();
	TIM3_Int_Init(19,7199);//0.1ms，10Khz的计数频率，计数到20为20ms
 	POINT_COLOR=BLUE;	 
	LCD_Clear (WHITE);
	while(DS18B20_Init())		//DS18B20初始化	
	{
		LCD_ShowString(30,130,200,16,16,"DS18B20 Error");
		delay_ms(200);
		LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
	}			
	 
	while(mpu_dmp_init()) 	//DMA初始化
 	{
		LCD_ShowString(30,130,200,16,16,"MPU6050 Error");
		delay_ms(200);
		LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
	}  
	
	LCD_Clear (WHITE);
	xianshi();              //显示汉字
	LCD_ShowString(50,40,200,320,16,":     .     C");
	LCD_ShowString(50,60,200,320,16,":           bpm");
	TIM_Cmd(TIM3, ENABLE);  //使能TIM3
	BEEP=0;   							//关闭蜂鸣器
 	while(1)
	{ 			 
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
		{ 
			temp=MPU_Get_Temperature();	//得到温度值
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//得到加速度传感器数据
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//得到陀螺仪数据
				
			temp_1=roll*10;
			
			if(temp_1<0)
			{
				temp_1=-temp_1;		//转为正数
			}
			temp_2=yaw*10;
			if(temp_2<0)
			{
				temp_2=-temp_2;		//转为正数
			} 
			if(temp_1>170&&temp_2>170)   //通过角度是否大于170来判断是否摔倒
			{
				TIM_Cmd(TIM3, DISABLE);  //使能TIM3
				BEEP=1;
				LED1=0;
				sim900a_sms_sendchinese();  //发送短信
				TIM_Cmd(TIM3, ENABLE);  //使能TIM3
				LED1=1;
			}
			else	BEEP=0;
		}
		if(temperature/10>35)   //通过温度来判断系统是否在被使用
		{
			if(BPM<60||BPM>100)   //心率正常与否判断
				BEEP=1;  
			else
				BEEP=0;
		}
		else
		 BEEP=0;
	
	} 	
}
 


