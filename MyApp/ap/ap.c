#include "cmsis_os2.h"
#define LOG_TAG "AP"

#include "ap.h"
#define AP_LED_MAX_CH 3

static bool is_can_monitor = false;
static bool is_can_clu_send = false;
static uint32_t temp_read_period = 0; // 주기(ms) 0이면 주기적 동작 멈춤
static uint32_t led_toggle_period[AP_LED_MAX_CH] = {0, }; // 0이면 자동 점멸 중지 상태

void apInit(void)
{
    LOG_INF("Application Init... Started");
    cliInit(0); // CLI 엔진 기본 세팅 (UART 채널 0번 주입)
    cliSetCtrlCHandler(apStopAutoTasks); // Ctrl+C 핸들러 등록 (DIP)
    monitorSetSyncHandler(apSyncPeriods); // 모니터 동기화 핸들러 등록 (DIP)
    logInit();
    monitorInit(); // 모니터링 시스템 스레드 락 및 변수 초기화

    cliAdd("led", cliLed); // "터미널에서 led 치면 cliLed 함수 실행해 줘" 등록
    cliAdd("info", cliInfo);
    cliAdd("sys", cliSys);
    cliAdd("gpio", cliGpio);     // GPIO 읽기/쓰기 커맨드 등록
    cliAdd("md", cliMd);         // 메모리 덤프 커맨드 등록
    cliAdd("button", cliButton); // 버튼 보고 제어 등록
    cliAdd("temp", cliTemp);     // 온도 센서 명령 등록
    cliAdd("can", cliCan);       // CAN 명령 등록
}

void apMain(void)
{

    while (1)
    {
        // cliMain 내부에서 큐를 이용해 무한 대기(Blocking)하므로 CPU 점유율이 0%가
        // 됩니다. 따라서 기존에 있던 osDelay()는 삭제해도 안전합니다.
        cliMain(0xFFFFFFFF);
    }
}
//Task




// 유효한 메모리 영역인지 검사하는 보호 함수
static bool isSafeAddress(uint32_t addr)
{
    // 1. STM32F411 Flash (보통 512KB: 0x0800 0000 ~ 0x0807 FFFF)
    if (addr >= 0x08000000 && addr <= 0x0807FFFF)
        return true;

    // 2. STM32F411 SRAM (보통 128KB: 0x2000 0000 ~ 0x2001 FFFF)
    if (addr >= 0x20000000 && addr <= 0x2001FFFF)
        return true;

    // 3. System Memory 영역 (부트로더 및 공장 출고 UID 영역 등)
    if (addr >= 0x1FFF0000 && addr <= 0x1FFF7A1F)
        return true;

    // 4. Peripheral 레지스터 영역 (0x40000000 부터 시작)
    // 주의: 유효한 영역이라도 클럭이 꺼져있으면 Bus Fault가 나지만, 허용은
    // 해둡니다.
    if (addr >= 0x40000000 && addr <= 0x5FFFFFFF)
        return true;

    // 그 외에 아무것도 매핑되지 않은 주소는 접근 불가 타겟으로 간주하여 차단
    return false;
}

void apStopAutoTasks(void)
{
    monitorOff();
    for (int i=0; i<AP_LED_MAX_CH; i++)
    {
        led_toggle_period[i] = 0;
        ledOff(i);
    }
    temp_read_period = 0;
    tempStopAuto();
}

void apSyncPeriods(uint32_t period)
{
    if (period > 0)
    {
        // 1. 온도 센서 주기 업데이트 및 자원 재기동(필요시)
        tempStartAuto();
        temp_read_period = period;

        // 2. LED 토글 주기 업데이트 (모든 채널 동기화)
        for (int i=0; i<AP_LED_MAX_CH; i++)
        {
            led_toggle_period[i] = period;
        }

        LOG_INF("Tasks Synchronized to %d ms", period);
    }
    else
    {
        temp_read_period = 0;
        for (int i=0; i<AP_LED_MAX_CH; i++)
        {
            led_toggle_period[i] = 0;
        }
    }
}




void StartDefaultTask(void *argument)
{
    hwInit();
    apInit();
    while (1)
    {
        apMain();
    }
}

