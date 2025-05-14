#include	"QC.h"
#include	"STC8G_H_ADC.h"
void setDP_Voltage(u8 v)
{
	if(v == 0)
	{
		DP_PU_Pin = 0;
		DP_PD_Pin = 0;
		DP_CONTROL_Pin = 0;
	}
	else 	if(v == 6)
	{
		DP_PU_Pin = 1;
		DP_PD_Pin = 0;
		DP_CONTROL_Pin = 0;
	}
	else 	if(v == 33)
	{
		DP_PU_Pin = 1;
		DP_PD_Pin = 0;
		DP_CONTROL_Pin = 1;
	}
}	
void setDM_Voltage(u8 v)
{
	if(v == 0)
	{
		DM_PU_Pin = 0;
		DM_PD_Pin = 0;
		DM_CONTROL_Pin = 0;	
	}
	else 	if(v == 6)
	{
		DM_PU_Pin = 1;
		DM_PD_Pin = 0;
		DM_CONTROL_Pin = 0;
	}
	else 	if(v == 33)
	{
		DM_PU_Pin = 1;
		DM_PD_Pin = 0;
		DM_CONTROL_Pin = 1;
	}
}	


void QC_Handshake(void)
{
	P3M0 = 0x00; P3M1 = 0xfb; 
	P2M0 = 0xdb; P2M1 = 0x24;
	setDP_Voltage(6);
	delay_ms(1500);
	P0M0 = 0x07; P0M1 = 0xf0; 
	setDM_Voltage(0);
	delay_ms(10);
	setDM_Voltage(33);
}


bit QCCharge(float qcTrickVoltage)
{
	u8 i, count = (u8)((qcTrickVoltage - 5)/0.2);
	QC_Handshake();
	delay_ms(50);
	for(i=0; i<count; i++)
	{
		setDP_Voltage(33);
		delay_ms(5);
		setDP_Voltage(6);
		delay_ms(5);
	}
	delay_ms(200);
	if(ReadInputVoltage() >= 5.8) return 1;
	return 0;
}
