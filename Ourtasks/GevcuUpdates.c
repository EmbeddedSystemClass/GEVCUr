/******************************************************************************
* File Name          : GevcuUpdates.c
* Date First Issued  : 07/02/2019
* Description        : Update outputs in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "CanTask.h"
#include "gevcu_msgs.h"

#include "morse.h"

/* From 'main.c' */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/* *************************************************************************
 * void GevcuUpdates(void);
 * @brief	: Update outputs based on bits set
 * *************************************************************************/
void GevcuUpdates(void)
{
	/* Reset new various flags. */
	gevcufunction.evstat &= ~CNCTEVADC;
	gevcufunction.evstat |= (
		CNCTEVTIMER1 | /* Timer tick */
		CNCTEVADC      /* new ADC readings */
		);

	

	/* Queue keep-alive status CAN msg */
	if ((gevcufunction.outstat & CNCTOUT05KA) != 0)
	{
		gevcufunction.outstat &= ~CNCTOUT05KA;	
	}


	return;
}

