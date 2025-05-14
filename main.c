#include	"config.h"
#include	"STC8G_H_Delay.h"
#include	"STC8G_H_NVIC.h"
#include	"STC8G_H_Switch.h"
#include "STC8G_H_EEPROM.h"
#include "STC8H_PWM.h"
#include	"STC8G_H_ADC.h"
#include "stc32_stc8_usb.h"
#include	"QC.h"
 

#define 	addr_pwm_fre							0x0000
#define 	addr_start_temp						0x0002
#define 	addr_start_speed					0x0003
#define 	addr_max_temp							0x0004
#define 	addr_max_speed						0x0005
#define 	addr_speed_levels     		0x0006
#define 	addr_led									0x0010
#define 	addr_QC_charging_value		0x0011


#define 	LED_PIN  P23
#define 	KEY_PIN  P32
#define 	FAN1_PWM_PIN  P20
#define 	FAN2_PWM_PIN  P21
#define 	FAN_TYPE_PIN  P03



char *USER_DEVICEDESC = NULL;
char *USER_PRODUCTDESC = NULL;
char *USER_STCISPCMD = "@STCISP#";
	

u8 fan_type = 2;
float temperature = 0;
u8 fan_speed_pwm = 0;
u16 pwm_fre = 25000;
u8 led = 1;
char start_temp = 40; 	
u8 start_speed = 60; 
char max_temp = 70; 	
u8 max_speed = 100;
u8 speed_levels[12]={60, 80, 100};
u8 fan_control_mode = 0;
float QC_charging_value = 0;
bit QC_charging_en = 0;
bit key_pressed = 0;
float input_voltage = 0; 

void SaveData(void);
void ReadData(void);
void SetFanSpeed(u8 speed);
u16 StringToU16(char *str); 
void isUsbOutReady(void);



void	GPIO_config(bit qc)
{	
	if(qc)
	{ 
		P1M0 = 0x00; P1M1 = 0xff; 
		P3M0 = 0x00; P3M1 = 0xfb; 
		P4M0 = 0x00; P4M1 = 0xff; 
		P5M0 = 0x00; P5M1 = 0xff; 
		P6M0 = 0x00; P6M1 = 0xff; 
		P7M0 = 0x00; P7M1 = 0xff; 
	}
	
	else
	{
		setDM_Voltage(0);
		setDP_Voltage(0);
		delay_ms(20);
		P0M0 = 0x00; P0M1 = 0xf7; 
		P1M0 = 0x00; P1M1 = 0xff; 
		P2M0 = 0x0b; P2M1 = 0xf4;
		P3M0 = 0x00; P3M1 = 0xfb; 
		P4M0 = 0x00; P4M1 = 0xff; 
		P5M0 = 0x00; P5M1 = 0xff; 
		P6M0 = 0x00; P6M1 = 0xff; 
		P7M0 = 0x00; P7M1 = 0xff; 
				
		IRC48MCR = 0x80;
		while (!(IRC48MCR & 0x01));
		USBCLK = 0x00;
		USBCON = 0x90;
		usb_init();                                     //USB CDC 接口配置
		IE2 |= 0x80;                                    //使能USB中断
		EA = 1;
		while (DeviceState != DEVSTATE_CONFIGURED);     //等待USB完成配置
	}
}
 

void checkFanType(void)
{
	FAN_TYPE_PIN = 1;
	_nop_(); 
	_nop_();  
	_nop_(); 
	_nop_(); 
	if(FAN_TYPE_PIN)
	{
		fan_type = 2;
	}
	else
	{
		fan_type = 4;
	}
	P0M0 &= ~0x08; P0M1 |= 0x08; 
}

void	ADC_config(void)
{
	ADC_InitTypeDef		ADC_InitStructure;		//结构定义

	ADC_InitStructure.ADC_SMPduty   = 31;		//ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
	ADC_InitStructure.ADC_CsSetup   = 0;		//ADC 通道选择时间控制 0(默认),1
	ADC_InitStructure.ADC_CsHold    = 1;		//ADC 通道选择保持时间控制 0,1(默认),2,3
	ADC_InitStructure.ADC_Speed     = ADC_SPEED_2X16T;		//设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
	ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED;	//ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
	ADC_Inilize(&ADC_InitStructure);		//初始化
	ADC_PowerControl(ENABLE);				//ADC电源开关, ENABLE或DISABLE
	NVIC_ADC_Init(DISABLE,Priority_0);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
}



