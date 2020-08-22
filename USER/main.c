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
 STM32������
 ���彡�����ϵͳ
///////////////////////////////////////////////
����Ӳ��ģ�鼰���Ӧ���ţ�
�¶ȴ����� DS18B20 DQ->PG11  ��ɫ
										VCC->5V  ��ɫ
										GND->GND ��ɫ
���ʴ�����  pulse sensor  S->PA0  ��ɫ
													VCC->3.3V  ��ɫ
													GND->GND  ��ɫ
���ٶȴ����� mpu6050  SCL->PB10  ��ɫ
											SDA->PB11  ��ɫ
											VCC->5V  ��ɫ
											GND->GND ��ɫ
�����շ�ģ��  sim900a 5VR->PA2  ��ɫ
											5VT->PA3	��ɫ
											GND->GND	��ɫ
											VCC_mcu->5V(�ɲ���)
	���⻹��Ҫ5V 2A��Դ����
32��Ƭ�� stm32f103zet6
					5V 1A ����
///////////////////////////////////////////////
************************************************/


//����1����1���ַ� 
//c:Ҫ���͵��ַ�
void usart1_send_char(u8 c)
{   	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //ѭ������,ֱ���������   
	USART_SendData(USART1,c);  
} 

 	
 int main(void)
 {	 
	 
	float pitch,roll,yaw; 		//ŷ����
	short aacx,aacy,aacz;			//���ٶȴ�����ԭʼ����
	short gyrox,gyroy,gyroz;	//������ԭʼ����
	short temp,temp_1,temp_2;	//�¶ȼ��Ƕ�ֵ	
 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	 //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//����1��ʼ��Ϊ115200
	usart2_init(115200);	//����2��ʼ��Ϊ115200
	delay_init();					//��ʱ��ʼ�� 
	
	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();						//��ʼ������
	LCD_Init();			   		//��ʼ��LCD  
	MPU_Init();						//��ʼ��MPU6050
	Adc_Init();
	BEEP_Init();
	TIM3_Int_Init(19,7199);//0.1ms��10Khz�ļ���Ƶ�ʣ�������20Ϊ20ms
 	POINT_COLOR=BLUE;	 
	LCD_Clear (WHITE);
	while(DS18B20_Init())		//DS18B20��ʼ��	
	{
		LCD_ShowString(30,130,200,16,16,"DS18B20 Error");
		delay_ms(200);
		LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
	}			
	 
	while(mpu_dmp_init()) 	//DMA��ʼ��
 	{
		LCD_ShowString(30,130,200,16,16,"MPU6050 Error");
		delay_ms(200);
		LCD_Fill(30,130,239,130+16,WHITE);
 		delay_ms(200);
	}  
	
	LCD_Clear (WHITE);
	xianshi();              //��ʾ����
	LCD_ShowString(50,40,200,320,16,":     .     C");
	LCD_ShowString(50,60,200,320,16,":           bpm");
	TIM_Cmd(TIM3, ENABLE);  //ʹ��TIM3
	BEEP=0;   							//�رշ�����
 	while(1)
	{ 			 
		if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
		{ 
			temp=MPU_Get_Temperature();	//�õ��¶�ֵ
			MPU_Get_Accelerometer(&aacx,&aacy,&aacz);	//�õ����ٶȴ���������
			MPU_Get_Gyroscope(&gyrox,&gyroy,&gyroz);	//�õ�����������
				
			temp_1=roll*10;
			
			if(temp_1<0)
			{
				temp_1=-temp_1;		//תΪ����
			}
			temp_2=yaw*10;
			if(temp_2<0)
			{
				temp_2=-temp_2;		//תΪ����
			} 
			if(temp_1>170&&temp_2>170)   //ͨ���Ƕ��Ƿ����170���ж��Ƿ�ˤ��
			{
				TIM_Cmd(TIM3, DISABLE);  //ʹ��TIM3
				BEEP=1;
				LED1=0;
				sim900a_sms_sendchinese();  //���Ͷ���
				TIM_Cmd(TIM3, ENABLE);  //ʹ��TIM3
				LED1=1;
			}
			else	BEEP=0;
		}
		if(temperature/10>35)   //ͨ���¶����ж�ϵͳ�Ƿ��ڱ�ʹ��
		{
			if(BPM<60||BPM>100)   //������������ж�
				BEEP=1;  
			else
				BEEP=0;
		}
		else
		 BEEP=0;
	
	} 	
}
 


