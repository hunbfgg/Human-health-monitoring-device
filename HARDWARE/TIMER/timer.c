#include "timer.h"
#include "led.h"
#include "usart.h"
#include "lcd.h"
#include "ds18b20.h"

// these variables are volatile because they are used during the interrupt service routine!
//int BPM;                   			 // used to hold the pulse rate
int Signal;               			 // holds the incoming raw data
int IBI = 600;            			 // holds the time between beats, must be seeded! 
unsigned char Pulse = false;     // true when pulse wave is high, false when it's low
unsigned char QS = false;        // becomes true when Arduoino finds a beat.
int rate[10];                    // array to hold last ten IBI values
unsigned long sampleCounter = 0; // used to determine pulse timing
unsigned long lastBeatTime = 0;  // used to find IBI
int P =512;                      // used to find peak in pulse wave, seeded
int T = 512;                     // used to find trough in pulse wave, seeded
int thresh = 512;                // used to find instant moment of heart beat, seeded
int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
unsigned char firstBeat = true;  // used to seed rate array so we startup with reasonable BPM
unsigned char secondBeat = false;// used to seed rate array so we startup with reaso

/*
 * 函数名：TIM3_Int_Init
 * 描述  ：配置TIM3
 * 输入  ：arr, psc
 * 输出  ：无
 * 调用  ：外部调用
 */
void TIM3_Int_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
  /****使能定时器时钟****/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	/****初始化时钟，配置ARR,PSC****/
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
	/****开启定时器中断****/
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3的更新中断,允许更新中断
	/****配置NVIC****/
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
	/****使能定时器****/
	TIM_Cmd(TIM3, DISABLE);  //使能TIMx					 
}

//定时器3中断服务函数
 /*该源码由开源硬件提供*/
void TIM3_IRQHandler(void)
{
	uint16_t runningTotal=0;
	uint8_t i;
	uint16_t Num;
 
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
	{

		//读取到的值右移2位，12位-->10位
//		Signal = ADC_GetConversionValue(ADC1)/4;     // read the Pulse Senso
		Signal=Get_Adc_Average(ADC_Channel_1,1)>>2; 

		sampleCounter += 2;                          // keep track of the time in mS with this variable
		Num = sampleCounter - lastBeatTime;          // monitor the time since the last beat to avoid noise	
		
		//发现脉冲波的波峰和波谷
		//  find the peak and trough of the pulse wave
		if(Signal < thresh && Num > (IBI/5)*3){   // avoid dichrotic noise by waiting 3/5 of last IBI
			if (Signal < T){                        // T is the trough
				T = Signal;                           // keep track of lowest point in pulse wave 
			}
		}

		if(Signal > thresh && Signal > P){        // thresh condition helps avoid noise
			P = Signal;                             // P is the peak
		}                                         // keep track of highest point in pulse wave
  //开始寻找心跳
	//当脉冲来临的时候，signal的值会上升
	//  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (Num > 250){                                 // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (Num > (IBI/5)*3) )
			{
		
      Pulse = true;                               // set the Pulse flag when we think there is a pulse
      LED0=0; 
      IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for(i=0; i<=9; i++){                 // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;                     //rate[0]-rate[9]的值都一样吗？？？？
        }
      }
			
      if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        return;                              // IBI value is unreliable so discard it
      }   

     // keep a running total of the last 10 IBI values
     // runningTotal = 0;                  		// clear the runningTotal variable    

      for(i=0; i<=8; i++){                    // shift data in the rate array
        rate[i] = rate[i+1];                  // and drop the oldest IBI value 
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal/2.8;               // how many beats can fit into a minute? that's BPM!
			if(temperature/10>35)
				LCD_ShowNum(70,60,BPM,3,16);
			else
				LCD_Fill(70,60,120,90,WHITE);
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

	//脉冲开始下降
  if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
    LED0=1; 															 //灯灭
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  //没有检测到脉冲，设置默认值
	if (Num > 2500)
	{                         // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    P = 512;                               // set P default
    T = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
  }
		if(Signal<600)
		{
			temperature=DS18B20_Get_Temp();	  		//获取温度值
			if(temperature<0)
			{
				LCD_ShowChar(60+40,150,'-',16,0);			//显示负号
				temperature=-temperature;					//转为正数
			}else ;
			LCD_ShowChar(60+40,150,' ',16,0);			//去掉负号
			LCD_ShowNum(70,40,temperature/10,2,16);	//显示正数部分	    
			LCD_ShowNum(105,40,temperature%10,1,16);	//显示小数部分 		   		
		}
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIM3更新中断标志 
}