void	PWM_config(void)
{
	PWMx_InitDefine		PWMx_InitStructure;
	PWMx_InitStructure.PWM_Mode    =	CCMRn_PWM_MODE1;	//模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
	PWMx_InitStructure.PWM_Duty    = 0;	//PWM占空比时间, 0~Period
	if(fan_type == 2)
	{
		PWMx_InitStructure.PWM_EnoSelect   = ENO5P;	//输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
		PWM_Configuration(PWM5, &PWMx_InitStructure);			//初始化PWM,  PWMA,PWMB
	}
	else
	{
		PWMx_InitStructure.PWM_EnoSelect   = ENO6P;	//输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
		PWM_Configuration(PWM6, &PWMx_InitStructure);			//初始化PWM,  PWMA,PWMB
	}
	PWMx_InitStructure.PWM_Period = (u16)(MAIN_Fosc/pwm_fre-1);						//周期时间,   0~65535
	PWMx_InitStructure.PWM_DeadTime = 0;					//死区发生器设置, 0~255
	PWMx_InitStructure.PWM_MainOutEnable= ENABLE ;			//主输出使能, ENABLE,DISABLE
	PWMx_InitStructure.PWM_CEN_Enable   = ENABLE;			//使能计数器, ENABLE,DISABLE
	PWM_Configuration(PWMB, &PWMx_InitStructure);			//初始化PWM通用寄存器,  PWMA,PWMB

	PWM5_SW(PWM5_SW_P20);			
	PWM6_SW(PWM6_SW_P21);			

	PWMB_IER = 0;
	IP2H &= ~PPWMBH; 
	IP2 &= ~PPWMB;
}


void main(void)
{ 
	EAXSFR();
	FAN1_PWM_PIN = 0;
	FAN2_PWM_PIN = 0;
	ADC_config();
	ReadData();
	input_voltage = ReadInputVoltage();
	if(QC_charging_value >= 6 && input_voltage < 6 && input_voltage > 4.5)
	{
		QC_charging_en = QCCharge(QC_charging_value);
		if(QC_charging_en)
		{
			GPIO_config(1);
		}
		else
		{
			GPIO_config(0);
		}
		
	}
	else
	{
		GPIO_config(0);
	}
	

	LED_PIN = 0;
	checkFanType();	
	KEY_PIN=1; 
	IT0 = 1;
	EX0 = 1;
	EA = 1;

	PWM_config();	
	while (1)
	{ 
		temperature = ReadTemperature();
		if(fan_control_mode == 0)
		{
			if(temperature >= max_temp && fan_speed_pwm != max_speed)
			{ 
				fan_speed_pwm = max_speed;
				SetFanSpeed(fan_speed_pwm);
			}
			else if(temperature >= start_temp)
			{ 
				u8 s = start_speed+(temperature-start_temp)/(max_temp-start_temp)*(max_speed-start_speed);
				if(s != fan_speed_pwm)
				{
					fan_speed_pwm = s;
					SetFanSpeed(fan_speed_pwm);
				} 
			}
			else if(temperature < (start_temp-2)  &&  fan_speed_pwm > 0)
			{  
					fan_speed_pwm = 0;
					SetFanSpeed(fan_speed_pwm); 
			}
		}
		
		if(key_pressed)
		{
			if(fan_control_mode == 0)
			{
				fan_control_mode++;
				fan_speed_pwm = speed_levels[0];
				SetFanSpeed(fan_speed_pwm);
			}
			else if(fan_control_mode > 0 && fan_control_mode < 11)
			{
				if(speed_levels[fan_control_mode] > fan_speed_pwm)
				{
					fan_speed_pwm = speed_levels[fan_control_mode];
					SetFanSpeed(fan_speed_pwm); 
					fan_control_mode++;
				}
				else
				{
					fan_speed_pwm = 0;
					SetFanSpeed(fan_speed_pwm); 
					fan_control_mode = 0;
				}
			}
			else
			{
				fan_speed_pwm = 0;
				SetFanSpeed(fan_speed_pwm); 
				fan_control_mode = 0;
			}
			key_pressed = 0;	
		}

		isUsbOutReady() ;
	}
}





