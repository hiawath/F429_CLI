#include "hw.h"



void hwInit(void){
    ledInit();
    uartInit(&huart3);
    buttonInit();
    tempInit(&hadc1);
}
