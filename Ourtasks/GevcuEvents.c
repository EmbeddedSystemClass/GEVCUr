/******************************************************************************
* File Name          : GevcuEvents.c
* Date First Issued  : 07/01/2019
* Description        : Events in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/
/*
The CL calibration and ADC->pct position is done via ADC new readings notifications.


*/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "gevcu_idx_v_struct.h"
#include "GevcuEvents.h"
#include "morse.h"

#include "SerialTaskReceive.h"
#include "GevcuTask.h"
#include "can_iface.h"
#include "gevcu_cmd_msg.h"
#include "gevcu_msgs.h"
#include "MailboxTask.h"
#include "calib_control_lever.h"
#include "contactor_control.h"
#include "dmoc_control.h"
#include "LEDTask.h"
#include "shiftregbits.h"

#include "main.h"

/* *************************************************************************
 * void GevcuEvents_00(void);
 * @brief	: ADC readings available
 * *************************************************************************/
void GevcuEvents_00(void)
{
	float fclpos; 

	gevcufunction.evstat |= EVNEWADC; // Show new readings ready

	/* Update Control Lever psosition with new ADC readings (or do initial calib). */
//	fclpos = calib_control_lever();

	/* Convert control level position into torque request for DMOC #1. */
	dmoc_control_throttlereq(&dmocctl[0], fclpos);
	
	return;
}
/* *************************************************************************
 * void GevcuEvents_01(void);
 * @brief	: Switch pair: SAFE/ACTIVE
 * *************************************************************************/
// Debugging & test
struct LEDREQ led_safe    = {LED_SAFE,   0};
void GevcuEvents_01(void)
{
	struct SWITCHPTR* p = psw[PSW_PR_SAFE];
	switch (p->db_on)
	{
	case 1:
      led_safe.mode = 0;
		break;
			
	case 2:
      led_safe.mode = 1;
		break;
	}
	xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);
	return;
}
/* *************************************************************************
 * void GevcuEvents_02(void);
 * @brief	: Z_ODOMTR
 * @param	: psw = pointer to switch struct
 * *************************************************************************/

void GevcuEvents_02(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_03(void);
 * @brief	: Torque reversal pushbutton
 * @param	: psw = pointer to switch struct
 * *************************************************************************/
// Debugging & test
struct LEDREQ led_climb    = {LED_CLIMB,   0};
struct LEDREQ led_retrieve = {LED_RETRIEVE,0};
struct LEDREQ led_arm_pb   = {LED_ARM_PB,  0};
struct LEDREQ led_prep_pb  = {LED_PREP_PB, 0};

void GevcuEvents_03(void)
{  // One or more pushbuttons have changed

/* Update all four LEDs even though only one PB changed. */
	struct SWITCHPTR* p = psw[PSW_ZTENSION];
	if (p->db_on == SW_CLOSED) 
      led_climb.mode = LED_BLINKFAST;
	else 	
      led_climb.mode = 0;
	xQueueSendToBack(LEDTaskQHandle,&led_climb,portMAX_DELAY);

	p = psw[PSW_ZODOMTR];
	if (p->db_on == SW_CLOSED)
		led_retrieve.mode = 1;
	else
		led_retrieve.mode = 0;	
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);

	p = psw[PSW_PB_ARM];
	if (p->db_on == SW_CLOSED)
		led_arm_pb.mode = 1;
	else
		led_arm_pb.mode = 0;	
	xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

	p = psw[PSW_PB_PREP];
	if (p->db_on == SW_CLOSED)
		led_prep_pb.mode = LED_BLINKWINK;
	else
		led_prep_pb.mode = 0;	
	xQueueSendToBack(LEDTaskQHandle,&led_prep_pb,portMAX_DELAY);

	return;
}
/* *************************************************************************
 * void GevcuEvents_04(void);
 * @brief	: TIMER1: Software timer 1
 * *************************************************************************/
uint32_t dbgev04;


