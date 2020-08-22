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
 * ��������TIM3_Int_Init
 * ����  ������TIM3
 * ����  ��arr, psc
 * ���  ����
 * ����  ���ⲿ����
 */
void TIM3_Int_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
  /****ʹ�ܶ�ʱ��ʱ��****/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
	/****��ʼ��ʱ�ӣ�����ARR,PSC****/
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	/****������ʱ���ж�****/
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ĸ����ж�,��������ж�
	/****����NVIC****/
	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���
	/****ʹ�ܶ�ʱ��****/
	TIM_Cmd(TIM3, DISABLE);  //ʹ��TIMx					 
}

//��ʱ��3�жϷ�����
 /*��Դ���ɿ�ԴӲ���ṩ*/
void TIM3_IRQHandler(void)
{
	uint16_t runningTotal=0;
	uint8_t i;
	uint16_t Num;
 
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
	{

		//��ȡ����ֵ����2λ��12λ-->10λ
//		Signal = ADC_GetConversionValue(ADC1)/4;     // read the Pulse Senso
		Signal=Get_Adc_Average(ADC_Channel_1,1)>>2; 

		sampleCounter += 2;                          // keep track of the time in mS with this variable
		Num = sampleCounter - lastBeatTime;          // monitor the time since the last beat to avoid noise	
		
		//�������岨�Ĳ���Ͳ���
		//  find the peak and trough of the pulse wave
		if(Signal < thresh && Num > (IBI/5)*3){   // avoid dichrotic noise by waiting 3/5 of last IBI
			if (Signal < T){                        // T is the trough
				T = Signal;                           // keep track of lowest point in pulse wave 
			}
		}

		if(Signal > thresh && Signal > P){        // thresh condition helps avoid noise
			P = Signal;                             // P is the peak
		}                                         // keep track of highest point in pulse wave
  //��ʼѰ������
	//���������ٵ�ʱ��signal��ֵ������
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
          rate[i] = IBI;                     //rate[0]-rate[9]��ֵ��һ���𣿣�����
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

	//���忪ʼ�½�
  if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
    LED0=1; 															 //����
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  //û�м�⵽���壬����Ĭ��ֵ
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
			temperature=DS18B20_Get_Temp();	  		//��ȡ�¶�ֵ
			if(temperature<0)
			{
				LCD_ShowChar(60+40,150,'-',16,0);			//��ʾ����
				temperature=-temperature;					//תΪ����
			}else ;
			LCD_ShowChar(60+40,150,' ',16,0);			//ȥ������
			LCD_ShowNum(70,40,temperature/10,2,16);	//��ʾ��������	    
			LCD_ShowNum(105,40,temperature%10,1,16);	//��ʾС������ 		   		
		}
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���TIM3�����жϱ�־ 
}