// 📌 백그라운드 시스템 태스크 (LED 점멸 등)
void ledStartTask(void *argument)
{
    uint32_t pre_time[AP_LED_MAX_CH];
    for (int i=0; i<AP_LED_MAX_CH; i++) pre_time[i] = millis();

    while (1)
    {
        for (int i=0; i<AP_LED_MAX_CH; i++)
        {
            if (led_toggle_period[i] > 0)
            {
                if (millis() - pre_time[i] >= led_toggle_period[i])
                {
                    pre_time[i] = millis();
                    ledToggle(i);

                    if (i == 0) // 모니터링은 0번 기준 유지
                    {
                        bool led_state = ledGetStatus(0);
                        monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);
                    }
                }
            }
            else
            {
                pre_time[i] = millis();
            }
        }
        osDelay(10); // 10ms 주기로 체크
    }
}

// 📌 백그라운드 온도 센서 태스크
void tempStartTask(void *argument)
{
    while (1)
    {
        if (temp_read_period > 0)
        {
            // 이미 켜진 DMA 버퍼에서 값만 쏙 빼와서 출력 (Zero Overhead)
            float t = tempReadAuto();

            // 시나리오 로그 적용
            LOG_DBG("Temp Sensor DMA Buffer Read Success");

            if (t > 40.0f)
            {
                LOG_WRN("High Temperature Alert: %.2f *C", t);
            }

            // 모니터 시스템에 현재 온도값을 갱신 (전략 1)
            monitorUpdateValue(ID_ENV_TEMP, TYPE_FLOAT, &t);

            // 사람을 위한 텍스트 모드일 때만 출력
            if (!isMonitoringOn())
            {
                cliPrintf("Current Temp: %.2f *C\r\n", t);
            }

            osDelay(temp_read_period);
        }
        else
        {
            osDelay(50);
        }
    }
}

// 📌 20ms 고속 모니터링 전용 송신 태스크 (전략 2)
void monitorStartTask(void *argument)
{
    while (1)
    {
        if (isMonitoringOn())
        {
            // 현재 스냅샷에 찍힌 모든 센서들의 값을 TLV 패킷으로 조립하여 UART로 고속
            // 블라스트!
            monitorSendPacket();
        }
        else{


        }
        osDelay(monitorGetPeriod()); 
    }
}



// 📌 백그라운드 CAN 모니터링 태스크
void canStartTask(void *argument)
{
    uint32_t pre_time_clu = millis();

    while (1)
    {
        if (is_can_monitor)
        {
            can_msg_t msg;
            if (canReceive(0, &msg))
            {
                cliPrintf("CAN RX > ID: 0x%03X, DLC: %d, DATA: ", msg.id, msg.dlc);
                for (int i=0; i<msg.dlc; i++) cliPrintf("%02X ", msg.data[i]);
                cliPrintf("\r\n");
            }
        }

        if (is_can_clu_send)
        {
            if (millis() - pre_time_clu >= 100)
            {
                pre_time_clu = millis();

                struct hyundai_2015_mcan_gw_clu_p_t clu_p;
                clu_p.c_vehicle_speed = 100; // 100 km/h
                clu_p.c_odometer = 123456;   // 123,456 km

                uint8_t buf[8];
                hyundai_2015_mcan_gw_clu_p_pack(buf, &clu_p, 8);

                can_msg_t msg;
                msg.id = HYUNDAI_2015_MCAN_GW_CLU_P_FRAME_ID;
                msg.dlc = 8;
                memcpy(msg.data, buf, 8);
                msg.format = 0;
                msg.type = 0;

                canSend(0, &msg);
            }
        }
        else
        {
            pre_time_clu = millis();
        }

        osDelay(10);
    }
}


static servo_t my_servo;

