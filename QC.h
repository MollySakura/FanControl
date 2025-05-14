#ifndef	__QC_H
#define	__QC_H

#include	"config.h"
#include	"STC8G_H_Delay.h"
#define 	DP_PU_Pin  P24
#define 	DP_PD_Pin  P26
#define 	DP_CONTROL_Pin  P27

#define 	DM_PU_Pin  P00
#define 	DM_PD_Pin  P01
#define 	DM_CONTROL_Pin  P02

void setDP_Voltage(u8 v);
void setDM_Voltage(u8 v);
void QC_Handshake(void);
bit QCCharge(float qcTrickVoltage);

#endif
