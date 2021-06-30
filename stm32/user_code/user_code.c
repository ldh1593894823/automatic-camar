#include "user_code.h"
#include "oled.h"
#include "rtthread.h"
#include "tim.h"
#include "usbd_cdc_if.h"
#include <stdint.h>
#include "user_code.h"

typedef struct //接收串口数据
{
    uint16_t now_x;
    uint16_t now_y;
    float    p;
    uint8_t  led;

} position;

typedef struct // PID 存放 X 轴
{
    uint16_t P; //设定P值
    uint16_t I; //设定i值
    uint16_t D; //设定D值

    int16_t ErrNow; //当前误差
    int16_t ErrUP;  //上次误差

    uint16_t PWM; //电机PWM
} PIDX;

typedef struct // PID 存放 Y 轴
{
    uint16_t P; //设定P值
    uint16_t I; //设定i值
    uint16_t D; //设定D值

    int16_t ErrNow; //当前误差
    int16_t ErrUP;  //上次误差

    uint16_t PWM; //电机PWM
} PIDY;

position data       = {0};
PIDX     pid_data_x = {0};
PIDY     pid_data_y = {0};

static rt_sem_t dynamic_sem_PID = RT_NULL; //定义一个信号量 释放PID计算函数
static rt_sem_t dynamic_sem_LED = RT_NULL; //定义一个信号量 释放LED控制函数
uint16_t        pwmdatas[290];         // WS2812数组

void user_code_init();
void usb_receive(uint8_t* Buf, uint32_t* Len);
void fill_pwm_data(uint16_t* pwm_data, uint8_t r, uint8_t g, uint8_t b);

void PID_X(void* argv)
{
    while (1)
    {
        rt_sem_take(dynamic_sem_PID, RT_WAITING_FOREVER); //等待释放信号量，用来计算PID参数
        pid_data_x.PWM = pid_data_x.PWM - (data.p * (300 - data.now_x) * -1);
        // pid_data_x.ErrUP  = pid_data_x.ErrNow;
        // pid_data_x.ErrNow = 300 - data.now_x;
        TIM2->CCR2 = pid_data_x.PWM;
    }
}

void PID_Y(void* argv)
{
    while (1)
    {
        rt_sem_take(dynamic_sem_PID, RT_WAITING_FOREVER);
        pid_data_y.PWM = pid_data_y.PWM + (data.p * (250 - data.now_y) * -1);
        TIM2->CCR1     = pid_data_y.PWM;
    }
}

void oled(void* argv)
{
    OLED_ShowCHinese(0, 0, 0);
    OLED_ShowCHinese(16, 0, 1);
    OLED_ShowString(0, 2, "x", 16);
    OLED_ShowString(0, 4, "y", 16);
    while (1)
    {
        // CDC_Transmit_FS("0XFF",1);
        OLED_ShowNum(50, 4, pid_data_x.PWM, 5, 20); //显示当前PWM
        OLED_ShowNum(50, 4, pid_data_y.PWM, 5, 20); //显示当前误差
        if (pid_data_x.PWM > 1260)
        {
            pid_data_x.PWM = 750;
            pid_data_y.PWM = 750;
        }
    }
}

void WS2812(void* argv)
{
    while (1)
    {
        rt_sem_take(dynamic_sem_LED, RT_WAITING_FOREVER);
        fill_pwm_data(pwmdatas, 255, 255, 255);
        rt_thread_delay(500);
        fill_pwm_data(pwmdatas, 0, 0, 0);
        rt_thread_delay(50);
        fill_pwm_data(pwmdatas, 255, 255, 255);
        rt_thread_delay(100);
        fill_pwm_data(pwmdatas, 0, 0, 0);
        rt_thread_delay(100);
        fill_pwm_data(pwmdatas, 255, 255, 255);
        rt_thread_delay(100);
        fill_pwm_data(pwmdatas, 0, 0, 0);
        rt_thread_delay(100);

        fill_pwm_data(pwmdatas, 255, 255, 255);
        data.led = 0;
    }
}

void user_code()
{
    user_code_init(); //代码初始化

    rt_thread_startup(rt_thread_create("oled", oled, NULL, 3072, 15, 10));     // oled线程
    rt_thread_startup(rt_thread_create("PID_X", PID_X, NULL, 2048, 15, 10));   // PID_X线程
    rt_thread_startup(rt_thread_create("PID_Y", PID_Y, NULL, 2048, 15, 10));   // PID_Y线程
    rt_thread_startup(rt_thread_create("WS2812", WS2812, NULL, 4096, 15, 10)); // WS2812 代码
    rt_system_scheduler_start();
}

void user_code_init()
{

    OLED_Init();  // OLED屏幕初始化
    OLED_Clear(); // OLED屏幕清屏

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); //开启舵机通道
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); //开启舵机通道

    pid_data_x.PWM = 750; //初始化舵机位置
    pid_data_y.PWM = 800; //初始化舵机位置

    TIM2->CCR2 = 750; // 250-1250  X
    TIM2->CCR1 = 750; // 250-1250  Y

    dynamic_sem_PID = rt_sem_create("lala", 0, RT_IPC_FLAG_FIFO); //初始化信号量 用来控制PID参数
    dynamic_sem_LED = rt_sem_create("ws2812",0,RT_IPC_FLAG_FIFO); //初始化信号量 用来控制LED参数
}

void usb_receive(uint8_t* Buf, uint32_t* Len)
{
    position* ho = (position*)Buf;
    data.now_y   = ho->now_y;
    data.now_x   = ho->now_x;
    data.p       = ho->p;
    data.led     = ho->led;
    if (data.led)
    {
        rt_sem_release(dynamic_sem_LED);
    }
    
    rt_sem_release(dynamic_sem_PID); //释放一个信号量，用来计算PID参数
}

void fill_pwm_data(uint16_t* pwm_data, uint8_t r, uint8_t g, uint8_t b)
{
    *pwm_data++ = 0;
    for (uint32_t i = 0; i < 12; i++)
    {
        for (uint32_t j = 0; j < 8; j++)
            *pwm_data++ = (0x80 >> j) & r ? 60 : 29;
        for (uint32_t j = 0; j < 8; j++)
            *pwm_data++ = (0x80 >> j) & g ? 60 : 29;
        for (uint32_t j = 0; j < 8; j++)
            *pwm_data++ = (0x80 >> j) & b ? 60 : 29;
    }
    *pwm_data++ = 0;
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, (uint32_t*)pwmdatas, 290);
}