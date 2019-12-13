/******************************************************************************
* File Name          : dmoc_control.h
* Date First Issued  : 12/01/2019
* Board              : DiscoveryF4
* Description        : Control of dmoc unit
*******************************************************************************/
#ifndef __DMOC_CONTROL
#define __DMOC_CONTROL

#include <stdio.h>

#include <string.h>
#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "main.h"
#include "morse.h"
#include "common_can.h"

#define NUMDMOC 1	// Number of DMOCs

/* Number ofr sw1tim ticks to give 64/sec rate. */
#define DMOC_KATICKS (2)	

/* GEVCU MotorController.h */
	enum Gears {
		DMOC_NEUTRAL = 0,
		DMOC_DRIVE = 1,
		DMOC_REVERSE = 2,
		DMOC_ERROR = 3,
	};

	enum PowerMode {
		DMOC_MODETORQUE,
		DMOC_MODESPEED
	};

        enum OperationState {
		DMOC_DISABLED = 0,
		DMOC_STANDBY = 1,
		DMOC_ENABLE = 2,
		DMOC_POWERDOWN = 3
	};

enum DMOC_CONTROL_STATE
{
	DMOCINIT1,
	DMOCINIT2,
	DMOCCLEAR,
	DMOCONLINE
};

/* CAN msgs sent to DMOC. */
struct DMOCCMDMSG
{
	struct CANTXQMSG txqcan; // Can msg
	uint32_t nextctr;     // Next send time ct
	uint8_t sendflag;     // 1 = send CAN msg, 0 = skip
};


/* DMOC Control */
struct DMOCCTL
{
	struct DMOCCMDMSG cmd[3]; // Three command msgs required
	uint32_t nextctr;     // Next send time ct

	int32_t speedreq;     // Requested speed (signed)
	int32_t torquereq;    // Torque Request (signed)
	int32_t torquecmd;    // Command (do we need this?)
	int32_t maxspeed;     // Max speed (signed)
	int32_t maxtorque;    // Max torque (signed)
	int32_t speedact;     // Speed actual (reported)
	int32_t torqueact;    // Torque actual (signed)
	int32_t regencalc;    // Calculated from maxregenwatts
	int32_t accelcalc;    // Calculated from maxaccelwatts
	int32_t currentact;   // Current Actual (reported)


	uint32_t maxregenwatts;
	uint32_t maxaccelwatts;
	uint32_t torqueoffset; // Offset for zero torque,     (nominally 30000)
	uint32_t speedoffset;  // Offset for zero speed       (nominally 20000)
	uint32_t currentoffset;// Offset for reported current (nominally  5000)
	uint32_t activityctr;  // A counter

	// Extracted and calculated (deg C) temperatures
	uint8_t rotortemp;     // Temperature: rotor (raw)
	uint8_t invtemp;       // Temperature: inverter (raw)
	uint8_t statortemp;    // Temperature: stator (raw)
	uint8_t invtempcalc;   // Temperature: inverter (deg C)
	uint8_t motortemp;     // Temperature: motor (deg C)

	uint8_t state;        // Our State machine
	uint8_t dmocstatereq; // DMOC state requested
	uint8_t dmocstateact; // DMOC state actual
	uint8_t dmocstatefaulted; // 1 = faulted
	uint8_t dmocready;
	uint8_t alive;        // DMOC counter (see docs)
	uint8_t mode;         // Speed or Torque selection
	uint8_t sendflag;     // 1 = send CAN msg, 0 = skip
};

/* ***********************************************************************************************************/
void dmoc_control_init(struct DMOCCTL* pdmocctl);
/* @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Prep for dmoc handling
 * ***********************************************************************************************************/
void dmoc_control_time(struct DMOCCTL* pdmocctl, uint32_t ctr);
/* @brief	: Timer input to state machine
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: ctr = sw1ctr time ticks
 ************************************************************************************************************* */
void dmoc_control_GEVCUBIT08(struct DMOCCTL* pdmocctl, struct CANRCVBUF* pcan);
/* @brief	: CAN msg received: cid_dmoc_actualtorq
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: pcan = pointer to CAN msg struct
************************************************************************************************************* */
void dmoc_control_throttlereq(struct DMOCCTL* pdmocctl, float fpct);
/* @brief	: Convert 0 - 100 percent into torquereq
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: fpct = throttle (control lever) position: 0.0 - 100.0
 ************************************************************************************************************* */
void dmoc_control_CANsend(struct DMOCCTL* pdmocctl);
/* @brief	: Send group of three CAN msgs to DMOC
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 ************************************************************************************************************* */

extern struct DMOCCTL dmocctl[NUMDMOC]; // Allow for multiple DMOCs

#endif

