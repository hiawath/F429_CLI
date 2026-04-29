#ifndef SERVO_H
#define SERVO_H

#include "hw_def.h"

typedef struct {
    GPIO_TypeDef *in1_port;
    uint16_t      in1_pin;
    GPIO_TypeDef *in2_port;
    uint16_t      in2_pin;
    GPIO_TypeDef *in3_port;
    uint16_t      in3_pin;
    GPIO_TypeDef *in4_port;
    uint16_t      in4_pin;
    GPIO_TypeDef *en_port;  // Enable pin port
    uint16_t      en_pin;   // Enable pin number
} servo_cfg_t;

typedef enum {
    SERVO_DIR_STOP = 0,
    SERVO_DIR_FWD = 1,
    SERVO_DIR_BWD = -1
} servo_dir_t;

typedef struct {
    servo_cfg_t cfg;
    int current_step;
    uint32_t speed_delay_ms;
    servo_dir_t direction;
    bool is_enabled;
    uint32_t last_step_tick;
} servo_t;

bool servoInit(servo_t *p_servo, servo_cfg_t *p_cfg);
void servoEnable(servo_t *p_servo);
void servoDisable(servo_t *p_servo);
void servoSetSpeed(servo_t *p_servo, uint32_t delay_ms);
void servoRun(servo_t *p_servo, servo_dir_t dir);
void servoStop(servo_t *p_servo);

// 백그라운드 태스크에서 계속 호출해 주어야 하는 업데이트 함수
void servoUpdate(servo_t *p_servo);

// 내부 스텝 제어용 (일반적으로 직접 호출하지 않음)
void servoStep(servo_t *p_servo, int step);

#endif // SERVO_H