void servoStartTask(void *argument){

    servo_cfg_t cfg = {
        .in1_port = GPIOG, .in1_pin = GPIO_PIN_1,
        .in2_port = GPIOF, .in2_pin = GPIO_PIN_9,
        .in3_port = GPIOF, .in3_pin = GPIO_PIN_7,
        .in4_port = GPIOF, .in4_pin = GPIO_PIN_8,
        .en_port  = GPIOG, .en_pin  = GPIO_PIN_0 // [TODO] 실제 Enable 핀으로 변경하세요!
    };
    servoInit(&my_servo, &cfg);

    // 속도 설정 (스텝 간격 5ms)
    servoSetSpeed(&my_servo, 5);

    // 정방향 실행 시작
    servoRun(&my_servo, SERVO_DIR_FWD);

    uint32_t last_toggle = osKernelGetTickCount();

    while (1) {
        // 백그라운드 업데이트 (이 함수가 지연 없이 계속 호출되어야 함)
        servoUpdate(&my_servo);

        // 예시: 2초마다 방향 전환
        if (osKernelGetTickCount() - last_toggle >= 2000) {
            last_toggle = osKernelGetTickCount();
            if (my_servo.direction == SERVO_DIR_FWD) {
                servoRun(&my_servo, SERVO_DIR_BWD);
            } else {
                servoRun(&my_servo, SERVO_DIR_FWD);
            }
        }

        osDelay(1); // FreeRTOS 태스크 양보 (1ms마다 체크)
    }
}


#define AP_LED_MAX_CH 3


void cliLed(uint8_t argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "all") == 0 && strcmp(argv[2], "off") == 0)
    {
        for (int i=0; i<AP_LED_MAX_CH; i++)
        {
            led_toggle_period[i] = 0;
            ledOff(i);
        }
        cliPrintf("All LEDs OFF\r\n");
        return;
    }

    if (argc >= 3)
    {
        uint8_t ch = atoi(argv[1]);
        if (ch >= AP_LED_MAX_CH)
        {
            cliPrintf("Invalid LED Channel (0~%d) or 'all'\r\n", AP_LED_MAX_CH - 1);
            return;
        }


        if (strcmp(argv[2], "on") == 0)
        {
            led_toggle_period[ch] = 0; // 동작 중지
            ledOn(ch);
            cliPrintf("LED %d ON\r\n", ch);
        }
        else if (strcmp(argv[2], "off") == 0)
        {
            led_toggle_period[ch] = 0; // 동작 중지
            ledOff(ch);
            cliPrintf("LED %d OFF\r\n", ch);
        }
        else if (strcmp(argv[2], "toggle") == 0)
        {
            if (argc == 4)
            {
                // 주기적인 자동 토글 모드 (예: led 0 toggle 1000)
                int period = atoi(argv[3]);
                if (period > 0)
                {
                    led_toggle_period[ch] = period;
                    cliPrintf("LED %d Auto-Toggle Started (%d ms)\r\n", ch, period);
                }
                else
                {
                    cliPrintf("Invalid Period\r\n");
                }
            }
            else
            {
                // 단발성 토글 모드 (예: led 0 toggle)
                led_toggle_period[ch] = 0; 
                ledToggle(ch);
                cliPrintf("LED %d TOGGLED ONCE\r\n", ch);
            }
        }
        
        // LED 상태 즉시 업데이트 (모니터링용은 0번 채널 위주로 유지하거나 확장 필요)
        if (ch == 0)
        {
            bool led_state = ledGetStatus(0);
            monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);
        }
    }
    else
    {
        cliPrintf("Usage: led [ch] [on|off]\r\n");
        cliPrintf("       led [ch] toggle\r\n");
        cliPrintf("       led [ch] toggle [period_ms]\r\n");
        cliPrintf("       led all off\r\n");
    }
}
void cliInfo(uint8_t argc, char **argv)
{
    if (argc == 1) // 인자가 없을 때 기본 정보 출력
    {
        cliPrintf("====================================\r\n");
        cliPrintf(" HW Model   : STM32F411\r\n");
        cliPrintf(" FW Version : V1.0.0\r\n");
        cliPrintf(" Build Date : %s %s\r\n", __DATE__, __TIME__);

        // STM32 Unique ID 레지스터 주소 (F411 기준)
        uint32_t uid0 = *(volatile uint32_t *)0x1FFF7A10;
        uint32_t uid1 = *(volatile uint32_t *)0x1FFF7A14;
        uint32_t uid2 = *(volatile uint32_t *)0x1FFF7A18;
        cliPrintf(" Serial Num : %08X-%08X-%08X\r\n", uid0, uid1, uid2);
        cliPrintf("====================================\r\n");
    }
    else if (argc == 2 && strcmp(argv[1], "uptime") == 0)
    {
        cliPrintf("System Uptime: %d ms\r\n", millis());
    }
    else
    {
        cliPrintf("Usage: info\r\n");
        cliPrintf("       info uptime\r\n");
    }
}
void cliSys(uint8_t argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "reset") == 0)
    {
        cliPrintf("System Reset\r\n");
        NVIC_SystemReset(); // ARM Cortex-M 자체 리셋 함수
    }
    else
    {
        cliPrintf("Usage: sys [reset]\r\n");
    }
}

