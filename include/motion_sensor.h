#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Arduino.h>

// Motion sensor state
extern volatile bool motion_detected;
extern volatile unsigned long last_motion_time;

// Motion sensor functions
bool initMotionSensor();
void deinitMotionSensor();
bool isMotionDetected();
void resetMotionTimer();
unsigned long getTimeSinceLastMotion();

// Interrupt handler (called from ISR)
void IRAM_ATTR motionDetectedISR();

#endif  // MOTION_SENSOR_H
