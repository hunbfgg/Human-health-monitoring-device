#ifndef __SIM900A_H__
#define __SIM900A_H__	 
#include "sys.h"



void sim900a_send_cmd(u8 *cmd,u8 flag);
void sim900a_sms_sendenglish(u8 *message);
void sim900a_sms_sendchinese(void );

#endif





