/******************************************************************************
* File Name          : control_law_v1.c
* Date First Issued  : 03/22/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc--PI Loop
*******************************************************************************/
/*
Speed PI Loop

*/
#include <stdio.h>

#include "GevcuTask.h"
#include "GevcuEvents.h"
#include "calib_control_lever.h"
#include "spiserialparallelSW.h"
#include "LEDTask.h"
#include "control_law_v1.h"

struct CTLLAWPILOOP // Control Law PI Loop
{
	//	Working variables
	float spderr;	//	speed error
	float dsrdspd;	//	desired speed
	float intgrtr;//	PI integrator

	//	Parameters
	float kp;    	// Proportional constant
	float ki;    	// Integral constant
	float clp;		//	integrator anti-windup clip level
	float fllspd;	//	100% control lever speed magnitude

   // Limits
	float fmaxtorque_pbopen;  // Max torque (Nm) (pushbutton open/released)
	float fmaxtorque_pbclosed;// Max torque (Nm) (pushbutton closed/pressed)
	float maxspeed_pos;
	float maxspeed_neg;
};

static struct CTLLAWPILOOP pi;

static uint8_t init_flag = 0; // Bootup one-time init

/* *************************************************************************
 * void control_law_v1_init(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Load parameters
 * *************************************************************************/
void control_law_v1_init(void)
{
	init_flag = 1; // Bootup one-time init

	/* Load parameters and initialize variables. */
	/* See: struct CTLLAWPILOOP in dmoc_control.h. */
	pi.kp = 0.015f;  	// Proportional constant
	pi.ki = 0.15E-3f; 	// Integral constant
	pi.fllspd = 360.0f;	//	100% control lever desired speed magnitude

	pi.spderr    = 0;
	pi.dsrdspd   = 0;
	pi.intgrtr   = 0;

	pi.fmaxtorque_pbopen   =  30.0f; // Max torque (Nm) (Pushbutton open/released)
	pi.fmaxtorque_pbclosed = -30.0f; // Max torque (Nm) (Pushbutton closed/pressed)
	pi.maxspeed_pos     =  2500; // Max speed forward
	pi.maxspeed_neg     = -2500; // Max speed in reverse

	/* Initialize DMOC that is in TORQUE mode. */
	dmoc_control_initTORQUE();

	/* Initialize DMOC that is in SPEED mode. */
// TODO: resolve if DMOC "mode" should be speed of torque!
// I.e. do"we" control speed via torque, or does DMOC via its speed mode?
	dmoc_control_initSPEED();

	pi.maxspeed_pos     =  2500; // Max speed forward
	pi.maxspeed_neg     = -2500; // Max speed in reverse

	/* Initialize DMOC that is in TORQUE mode. */
	dmoc_control_initTORQUE();

	/* Initialize DMOC that is in SPEED mode. */
// TODO: resolve if DMOC "mode" should be speed of torque!
// I.e. do"we" control speed via torque, or does DMOC via its speed mode?
//	dmoc_control_initSPEED();

	return;
}

/* *************************************************************************
 * void control_law_v1_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v1_calc(struct DMOCCTL* pdmocctl)
{
	/* Init parameters automatically on bootup. */
	if (init_flag == 0) control_law_v1_init();

	//	Compute desred speed based on control lever and PB conditons
	/* Press pushbutton for direction reversal */
	pi.dsrdspd = 0.01f * clfunc.curpos * pi.fllspd;	//	Desired speed magnitude
	if (gevcufunction.psw[PSW_ZODOMTR]->db_on == SW_CLOSED)
	{ 
		pi.dsrdspd = -pi.dsrdspd;
		led_retrieve.mode = LED_ON;
	}
	else
	{
		led_retrieve.mode = LED_OFF;
	}

	//	Compute speed error
	pi.spderr = pi.dsrdspd - pdmocctl->speedact;

	//	Update integrator and clp if needed
	pi.intgrtr += pi.spderr * pi.ki;
	if (pi.intgrtr > pi.clp) 
	{
		pi.intgrtr = pi.clp;
	}
	else if (pi.intgrtr < -pi.clp)
	{
		pi.intgrtr = -pi.clp;
	}

	//	Compute and limit torque command
	pdmocctl->ftorquereq = pi.spderr * pi.kp + pi.intgrtr;
	if (pdmocctl->ftorquereq > pi.clp) 
	{
		pdmocctl->ftorquereq = pi.clp;
	}
	else if (pdmocctl->ftorquereq < -pi.clp)
	{
		pdmocctl->ftorquereq = -pi.clp;
	}

	/* Update LED state. */
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}

/*
QUESTIONS:
Control Law V2???
What controls persistence/scope of variables
f and i before floats and integers. when to use.
Is init needed?
Is structure needed
What makes the ftorquereq be sent out immediately
fmaxtorque_pbopen misnomer
// vs /*, when to use
Elaborate comment blocks and @ values, purpose and where did they come from
*/
