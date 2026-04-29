#include "servo.h"
#include "cmsis_os2.h" // For osKernelGetTickCount

bool servoInit(servo_t *p_servo, servo_cfg_t *p_cfg) {
    if (p_servo == NULL || p_cfg == NULL) return false;
    
    p_servo->cfg = *p_cfg;
    p_servo->current_step = 0;
    p_servo->speed_delay_ms = 5; // 기본 속도 5ms
    p_servo->direction = SERVO_DIR_STOP;
    p_servo->is_enabled = false;
    p_servo->last_step_tick = osKernelGetTickCount();
    
    // 초기화 시 모터 비활성화
    servoDisable(p_servo);
    
    return true;
}

void servoEnable(servo_t *p_servo) {
    if (p_servo == NULL || p_servo->cfg.en_port == NULL) return;
    p_servo->is_enabled = true;
    HAL_GPIO_WritePin(p_servo->cfg.en_port, p_servo->cfg.en_pin, GPIO_PIN_SET); // Enable 핀 HIGH 활성화 가정
}

void servoDisable(servo_t *p_servo) {
    if (p_servo == NULL || p_servo->cfg.en_port == NULL) return;
    p_servo->is_enabled = false;
    HAL_GPIO_WritePin(p_servo->cfg.en_port, p_servo->cfg.en_pin, GPIO_PIN_RESET);
}

void servoSetSpeed(servo_t *p_servo, uint32_t delay_ms) {
    if (p_servo == NULL) return;
    p_servo->speed_delay_ms = (delay_ms == 0) ? 1 : delay_ms; // 최소 1ms 보호
}

void servoRun(servo_t *p_servo, servo_dir_t dir) {
    if (p_servo == NULL) return;
    p_servo->direction = dir;
    if (dir != SERVO_DIR_STOP) {
        servoEnable(p_servo); // 달릴 때는 자동으로 Enable
    }
}

void servoStop(servo_t *p_servo) {
    if (p_servo == NULL) return;
    p_servo->direction = SERVO_DIR_STOP;
    // Note: 정지해도 토크 유지를 위해 Enable은 그대로 둠.
    // 완전히 끄려면 servoDisable() 호출 필요.
}

void servoUpdate(servo_t *p_servo) {
    if (p_servo == NULL || !p_servo->is_enabled || p_servo->direction == SERVO_DIR_STOP) {
        return;
    }

    uint32_t tick_now = osKernelGetTickCount();
    if (tick_now - p_servo->last_step_tick >= p_servo->speed_delay_ms) {
        p_servo->last_step_tick = tick_now;
        
        // 방향에 따라 스텝 이동
        if (p_servo->direction == SERVO_DIR_FWD) {
            servoStep(p_servo, p_servo->current_step + 1);
        } else if (p_servo->direction == SERVO_DIR_BWD) {
            servoStep(p_servo, p_servo->current_step - 1);
        }
    }
}


void servoStep(servo_t *p_servo, int step) {
    if (p_servo == NULL) return;

    // 4단계 시퀀스 (Full Step 방식)
    // 음수 step 처리를 위해 (step % 4 + 4) % 4 적용
    int phase = (step % 4 + 4) % 4;
    switch(phase) {
        case 0: // IN1:H, IN2:L, IN3:H, IN4:L
            HAL_GPIO_WritePin(p_servo->cfg.in1_port, p_servo->cfg.in1_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in2_port, p_servo->cfg.in2_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in3_port, p_servo->cfg.in3_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in4_port, p_servo->cfg.in4_pin, GPIO_PIN_RESET);
            break;
        case 1: // IN1:L, IN2:H, IN3:H, IN4:L
            HAL_GPIO_WritePin(p_servo->cfg.in1_port, p_servo->cfg.in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in2_port, p_servo->cfg.in2_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in3_port, p_servo->cfg.in3_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in4_port, p_servo->cfg.in4_pin, GPIO_PIN_RESET);
            break;
        case 2: // IN1:L, IN2:H, IN3:L, IN4:H
            HAL_GPIO_WritePin(p_servo->cfg.in1_port, p_servo->cfg.in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in2_port, p_servo->cfg.in2_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in3_port, p_servo->cfg.in3_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in4_port, p_servo->cfg.in4_pin, GPIO_PIN_SET);
            break;
        case 3: // IN1:H, IN2:L, IN3:L, IN4:H
            HAL_GPIO_WritePin(p_servo->cfg.in1_port, p_servo->cfg.in1_pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(p_servo->cfg.in2_port, p_servo->cfg.in2_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in3_port, p_servo->cfg.in3_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(p_servo->cfg.in4_port, p_servo->cfg.in4_pin, GPIO_PIN_SET);
            break;
    }
    
    p_servo->current_step = step;
}
