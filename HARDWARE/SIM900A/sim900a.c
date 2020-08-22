#include "sim900a.h"
#include "usart.h"		
#include "delay.h"
#include "string.h"
#include "lcd.h"
#include "stdio.h"
#include "usart2.h"

typedef unsigned short	WCHAR;
typedef unsigned int	UINT;
typedef unsigned char uchar;

#define swap16(x) (x&0XFF)<<8|(x&0XFF00)>>8		//�ߵ��ֽڽ����궨��

#define	SPI_FLASH_CS PAout(2)  	//ѡ��FLASH	

#define W25X_ReadData			0x03 

u8 b=1;

//PA2��PA3 IO�ڷ������ú���
//��ΪUSART2Ҳ�õ�PA2��PA3����SPI FLASH/SD����CS�г�ͻ������Ҫ���÷���
//mode��0,SPI FLASH/SD������
//      1,USART2����
void SPI_USART_IO_SET(u8 mode)
{  
	if(mode)//����ΪUSART ����ģʽ
	{
		GPIOA->CRL&=0XFFFF00FF; 
		GPIOA->CRL|=0X00008B00;//PA2.3 �������+�������� 
		GPIOA->ODR|=3<<2;		
	}else
	{ 
		GPIOA->CRL&=0XFFFF00FF; 
		GPIOA->CRL|=0X00003300;//PA2.3 ������� 
		GPIOA->ODR|=3<<2;		
	}
}

//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPIx_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 
	while((SPI1->SR&1<<1)==0)//�ȴ���������	
	{
		retry++;
		if(retry>200)return 0;
	}			  
	SPI1->DR=TxData;	 	  //����һ��byte 
	retry=0;
	while((SPI1->SR&1<<0)==0) //�ȴ�������һ��byte  
	{
		retry++;
		if(retry>200)return 0;
	}	  						    
	return SPI1->DR;          //�����յ�������				    
}


//��ȡSPI FLASH  
//��ָ����ַ��ʼ��ȡָ�����ȵ�����
//pBuffer:���ݴ洢��
//ReadAddr:��ʼ��ȡ�ĵ�ַ(24bit)
//NumByteToRead:Ҫ��ȡ���ֽ���(���65535)
void SPI_Flash_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	u16 i;    	
	SPI_USART_IO_SET(0);//SPI ����
	SPI_FLASH_CS=0;                            //ʹ������   
    SPIx_ReadWriteByte(W25X_ReadData);         //���Ͷ�ȡ����   
    SPIx_ReadWriteByte((u8)((ReadAddr)>>16));  //����24bit��ַ    
    SPIx_ReadWriteByte((u8)((ReadAddr)>>8));   
    SPIx_ReadWriteByte((u8)ReadAddr);   
    for(i=0;i<NumByteToRead;i++)
	{ 
        pBuffer[i]=SPIx_ReadWriteByte(0XFF);   //ѭ������  
    }
	SPI_FLASH_CS=1;                            //ȡ��Ƭѡ     	      
	SPI_USART_IO_SET(1);//USART2 ����
}  



__packed typedef struct 
{
	u8 fontok;				//�ֿ���ڱ�־��0XAA���ֿ��������������ֿⲻ����
	u32 ugbkaddr; 			//unigbk�ĵ�ַ
	u32 ugbksize;			//unigbk�Ĵ�С	 
	u32 f12addr;			//gbk12��ַ	
	u32 gbk12size;			//gbk12�Ĵ�С	 
	u32 f16addr;			//gbk16��ַ
	u32 gkb16size;			//gbk16�Ĵ�С	 
}_font_info;	


_font_info ftinfo;

//��1��16��������ת��Ϊ�ַ�
//hex:16��������,0~15;
//����ֵ:�ַ�
u8 sim900a_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}


//��1���ַ�ת��Ϊ16��������
//chr:�ַ�,0~9/A~F/a~F
//����ֵ:chr��Ӧ��16������ֵ
u8 sim900a_chr2hex(u8 chr)
{
	if(chr>='0'&&chr<='9')return chr-'0';
	if(chr>='A'&&chr<='F')return (chr-'A'+10);
	if(chr>='a'&&chr<='f')return (chr-'a'+10); 
	return 0;
}