void SetFanSpeed(u8 speed)
{
	u32	PWM_Period = MAIN_Fosc/pwm_fre-1;	
	if(led && speed>0) LED_PIN  = 1;
	else LED_PIN = 0;

	if(fan_type == 2)
	{
		PWMB_Duty5((u32)speed*PWM_Period/100); 
	}
	else
	{
		PWMB_Duty6((u32)speed*PWM_Period/100);
		if(speed > 0 && !FAN1_PWM_PIN) FAN1_PWM_PIN = 1;
		if(speed ==0 && FAN1_PWM_PIN) FAN1_PWM_PIN = 0;
	}
}



 

 
void INT0_Isr() interrupt 0
{
	EX0 = 0; 
	delay_ms(10);
	if(!KEY_PIN) key_pressed = 1; 
	EX0 = 1;	
}


void isUsbOutReady() {
	u8	i,j;
	if(QC_charging_en)	return;
	if (!bUsbOutReady || !OutNumber) return;
	//USB_SendData(UsbOutBuffer,OutNumber); 
	for(i=0; i<OutNumber; i++)
	{
		if((UsbOutBuffer[i] >= 'A') && (UsbOutBuffer[i] <= 'Z'))    UsbOutBuffer[i] = UsbOutBuffer[i] - 'A' + 'a';
	}

	//a
	if(UsbOutBuffer[0] == 'a' && OutNumber==1 )
	{
		u8 l = 0;
		printf("\r\n\r\n风扇:       %bu线\r\n", fan_type);
		if(!QC_charging_value) 
						printf("QC诱骗:     否\r\n");
		else 		printf("QC诱骗:     %.1fV\r\n", QC_charging_value);
						printf("当前温度:   %.1f℃\r\n", temperature);	
						printf("风扇速度:   %bu%%\r\n", fan_speed_pwm);
						printf("PWM频率:    %u hz\r\n", pwm_fre);
						printf("LED指示灯:  "); 
		if(led) printf("开启\r\n"); 
		else printf("关闭\r\n"); 
		printf("\r\n温控\r\n    start:%4bd℃ %4bu\%%\r\n    max:  %4bd℃ %4bu\%%\r\n\r\n", start_temp, start_speed,  max_temp, max_speed); 
		for(i=0;i<10;i++)
		{
			if(speed_levels[i]>l)
			{
				l=speed_levels[i];
				printf("档位%bu: %4bu%%\r\n",i+1, speed_levels[i]);
			}
		}
	}
	
	//qc 10.8 
	else if(UsbOutBuffer[0] == 'q' && UsbOutBuffer[1] == 'c' && OutNumber>=4 )
	{
		if(UsbOutBuffer[3] >='0' && UsbOutBuffer[3] <='9')
		{ 
			float result = 0;
			i = 3;
			while (i<OutNumber)
			{
				if(UsbOutBuffer[i] >= '0' && UsbOutBuffer[i] <= '9') break;
				i++;
			}
			while (i<OutNumber)
			{
				if(UsbOutBuffer[i] >= '0' && UsbOutBuffer[i] <= '9') result = result * 10 + (UsbOutBuffer[i] - '0');
				else break;
				i++;
				if(UsbOutBuffer[i] == '.' && i<OutNumber-1)
				{
					result += (UsbOutBuffer[i+1] - '0') /10.0;
					break;
				} 
			}
		
			if(result >= 6 && result <= 20)
			{
				QC_charging_value = result;
				if( (int)QC_charging_value * 10 % 2) QC_charging_value -= 0.1;
			}
			else if(result > 20) QC_charging_value = 20;
			else QC_charging_value = 0;
			if(!QC_charging_value) 
							printf("\r\nQC诱骗:     否\r\n");
			else 		printf("\r\nQC诱骗:     %.1fV\r\n", QC_charging_value);
			SaveData();
		} 

	}
	
	
	//s 50
	else if(UsbOutBuffer[0] == 's' && UsbOutBuffer[1] == ' '  && OutNumber>=3  && OutNumber<=5 )
	{
		if(UsbOutBuffer[2] >='0' && UsbOutBuffer[2] <='9')
		{ 
			u16 num = StringToU16(UsbOutBuffer); 
			if(num>=0 && num<=100)
			{
				fan_speed_pwm = (u8)num;
				fan_control_mode = 99;
				SetFanSpeed(fan_speed_pwm); 
			}
			else 
			{  		
				fan_speed_pwm = 0;
				fan_control_mode = 0;
				SetFanSpeed(fan_speed_pwm); 
			}
			printf("\r\n风扇速度: %bu%%\r\n",fan_speed_pwm);
		} 
	}
	
	//F 40 80 100
	else if(UsbOutBuffer[0] == 'f' && UsbOutBuffer[1] == ' '&& UsbOutBuffer[2]>='0'  && UsbOutBuffer[2]<='9' )
	{
		u8 num = 0, num2 = 0;
		i=2;
		j=0;
		printf("\r\n");
		while (i<OutNumber)
		{
			if (UsbOutBuffer[i] >= '0' && UsbOutBuffer[i] <= '9') 
			{
				num = num * 10 + (UsbOutBuffer[i] - '0');
			} 
			else if (UsbOutBuffer[i] == ',' || UsbOutBuffer[i] == ' ')
			{
				if (num <= 100 && num > num2) 
				{
					speed_levels[j++] = (u8)num;
					printf("%bu  ",speed_levels[j-1]);
					num2 = num;
				}
				num = 0;
			}
			i++;
		}
		if (num <= 100 && num > num2) 
		{
			speed_levels[j] = (u8)num;
			printf("%bu \r\n",speed_levels[j]);
		}
		while(j++<12)
		{
			speed_levels[j] = 0;
		}
		SaveData();					
	}
	
	
	//ST -55
	//MT 70
	else if((UsbOutBuffer[0] == 's'  || UsbOutBuffer[0] == 'm' ) && UsbOutBuffer[1] == 't'  && OutNumber>=4  && OutNumber<=6 )
	{
		if((UsbOutBuffer[3] >='0' && UsbOutBuffer[3] <='9')  || UsbOutBuffer[3] =='-')
		{ 
			char num = (char)StringToU16(UsbOutBuffer);
			if((UsbOutBuffer[3] =='-' && num>=0 && num<=55) || (UsbOutBuffer[3] >='0' && UsbOutBuffer[3] <='9' && num>=0 && num<=125))
			{
				char tt = UsbOutBuffer[3] =='-'?-1*num:num;
				if(UsbOutBuffer[0] == 's')
				{
					start_temp = tt<max_temp? tt:start_temp;
					printf("\r\nstart_temp: %bd℃\r\n",start_temp);
				}
				else if(UsbOutBuffer[0] == 'm')
				{
					max_temp = tt > start_temp?tt:max_temp;
					printf("\r\nmax_temp: %bd℃\r\n",max_temp);
				} 
			}
			SaveData();
		}  
	}
				
	//SS 10
	//MS 100
	else if((UsbOutBuffer[0] == 's'  || UsbOutBuffer[0] == 'm' ) && UsbOutBuffer[1] == 's'  && OutNumber>=4  && OutNumber<=6 )
	{
		if(UsbOutBuffer[3] >='0' && UsbOutBuffer[3] <='9')
		{ 
			u16 num = StringToU16(UsbOutBuffer); 
			if(num<=0 ||  num>100) return; 
			if(UsbOutBuffer[0] == 's')
			{
				start_speed = num < max_speed? (u8)num:start_speed;
				SaveData();
				printf("\r\nstart_speed: %bu%%\r\n",start_speed);
			}
			else if(UsbOutBuffer[0] == 'm' && max_speed!=i)
			{
				max_speed = num>start_speed? (u8)num:max_speed;
				SaveData();
				printf("\r\nmax_speed: %bu%%\r\n",max_speed);
			}
		} 
	}
					
	//fre 1000
	else if(UsbOutBuffer[0] == 'f' && UsbOutBuffer[1] == 'r'&& UsbOutBuffer[2] == 'e')
	{
		u16 num = StringToU16(UsbOutBuffer);
		if(num >= 500 && num <= 50000)
		{
			if(pwm_fre != num)
			{
				pwm_fre = num;
				PWMB_CEN_Enable(0);   
				PWMB_BrakeOutputEnable(0);
				PWMB_AutoReload(MAIN_Fosc / pwm_fre - 1);
				PWMB_CEN_Enable(1);    
				PWMB_BrakeOutputEnable(1); 
				SetFanSpeed(fan_speed_pwm); 
				SaveData();
			} 
			printf("\r\nPWM_Fre: %u\r\n",pwm_fre);
		}
	}		
					
	else if(UsbOutBuffer[0] == 'l' && UsbOutBuffer[1] == 'e' && UsbOutBuffer[2] == 'd' && OutNumber==5 )
	{
		if(UsbOutBuffer[4]=='1')
		{
			led = 1;
			printf("\r\nLED: 1\r\n");
			if(fan_speed_pwm>0) LED_PIN = 1;
			else LED_PIN = 0;
			SaveData();
		}
		else if(UsbOutBuffer[4]=='0')
		{
			led = 0;
			printf("\r\nLED: 0\r\n");
			LED_PIN = 0;
			SaveData();
		}
	}
					
	else if(UsbOutBuffer[0] == 't' && OutNumber==1 )
	{
			printf("    %.1f℃  %bu%%\r\n",temperature,fan_speed_pwm);
	}				
					
	usb_OUT_done();      
}