void cliButton(uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            buttonEnable(true);
            cliPrintf("Button Interrupt Report: ON\r\n");
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            buttonEnable(false);
            cliPrintf("Button Interrupt Report: OFF\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: button [on/off]\r\n");
        cliPrintf("Current Status: %s\r\n", buttonGetEnable() ? "ON" : "OFF");
    }
}


void cliTemp(uint8_t argc, char **argv)
{
    if (argc == 1)
    {
        // 1회 단독 측정 수행 (자동 모드 끄기)
        if (temp_read_period > 0)
        {
            tempStopAuto(); // 기존 자동 모드가 켜져있었다면 DMA 끄기
        }
        temp_read_period = 0;

        float t = tempReadSingle(); // 1회만 Polling으로 전력 아껴서 읽음
        cliPrintf("Current Temp: %.2f *C\r\n", t);
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "off") == 0 || strcmp(argv[1], "stop") == 0)
        {
            temp_read_period = 0;
            tempStopAuto();
            cliPrintf("Temperature Auto-Read Stopped\r\n");
            return;
        }

        int period = atoi(argv[1]);
        if (period > 0)
        {
            if (temp_read_period == 0)
            {
                tempStartAuto(); // DMA Continuous 모드 가동!
            }
            temp_read_period = period;
            cliPrintf("Temperature Auto-Read Started (%d ms)\r\n", period);
        }
        else
        {
            cliPrintf("Invalid Period\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: temp\r\n");
        cliPrintf("       temp [period_ms]\r\n");
        cliPrintf("       temp off\r\n");
    }
}




void cliCan(uint8_t argc, char **argv)
{
    bool ret = false;

    if (argc == 2 && cliIsStr(argv[1], "info"))
    {
        cliPrintf("CAN Channel: 0\r\n");
        cliPrintf("Opened     : %s\r\n", canIsOpened(0) ? "YES":"NO");
        cliPrintf("Rx Count   : %d\r\n", canGetRxCount(0));
        cliPrintf("Tx Count   : %d\r\n", canGetTxCount(0));
        cliPrintf("Err Count  : %d\r\n", canGetErrorCount(0));
        ret = true;
    }

    if (argc == 2 && cliIsStr(argv[1], "open"))
    {
        if (canOpen(0)) cliPrintf("CAN Open Success\r\n");
        else cliPrintf("CAN Open Fail\r\n");
        ret = true;
    }

    if (argc == 2 && cliIsStr(argv[1], "monitor"))
    {
        is_can_monitor = !is_can_monitor;
        cliPrintf("CAN Monitor: %s\r\n", is_can_monitor ? "ON":"OFF");
        ret = true;
    }

    if (argc >= 3 && cliIsStr(argv[1], "send"))
    {
        can_msg_t msg;
        msg.id = (uint32_t)strtoul(argv[2], NULL, 16);
        msg.dlc = argc - 3;
        if (msg.dlc > 8) msg.dlc = 8;
        for (int i=0; i<msg.dlc; i++)
        {
            msg.data[i] = (uint8_t)strtoul(argv[3+i], NULL, 16);
        }
        msg.format = 0;
        msg.type = 0;

        if (canSend(0, &msg)) cliPrintf("CAN Send Success\r\n");
        else cliPrintf("CAN Send Fail\r\n");
        ret = true;
    }

    if (argc == 2 && cliIsStr(argv[1], "sample"))
    {
        hyundai_2015_mcan_sample_cluster();
        cliPrintf("MCAN Cluster Sample Executed (Check source for details)\r\n");
        ret = true;
    }

    if (argc == 3 && cliIsStr(argv[1], "clu_send"))
    {
        if (cliIsStr(argv[2], "on")) is_can_clu_send = true;
        else is_can_clu_send = false;
        cliPrintf("CAN CLU Send: %s\r\n", is_can_clu_send ? "ON":"OFF");
        ret = true;
    }

    if (ret != true)
    {
        cliPrintf("can info\r\n");
        cliPrintf("can open\r\n");
        cliPrintf("can monitor\r\n");
        cliPrintf("can send [id] [d1] [d2] ...\r\n");
        cliPrintf("can sample\r\n");
        cliPrintf("can clu_send [on/off]\r\n");
    }
}