WCHAR ff_convert (	/* Converted code, 0 means conversion error */
	WCHAR	src,	/* Character code to be converted */
	UINT	dir		/* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
)
{
	WCHAR t[2];
	WCHAR c;
	u32 i, li, hi;
	u16 n;			 
	u32 gbk2uni_offset=0;		  
						  
	if (src < 0x80)c = src;//ASCII,ֱ�Ӳ���ת��.
	else 
	{
 		if(dir)	//GBK 2 UNICODE
		{
			gbk2uni_offset=ftinfo.ugbksize/2;	 
		}else	//UNICODE 2 GBK  
		{   
			gbk2uni_offset=0;	
		}    
		/* Unicode to OEMCP */
		hi=ftinfo.ugbksize/2;//�԰뿪.
		hi =hi / 4 - 1;
		li = 0;
		for (n = 16; n; n--)
		{
			i = li + (hi - li) / 2;	
			SPI_Flash_Read((u8*)&t,ftinfo.ugbkaddr+i*4+gbk2uni_offset,4);//����4���ֽ�  
			if (src == t[0]) break;
			if (src > t[0])li = i;  
			else hi = i;    
		}
		c = n ? t[1] : 0;  	    
	}
	return c;
}		   


//unicode gbk ת������
//src:�����ַ���
//dst:���(uni2gbkʱΪgbk����,gbk2uniʱ,Ϊunicode�ַ���)
//mode:0,unicode��gbkת��;
//     1,gbk��unicodeת��;
void sim900a_unigbk_exchange(u8 *src,u8 *dst,u8 mode)
{
	u16 temp; 
	u8 buf[2];
	if(mode)//gbk 2 unicode
	{
		while(*src!=0)
		{
			if(*src<0X81)	//�Ǻ���
			{
				temp=(u16)ff_convert((WCHAR)*src,1);
				src++;
			}else 			//����,ռ2���ֽ�
			{
				buf[1]=*src++;
				buf[0]=*src++; 
				temp=(u16)ff_convert((WCHAR)*(u16*)buf,1); 
			}
			*dst++=sim900a_hex2chr((temp>>12)&0X0F);
			*dst++=sim900a_hex2chr((temp>>8)&0X0F);
			*dst++=sim900a_hex2chr((temp>>4)&0X0F);
			*dst++=sim900a_hex2chr(temp&0X0F);
		}
	}else	//unicode 2 gbk
	{ 
		while(*src!=0)
		{
			buf[1]=sim900a_chr2hex(*src++)*16;
			buf[1]+=sim900a_chr2hex(*src++);
			buf[0]=sim900a_chr2hex(*src++)*16;
			buf[0]+=sim900a_chr2hex(*src++);
 			temp=(u16)ff_convert((WCHAR)*(u16*)buf,0);
			if(temp<0X80){*dst=temp;dst++;}
			else {*(u16*)dst=swap16(temp);dst+=2;}
		} 
	}
	*dst=0;//��ӽ�����
} 

void sim900a_send_cmd(u8 *cmd,u8 flag)
{
	while(*cmd!='\0')	
	{ 		
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC )==RESET);			
		USART_SendData(USART3,*cmd);		
		cmd++;	
		delay_ms(10);
	}
	if(flag==1)
	{
		USART3->DR =0x0D;
		USART3->DR =0x0A;
		
	}
}

void Usart_SendString(USART_TypeDef * USARTx,u8 *s)
{	
	while(*s!='\0')	
	{ 		
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC )==RESET);			
		USART_SendData(USARTx,*s);		
		s++;	
	}
}

//����һ�������Ķ���
void sim900a_sms_sendchinese()
{	
	Usart_SendString(USART2,"AT\r\n");  //���ģ���Ƿ���������
	delay_ms(1000);
	delay_ms(1000);
	Usart_SendString(USART2,"AT+CSQ\r\n");  //��⵱ǰ�ź�
	delay_ms(1000);
	delay_ms(1000);
	Usart_SendString(USART2,"AT+CMGF=1\r\n");
	delay_ms(1000);
	delay_ms(1000);
	Usart_SendString(USART2,"AT+CSCS=\"UCS2\"\r\n");  
	delay_ms(1000);
	delay_ms(1000);
	
	Usart_SendString(USART2,"AT+CSMP=17,167,2,25\r\n");
	delay_ms(1000);
	delay_ms(1000);
	Usart_SendString(USART2,"AT+CMGS=\"00310038003500380030003500320031003600310034\"\r\n");  //����Ҫ���Ͷ��ŵ��ֻ�����

	delay_ms(1000);
	delay_ms(1000);
	
	Usart_SendString(USART2,"80014EBA64545012FF0C901F53BB003100320033");  //���ŷ��͵�����
	delay_ms(1000);
	delay_ms(1000);
	
	USART2->DR =0x1A;
	delay_ms(1000);
	

}				



