#ifndef GLOBAL_H
#define GLOBAL_H

#include <TMC2130Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>
#include "ADF4351.h" 
#include "AD5593R.h"

#include "Pins.h"      // Pins are defined here
#include "Stepper.h"
#include "Positions.h" // Calibrated frequency positions are defined her

// We want these objects to be accessible from all files
extern ADF4351 adf4351;
extern Stepper tuner;
extern Stepper matcher;
extern AD5593R adac;

#endif