void cliGpio(uint8_t argc, char **argv)
{
    if (argc >= 3)
    {
        // argv[1]: "read" 또는 "write"
        // argv[2]: 핀 이름 (예: "a5", "b12")
        char port_char = tolower(argv[2][0]); // 첫 글자를 소문자로 ('a' ~ 'h')
        int pin_num = atoi(&argv[2][1]);      // 두 번째 글자부터는 숫자로 변환 (0~15)

        uint8_t port_idx = port_char - 'a'; // 'a'는 0, 'b'는 1 ...

        if (strcmp(argv[1], "read") == 0)
        {
            int8_t state = gpioExtRead(port_idx, pin_num);
            if (state < 0)
            {
                cliPrintf("Invalid Port or Pin (ex: a5, b12)\r\n");
            }
            else
            {
                cliPrintf("GPIO %c%d = %d\r\n", toupper(port_char), pin_num, state);
            }
        }
        else if (strcmp(argv[1], "write") == 0 && argc == 4)
        {
            int val = atoi(argv[3]);
            if (gpioExtWrite(port_idx, pin_num, val))
            {
                cliPrintf("GPIO %c%d Set to %d\r\n", toupper(port_char), pin_num, val ? 1 : 0);
            }
            else
            {
                cliPrintf("Invalid Port or Pin (ex: a5, b12)\r\n");
            }
        }
        else
        {
            cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
            cliPrintf("       gpio write [a~h][0~15] [0/1]\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: gpio read [a~h][0~15]\r\n");
        cliPrintf("       gpio write [a~h][0~15] [0/1]\r\n");
    }
}

// 메모리 덤프 (Memory Dump) 명령어
void cliMd(uint8_t argc, char **argv)
{
    if (argc >= 2)
    {
        uint32_t addr = strtoul(argv[1], NULL, 16);
        uint32_t length = 16;

        if (argc >= 3)
        {
            length = strtoul(argv[2], NULL, 0);
        }

        // 16바이트씩 단위로 루프
        for (uint32_t i = 0; i < length; i += 16)
        {
            cliPrintf("0x%08X : ", addr + i);

            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length)
                {
                    uint32_t target_addr = addr + i + j;

                    // Bus Fault 방지용 메모리 맵 유효성 검사
                    if (isSafeAddress(target_addr))
                    {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        cliPrintf("%02X ", val);
                    }
                    else
                    {
                        // 읽기 금지 구역은 ?? 로 출력
                        cliPrintf("?? ");
                    }
                }
                else
                {
                    cliPrintf("   ");
                }
            }

            cliPrintf(" | ");

            for (uint32_t j = 0; j < 16; j++)
            {
                if (i + j < length)
                {
                    uint32_t target_addr = addr + i + j;
                    if (isSafeAddress(target_addr))
                    {
                        uint8_t val = *((volatile uint8_t *)target_addr);
                        if (val >= 32 && val <= 126)
                            cliPrintf("%c", val);
                        else
                            cliPrintf(".");
                    }
                    else
                    {
                        cliPrintf(".");
                    }
                }
            }
            cliPrintf("\r\n");
        }
    }
    else
    {
        cliPrintf("Usage: md [addr(hex)] [length]\r\n");
        cliPrintf("       md 08000000 32\r\n");
    }
}