void ReadData(void)
{
	u8 h,l;
	char temp;
	h = EEPROM_read(addr_pwm_fre);
	l = EEPROM_read(addr_pwm_fre + 1);
	if(h !=0xFF && l !=0xFF)	pwm_fre = (h << 8) | l;

	h = EEPROM_read(addr_QC_charging_value);
	if(h !=0xFF)
	{
		if(h >=6 && h <=20)
		{
			QC_charging_value =(float)h;
			l = EEPROM_read(addr_QC_charging_value+1);
			if(l !=0xFF)
			{
				QC_charging_value += l/10.0;
			}
		}
	}
	
	temp = EEPROM_readChar(addr_start_temp);
	if(h !=0xFF)	start_temp = temp;

	temp = EEPROM_read(addr_max_temp);
	if(temp != 0xFF && temp > start_temp)	max_temp = temp;
	
	h = EEPROM_read(addr_start_speed);
	if(h !=0xFF)
	{
		start_speed = h>100?100:h;
	}
	
	
	h = EEPROM_read(addr_max_speed);
	if(h !=0xFF)
	{
		max_speed = h>100?100:h;
	}
	
	h = EEPROM_read(addr_led);
	if(h !=0xFF)
	{
		led = h;
	}


	
	for(l=0; l<10; l++)
	{ 
		h = EEPROM_read(addr_speed_levels + l);
		if(h ==0xFF) break;
		speed_levels[l] = h;
		speed_levels[l+1] = 0;
	}  
}


