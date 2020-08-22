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

#define swap16(x) (x&0XFF)<<8|(x&0XFF00)>>8		//高低字节交换宏定义

#define	SPI_FLASH_CS PAout(2)  	//选中FLASH	

#define W25X_ReadData			0x03 

u8 b=1;

//PA2和PA3 IO口方向设置函数
//因为USART2也用到PA2，PA3，和SPI FLASH/SD卡的CS有冲突，故需要设置方向
//mode：0,SPI FLASH/SD卡工作
//      1,USART2工作
void SPI_USART_IO_SET(u8 mode)
{  
	if(mode)//设置为USART 工作模式
	{
		GPIOA->CRL&=0XFFFF00FF; 
		GPIOA->CRL|=0X00008B00;//PA2.3 复用输出+上拉输入 
		GPIOA->ODR|=3<<2;		
	}else
	{ 
		GPIOA->CRL&=0XFFFF00FF; 
		GPIOA->CRL|=0X00003300;//PA2.3 推挽输出 
		GPIOA->ODR|=3<<2;		
	}
}

//SPIx 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPIx_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 
	while((SPI1->SR&1<<1)==0)//等待发送区空	
	{
		retry++;
		if(retry>200)return 0;
	}			  
	SPI1->DR=TxData;	 	  //发送一个byte 
	retry=0;
	while((SPI1->SR&1<<0)==0) //等待接收完一个byte  
	{
		retry++;
		if(retry>200)return 0;
	}	  						    
	return SPI1->DR;          //返回收到的数据				    
}


//读取SPI FLASH  
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
void SPI_Flash_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	u16 i;    	
	SPI_USART_IO_SET(0);//SPI 工作
	SPI_FLASH_CS=0;                            //使能器件   
    SPIx_ReadWriteByte(W25X_ReadData);         //发送读取命令   
    SPIx_ReadWriteByte((u8)((ReadAddr)>>16));  //发送24bit地址    
    SPIx_ReadWriteByte((u8)((ReadAddr)>>8));   
    SPIx_ReadWriteByte((u8)ReadAddr);   
    for(i=0;i<NumByteToRead;i++)
	{ 
        pBuffer[i]=SPIx_ReadWriteByte(0XFF);   //循环读数  
    }
	SPI_FLASH_CS=1;                            //取消片选     	      
	SPI_USART_IO_SET(1);//USART2 工作
}  



__packed typedef struct 
{
	u8 fontok;				//字库存在标志，0XAA，字库正常；其他，字库不存在
	u32 ugbkaddr; 			//unigbk的地址
	u32 ugbksize;			//unigbk的大小	 
	u32 f12addr;			//gbk12地址	
	u32 gbk12size;			//gbk12的大小	 
	u32 f16addr;			//gbk16地址
	u32 gkb16size;			//gbk16的大小	 
}_font_info;	


_font_info ftinfo;

//将1个16进制数字转换为字符
//hex:16进制数字,0~15;
//返回值:字符
u8 sim900a_hex2chr(u8 hex)
{
	if(hex<=9)return hex+'0';
	if(hex>=10&&hex<=15)return (hex-10+'A'); 
	return '0';
}


//将1个字符转换为16进制数字
//chr:字符,0~9/A~F/a~F
//返回值:chr对应的16进制数值
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
						  
	if (src < 0x80)c = src;//ASCII,直接不用转换.
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
		hi=ftinfo.ugbksize/2;//对半开.
		hi =hi / 4 - 1;
		li = 0;
		for (n = 16; n; n--)
		{
			i = li + (hi - li) / 2;	
			SPI_Flash_Read((u8*)&t,ftinfo.ugbkaddr+i*4+gbk2uni_offset,4);//读出4个字节  
			if (src == t[0]) break;
			if (src > t[0])li = i;  
			else hi = i;    
		}
		c = n ? t[1] : 0;  	    
	}
	return c;
}		   


//unicode gbk 转换函数
//src:输入字符串
//dst:输出(uni2gbk时为gbk内码,gbk2uni时,为unicode字符串)
//mode:0,unicode到gbk转换;
//     1,gbk到unicode转换;
void sim900a_unigbk_exchange(u8 *src,u8 *dst,u8 mode)
{
	u16 temp; 
	u8 buf[2];
	if(mode)//gbk 2 unicode
	{
		while(*src!=0)
		{
			if(*src<0X81)	//非汉字
			{
				temp=(u16)ff_convert((WCHAR)*src,1);
				src++;
			}else 			//汉字,占2个字节
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
	*dst=0;//添加结束符
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

//发送一条纯中文短信
void sim900a_sms_sendchinese()
{	
	Usart_SendString(USART2,"AT\r\n");  //检测模块是否正常运行
	delay_ms(1000);
	delay_ms(1000);
	Usart_SendString(USART2,"AT+CSQ\r\n");  //检测当前信号
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
	Usart_SendString(USART2,"AT+CMGS=\"00310038003500380030003500320031003600310034\"\r\n");  //设置要发送短信的手机号码

	delay_ms(1000);
	delay_ms(1000);
	
	Usart_SendString(USART2,"80014EBA64545012FF0C901F53BB003100320033");  //短信发送的内容
	delay_ms(1000);
	delay_ms(1000);
	
	USART2->DR =0x1A;
	delay_ms(1000);
	

}				



