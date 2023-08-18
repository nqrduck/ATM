#ifndef GLOBAL_H
#define GLOBAL_H

#include <TMC2130Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>
#include "ADF4351.h"
#include "AD5593R.h"

#include "Pins.h" // Pins are defined here
#include "Stepper.h"
#include "Positions.h" // Calibrated frequency positions are defined her

// Global variables for the adac module
#define MAGNITUDE 0
#define PHASE 1
#define VT 2
#define VM 3

// We want these objects to be accessible from all files
extern ADF4351 adf4351;
extern Stepper tuner;
extern Stepper matcher;
extern AD5593R adac;

extern Filter active_filter;

#endif