void SaveData(void)
{
	u8 h,l;
	EEPROM_SectorErase(addr_pwm_fre);
	
	h = (pwm_fre >> 8) & 0xFF;
	l = pwm_fre & 0xFF;
	EEPROM_write(addr_pwm_fre,h);
	EEPROM_write(addr_pwm_fre+1,l);

	
	h = (u8)QC_charging_value;
	l = (u8)(QC_charging_value*10)%10;
	EEPROM_write(addr_QC_charging_value, h);
	EEPROM_write(addr_QC_charging_value+1, l);
	EEPROM_writeChar(addr_start_temp,start_temp);
	EEPROM_writeChar(addr_max_temp,max_temp);
	EEPROM_write(addr_start_speed, start_speed);
	EEPROM_write(addr_max_speed, max_speed);
	EEPROM_write(addr_led, led);
	for(l=0; l<10; l++)
	{ 
		if(speed_levels[l] ==0) break;
		EEPROM_write(addr_speed_levels+l, speed_levels[l]);
	}
	
}



u16 StringToU16(char *str) {
	u16 result = 0;
	u8  i = 0;
	while (str[i] < '0' || str[i] > '9')
	{
		i++;
	}
	while (str[i] >= '0' && str[i] <= '9' && i < OutNumber)
	{
		result = result * 10 + (str[i] - '0');
		i++;
		if (result > 65535)
		{
			return 65535;
		}
	}
	return result;
}



