#pragma once



#include "hw.h"
#include "hw_def.h"
#include "log_def.h"
#include "bsp.h"
#include "monitor.h" 


void apMain(void);
void apInit(void);
void apStopAutoTasks(void);
void apSyncPeriods(uint32_t period);

//
void cliTemp(uint8_t argc, char **argv);
void cliCan(uint8_t argc, char **argv);
void cliLed(uint8_t argc, char **argv);
void cliInfo(uint8_t argc, char **argv);
void cliSys(uint8_t argc, char **argv);
void cliButton(uint8_t argc, char **argv);
void cliMd(uint8_t argc, char **argv);
void cliGpio(uint8_t argc, char **argv);
void cliServo(uint8_t argc, char **argv);