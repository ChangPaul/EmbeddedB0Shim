/*
 * pulsecounter.h
 *
 *  Created on: Oct 14, 2015
 *      Author: changp
 */

#ifndef PULSECOUNTER_H_
#define PULSECOUNTER_H_

int      openPulseCounter     	( float time_ms );
int      closePulseCounter    	( void );
void     enablePulseCounter	  	( void );
void     disablePulseCounter  	( void );
void     setPulseTriggerLen   	( float time_ms );
unsigned readPulseCounter		( void );

#endif /* PULSECOUNTER_H_ */
