#include "config.h"
#include "tone.h"

#include <Arduino.h>

volatile uint16_t gTonePeriod = 16000;
volatile uint16_t gToneCount;
volatile uint16_t gToneCompensation = 0;
volatile uint16_t gTonePhase;
volatile uint16_t gToneFrequency;
volatile int32_t  gToneDelta;

void tone_setup() {
	// Set OC1B direction to OUTPUT 
	pinMode(10, OUTPUT);
	// Mode 15 (fast PWM), no prescaling (1:1)
	// Non-inverted output on OC1B, falling edge input capture
	TCCR1A = (1 << WGM10) | (1 << WGM11) | (1 << COM1B1); // (1 << COM1B0)
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10); // (1 << ICES1)
	// Setup PWM period and duty
	OCR1A = gTonePeriod - 1;	// TOP
	OCR1B = gTonePeriod / 2;	// PWM compare
	// Enable Input Capture and Timer Overflow interrupts
	TIMSK1 = (1 << ICIE1) | (1 << TOIE1);

	/*
	PCICR = (1 << PCIE2);
	PCMSK2 = (1 << PCINT21);
	*/
}

ISR(TIMER1_CAPT_vect) {
	uint16_t phase = ICR1;			// TCNT1 at capture event is stored in ICR1
//ISR(PCINT2_vect) {
//	if (0 == (PIND & (1 << 5))) return;
//	uint16_t phase = TCNT1;
	
	//TCNT1 = 0;
	
	int16_t dphase 	= (int16_t)phase - gTonePhase;
	int16_t dfreq 	= gToneCount - TONE_FREQUENCY;
	
	if (dfreq > 0 && dphase < 0) {
		dfreq--;
		dphase += gTonePeriod;
	}
	else if (dfreq < 0 && dphase > 0) {
		dfreq++;
		dphase -= gTonePeriod;
	}
	
	//int32_t dp32 = dphase + (int32_t)gTonePeriod * dfreq;
	int16_t phase_err = (phase < 8000) ? phase : (phase - gTonePeriod);

	uint16_t newPeriod = gTonePeriod;

	gTonePhase = phase;
	
	gToneCount = 0;	// Restart the frequency counter
	
	
	if (dfreq > 10) dfreq = 10;
	if (dfreq < -10) dfreq = -10;
	
	int16_t delta = 0;
	
	if (dfreq > 0 || dphase > 0) {
		delta += dphase / 4;
		if (dfreq) delta += dfreq * 1000;
	}
	else {
		delta += dphase / 4;
		if (dfreq) delta += dfreq * 1000;
	}
	
	if (!dfreq) {
		delta += phase_err / 256;
		if (phase_err > 3) delta += 1;
		if (phase_err < 3) delta -= 1;
	}
	
	if (delta > 0) {
		if (delta > 1000) delta = 1000;
		gToneCompensation += delta;
		if (gToneCompensation >= 1000) {
			gToneCompensation -= 1000;
			if (newPeriod < TONE_PERIOD_MAX) {
				newPeriod++;
			}
		}
	}
	else {
		delta = -delta;
		if (delta > 1000) delta = 1000;
		if (gToneCompensation > delta) gToneCompensation -= delta;
		else {
			gToneCompensation += (1000 - delta);
			if (newPeriod > TONE_PERIOD_MIN) {
				newPeriod--;
			}
		}		
	}

	// Update Timer1 PWM frequency if necessary
	if (newPeriod != gTonePeriod) {
		gTonePeriod = newPeriod;
		//OCR1A = gTonePeriod - 1;	// TOP
		//OCR1B = gTonePeriod / 2;	// PWM compare
	}
}

ISR(TIMER1_OVF_vect) {
	static uint16_t counter;
	
	counter += gToneCompensation;
	if (counter >= 1000) {
		counter -= 1000;
		OCR1A = gTonePeriod;
	}
	else {
		OCR1A = gTonePeriod - 1;
	}		

	gToneCount++;
}