void GevcuEvents_04(void)
{
	gevcufunction.swtim1ctr += 1;
	gevcufunction.evstat |= EVSWTIM1TICK; // Timer tick

	/* Keepalive for contactor CAN msgs. */
	contactor_control_time(gevcufunction.swtim1ctr);

	/* Keepalive and torque command for DMOC */
	dmoc_control_time(&dmocctl[0], gevcufunction.swtim1ctr);

	return;
}
/* *************************************************************************
 * void GevcuEvents_05(void);
 * @brief	: TIMER2: Software timer 2
 * *************************************************************************/
void GevcuEvents_05(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_06(void);
 * @brief	: CAN: cid_gps_sync
 * *************************************************************************/
void GevcuEvents_06(void)
{
//	gevcu_cmd_msg_i(pcf); // Build and send CAN msg with data requested
	return;
}
/* *************************************************************************
 * void GevcuEvents_07(void);
 * @brief	: CAN: cid_cntctr_keepalive_r
 * *************************************************************************/
void GevcuEvents_07(void)
{
	gevcufunction.evstat |= EVCANCNTCTR; // Show New Contactor CAN msg 
	
	/* Send pointer to CAN msg to contactor control routine */
	contactor_control_CANrcv(gevcufunction.swtim1ctr,\
              &gevcufunction.pmbx_cid_cntctr_keepalive_r->ncan.can);
		
	return;
}	
/* *************************************************************************
 * void GevcuEvents_08(void);
 * @brief	: CAN: cid_dmoc_actualtorq
 * *************************************************************************/
void GevcuEvents_08(void)
{
	dmoc_control_GEVCUBIT08(&dmocctl[0],\
        &gevcufunction.pmbx_cid_dmoc_actualtorq->ncan.can);
	return;
}
/* *************************************************************************
 * void GevcuEvents_09(void);
 * @brief	: CAN: cid_dmoc_speed,     NULL,GEVCUBIT09,0,I16_X6);
 * *************************************************************************/
void GevcuEvents_09(void)
{
	dmoc_control_GEVCUBIT09(&dmocctl[0],\
        &gevcufunction.pmbx_cid_dmoc_speed->ncan.can);
	return;
}
/* *************************************************************************
 * void GevcuEvents_10(void);
 * @brief	: CAN: cid_dmoc_dqvoltamp, NULL,GEVCUBIT10,0,I16_I16_I16_I16);
 * *************************************************************************/
void GevcuEvents_10(void)
{

	return;
}
/* *************************************************************************
 * void GevcuEvents_11(void);
 * @brief	: CAN: cid_dmoc_torque, NULL,GEVCUBIT11,0,I16_I16); 
 * *************************************************************************/
void GevcuEvents_11(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_12(void);
 * @brief	: CAN: cid_dmoc_critical_f,NULL,GEVCUBIT12,0,NONE);
 * *************************************************************************/
void GevcuEvents_12(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_13(void);
 * @brief	: CAN: cid_dmoc_hv_status, NULL,GEVCUBIT13,0,I16_I16_X6);
 * *************************************************************************/
void GevcuEvents_13(void)
{
	dmoc_control_GEVCUBIT13(&dmocctl[0],\
        &gevcufunction.pmbx_cid_dmoc_hv_status->ncan.can);
	return;
}
/* *************************************************************************
 * void GevcuEvents_14(void);
 * @brief	: CAN: cid_dmoc_hv_temps,  NULL,GEVCUBIT14,0,U8_U8_U8);
 * *************************************************************************/
void GevcuEvents_14(void)
{
	dmoc_control_GEVCUBIT13(&dmocctl[0],\
        &gevcufunction.pmbx_cid_dmoc_hv_temps->ncan.can);
	return;
}
/* *************************************************************************
 * void GevcuEvents_15(void);
 * @brief	: CAN: cid_gevcur_keepalive_i,NULL,GEVCUBIT15,0,23);
 * *************************************************************************/
void GevcuEvents_15(void)
{
	return;
}

