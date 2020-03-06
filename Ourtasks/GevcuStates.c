/******************************************************************************
* File Name          : GevcuStates.c
* Date First Issued  : 07/01/2019
* Description        : States in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"
#include "LEDTask.h"

#include "GevcuEvents.h"
#include "GevcuStates.h"
#include "GevcuTask.h"
#include "calib_control_lever.h"
#include "contactor_control.h"
#include "yprintf.h"
#include "lcdprintf.h"

#include "gevcu_idx_v_struct.h"
#include "morse.h"
#include "adcparamsinit.h"
#include "lcdmsg.h"
#include "dmoc_control.h"

#define GEVCULCDMSGDELAY 32 // Minimum number of time ticks between LCD msgs
#define GEVCULCDMSGLONG (128*30) // Very long delay

enum GEVCU_INIT_SUBSTATEA
{
	GISA_OTO,  
	GISA_WAIT,
};

/* Flag queue LCD msg only once. defaultTask will call these lcdprintf functions. */
static uint8_t msgflag = 0; // 0 = send; 1 = don't send

/* *************************************************************************
 * void GevcuStates_GEVCU_INIT(void);
 * @brief	: Initialization sequence: One Time Only
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg1(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_INT           ");}
static void lcdmsg2(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"SWITCH TO SAFE      ");}

void GevcuStates_GEVCU_INIT(void)
{	
	void (*ptr2)(void); // Pointer to queue LCD msg
	struct SWITCHPTR* p;

	switch (gevcufunction.substateA)
	{
	case GISA_OTO: // Cycle Safe/Active sw.
		if (flag_clcalibed == 0) 
			break;

	/* Queue LCD msg to be sent once. */
	if (msgflag == 0)
	{ 
		msgflag = 1; // Don't keep banging away with the same msg
		ptr2 = &lcdmsg1; // LCD msg pointer
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
	}

		/* Update LED with SAFE/ACTIVE switch status. */
		p = gevcufunction.psw[PSW_PR_SAFE];
		gevcufunction.safesw_prev = p->db_on;
		switch (p->db_on)
		{
		case 1:
	      led_safe.mode = 0; // LED_SAFE off
			break;
			
		case 2:
	      led_safe.mode = 1; // LED_SAFE on
			break;
		}
		xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);
		msgflag = 0; // Allow next LCD msg to be sent once
		gevcufunction.substateA = GISA_WAIT;
		break;

	case GISA_WAIT: // More OTO to do here?
		if (gevcufunction.psw[PSW_PR_SAFE]->db_on == SWP_OPEN )
		{ // Here SAFE/ACTIVE switch is in ACTIVE position
			if (msgflag == 0)
			{ 
				msgflag = 1; // Don't keep banging away with the same msg
				ptr2 = &lcdmsg2; // LCD msg pointer
				xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
			}
			break;
		}

		/* Go into safe mode,. */
		msgflag = 0; // Allow next LCD msg to be sent once
		gevcufunction.state = GEVCU_SAFE_TRANSITION;
		break;

		default:
			break;
	}
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_SAFE_TRANSITION(void);
 * @brief	: Peace and quiet, waiting for hapless Op.
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg3(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_SAFE_TRANSITIO");}

//#define DEHRIGTEST // Uncomment to skip contactor response waits

void GevcuStates_GEVCU_SAFE_TRANSITION(void)
{
	void (*ptr2)(void) = &lcdmsg3; // LCD msg pointer

	if (msgflag == 0)
	{ 
		msgflag = 1; // Don't keep banging away with the same msg
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
	}
      led_safe.mode    = LED_BLINKFAST; // LED_SAFE blinking
		led_arm_pb.mode  = LED_OFF; // ARM Pushbutton LED
		led_arm.mode     = LED_OFF; // ARM LED
		led_prep_pb.mode = LED_OFF; // PREP Pushbutton LED
		led_prep.mode    = LED_OFF; // PREP LED
	xQueueSendToBack(LEDTaskQHandle, &led_safe   ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_arm_pb ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_arm    ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_prep_pb,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_prep   ,portMAX_DELAY);

	/* Request contactor to DISCONNECT. */
	cntctrctl.req = CMDRESET;


#ifndef DEHRIGTEST
	/* Wait until contactor shows DISCONNECTED state. */
	if ((cntctrctl.cmdrcv & 0xf) != DISCONNECTED)
	{ // LCD msg here?
		return;
	}
#endif

	msgflag = 0; // Send LCD msg once

   led_safe.mode    = LED_ON;
	xQueueSendToBack(LEDTaskQHandle, &led_safe   ,portMAX_DELAY);

	msgflag = 0; // Allow next LCD msg to be sent once
	gevcufunction.state = GEVCU_SAFE;
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_SAFE(void);
 * @brief	: Peace and quiet, waiting for hapless Op.
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg4(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_SAFE          ");}

void GevcuStates_GEVCU_SAFE(void)
{
	void (*ptr2)(void) = &lcdmsg4; // LCD msg pointer

	if (msgflag == 0)
	{ 
		msgflag = 1; // Don't keep banging away with the same msg
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
	}
		
	if (gevcufunction.psw[PSW_PR_SAFE]->db_on == SWP_OPEN )
	{ // Here SAFE/ACTIVE switch is in ACTIVE position
		msgflag = 0; // Allow next LCD msg to be sent once
		gevcufunction.state = GEVCU_ACTIVE_TRANSITION;

		led_safe.mode = LED_OFF; // LED_SAFE off
		xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);

		led_prep.mode = LED_BLINKFAST; // PREP Pushbutton LED fast blink mode
		xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

		/* Request contactor to CONNECT. */
		cntctrctl.req = CMDCONNECT;

		/* Set the last received contactor response to bogus. */
		cntctrctl.cmdrcv = 0x8f; // Connect cmd w bogus response code
		return;
	}
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ACTIVE_TRANSITION(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg5(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ACTIVE_TRANSIT");}

void GevcuStates_GEVCU_ACTIVE_TRANSITION(void)
{
	void (*ptr2)(void) = &lcdmsg5; // LCD msg pointer

	if (msgflag == 0)
	{ 
		msgflag = 1; // Don't keep banging away with the same msg
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
	}

#ifndef DEHRIGTEST
	/* Wait for CONNECTED. */
	if ((cntctrctl.cmdrcv & 0xf) != CONNECTED)
	{ // Put a stalled loop timeout here?
		cntctrctl.req = CMDCONNECT;
		return;
	}

#endif

	/* Contactor connected. */

	led_prep_pb.mode = LED_OFF; // PREP Pushbutton off
	xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

	led_prep.mode = LED_ON; // PREP state led on
	xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

	led_arm_pb.mode = LED_BLINKFAST; // ARM Pushbutton LED fast blink mode
	xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

	msgflag = 0; // Allow next LCD msg to be sent once
	gevcufunction.state = GEVCU_ACTIVE;
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ACTIVE(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg6(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ACTIVE        ");}

void GevcuStates_GEVCU_ACTIVE(void)
{
	void (*ptr2)(void) = &lcdmsg6; // LCD msg pointer

	if (msgflag == 0)
	{ 
		msgflag = 1; // Don't keep banging away with the same msg
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
	}

	/* Wait for ARM pushbutton to be pressed. */	
	if (gevcufunction.psw[PSW_PB_ARM]->db_on != SW_CLOSED)
		return;
	
	/* Here, ARM_PB pressed, requesting ARMed state. */
	led_arm_pb.mode = LED_ON; // ARM Pushbutton LED
	xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

	led_prep.mode = LED_OFF; // PREP state LED
	xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

	msgflag = 0; // Allow next LCD msg to be sent once
	gevcufunction.state = GEVCU_ARM_TRANSITION;
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ARM_TRANSITION(void);
 * @brief	: Do everything needed to get into state
 * *************************************************************************/
//  20 chars will over-write all display chars from previous msg:       12345678901234567890
static void lcdmsg7(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"ARM: MOVE CL ZERO   ");}
static void lcdmsg8(void){lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ARM           ");}

void GevcuStates_GEVCU_ARM_TRANSITION(void)
{
	void (*ptr2)(void); // Pointer to queue LCD msg

		/* Make sure Op has CL in zero position. */
		if (clfunc.curpos > 0)
		{

			if (msgflag == 0)
			{ 
				msgflag = 1; // Don't keep banging away with the same msg
				ptr2 = &lcdmsg7; // LCD msg pointer
				xQueueSendToBack(lcdmsgQHandle,&ptr2,0);
			}
			return;
		}

		ptr2 = &lcdmsg8; // LCD msg pointer
		xQueueSendToBack(lcdmsgQHandle,&ptr2,0);

		led_arm_pb.mode = LED_OFF; // ARM Pushbutton LED
		xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

		led_prep.mode = LED_OFF; // PREP state LED
		xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

		led_arm.mode = LED_ON; // ARM state LED
		xQueueSendToBack(LEDTaskQHandle,&led_arm,portMAX_DELAY);

		msgflag = 0; // Allow next LCD msg to be sent once
		gevcufunction.state = GEVCU_ARM;
		return;

}
/* *************************************************************************
 * void GevcuStates_GEVCU_ARM(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
void GevcuStates_GEVCU_ARM(void)
{
	/* Pressing PREP returns to ACTIVE, (not armed) state. */
	if (gevcufunction.psw[PSW_PB_PREP]->db_on == SW_CLOSED)
	{
		led_prep.mode = LED_ON; // PREP state led on
		xQueueSendToBack(LEDTaskQHandle,&led_prep,portMAX_DELAY);

		led_arm.mode = LED_OFF; // ARM state LED
		xQueueSendToBack(LEDTaskQHandle,&led_arm,portMAX_DELAY);

		/* Set DMOC torque to zero. */
		dmocctl[0].pbctl = 0; 

		gevcufunction.state = GEVCU_ACTIVE_TRANSITION;
		return;		
	}

	/* Press pushbutton to send torque commands scaled by CL. */
	if (gevcufunction.psw[PSW_ZODOMTR]->db_on == SW_CLOSED)
	{
		/* Set DMOC torque to CL scaled. */
		dmocctl[0].pbctl = 1; 

		led_retrieve.mode = LED_ON;
	}
	else
	{
		/* Set DMOC torque to zero. */
		dmocctl[0].pbctl = 0; 

		led_retrieve.mode = LED_OFF;	
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);

	return;
}

