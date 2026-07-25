#include "stm32f10x.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
#include "dht11.h"
#include "oled.h"
#include "buzzer.h"
#include "pwm.h"
#include "relay.h"
#include "key.h"

u32 delay_get_ms(void);

#define DOG_SMALL_MODE_COUNTDOWN 1200U
#define DOG_SMALL_MODE_ALARM_TEMP 34U
#define DOG_SMALL_MODE_PWM_DUTY 60U
#define DOG_MEDIUM_MODE_COUNTDOWN 1500U
#define DOG_MEDIUM_MODE_ALARM_TEMP 36U
#define DOG_MEDIUM_MODE_PWM_DUTY 80U
#define DOG_LARGE_MODE_COUNTDOWN 1800U
#define DOG_LARGE_MODE_ALARM_TEMP 38U
#define DOG_LARGE_MODE_PWM_DUTY 100U
#define CAT_SMALL_MODE_COUNTDOWN 1000U
#define CAT_SMALL_MODE_ALARM_TEMP 32U
#define CAT_SMALL_MODE_PWM_DUTY 50U
#define CAT_MEDIUM_MODE_COUNTDOWN 1200U
#define CAT_MEDIUM_MODE_ALARM_TEMP 34U
#define CAT_MEDIUM_MODE_PWM_DUTY 70U
#define CAT_LARGE_MODE_COUNTDOWN 1400U
#define CAT_LARGE_MODE_ALARM_TEMP 36U
#define CAT_LARGE_MODE_PWM_DUTY 90U

#define PET_SENSOR_GPIO_PORT GPIOB
#define PET_SENSOR_GPIO_PIN GPIO_Pin_12
#define PET_SENSOR_GPIO_CLK RCC_APB2Periph_GPIOB
#define PET_SENSOR_ABSENCE_TIMEOUT_MS 5000U

static u8 temp1 = 0;
static u8 humi1 = 0;
static u8 temp2 = 0;
static u8 humi2 = 0;
static u8 alarm_temp = 35;
static u8 fan_pwm_duty = 0;
static u32 countdown = 60;
static u8 running = 0;
static u8 sensor_data_valid = 0;
static u8 sensor_fault_alarm = 0;
static u8 pet_monitor_enabled = 0;
static u8 pet_present = 0;
static u8 pet_status_known = 0;
static u8 pet_absence_alarm_triggered = 0;
static u16 pet_absence_elapsed_ms = 0;

static void PetSensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(PET_SENSOR_GPIO_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = PET_SENSOR_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(PET_SENSOR_GPIO_PORT, &GPIO_InitStructure);
}

static u8 PetSensor_IsPresent(void)
{
    return GPIO_ReadInputDataBit(PET_SENSOR_GPIO_PORT, PET_SENSOR_GPIO_PIN) ? 0 : 1;
}

static const char *PetStatusText(void)
{
    if(!pet_status_known)
    {
        return "unknown";
    }

    return pet_present ? "present" : "absent";
}

static void StopDryMode(u8 enable_alarm, const char *reason)
{
    running = 0;
    PWM_SetDuty(0);
    Relay_On();

    if(enable_alarm)
    {
        Buzzer_On();
    }
    else
    {
        Buzzer_Off();
    }

    printf("%s\r\n", reason);
}

static void StartDryMode(u32 mode_countdown, u8 mode_alarm_temp, u8 mode_pwm_duty, const char *mode_name)
{
    if(!sensor_data_valid)
    {
        StopDryMode(1, "Start rejected: temperature sensor data is invalid.");
        return;
    }

    countdown = mode_countdown;
    alarm_temp = mode_alarm_temp;
    fan_pwm_duty = mode_pwm_duty;
    running = 1;
    sensor_fault_alarm = 0;
    pet_absence_elapsed_ms = 0;
    pet_absence_alarm_triggered = 0;
    Buzzer_Off();
    Relay_Off();
    PWM_SetDuty(fan_pwm_duty);
    printf("%s dry mode: countdown=%lus, alarm_temp=%d, pwm=%d\r\n", mode_name, (unsigned long)countdown, alarm_temp, fan_pwm_duty);
}

static void ApplyKeyAction(Key_TypeDef key)
{
    switch(key)
    {
        case KEY_UP:
            if(countdown <= 0xFFFFFFFFUL - 60U)
            {
                countdown += 60U;
            }
            printf("Key UP: countdown +60s = %lu\r\n", (unsigned long)countdown);
            break;
        case KEY_DOWN:
            if(countdown > 60U)
            {
                countdown -= 60U;
            }
            printf("Key DOWN: countdown -60s = %lu\r\n", (unsigned long)countdown);
            break;
        case KEY_TEMP_UP:
            if(alarm_temp < 50U)
            {
                alarm_temp++;
            }
            printf("Key TEMP UP: alarm_temp = %d\r\n", alarm_temp);
            break;
        case KEY_TEMP_DOWN:
            if(alarm_temp > 20U)
            {
                alarm_temp--;
            }
            printf("Key TEMP DOWN: alarm_temp = %d\r\n", alarm_temp);
            break;
        default:
            break;
    }
}

static void ProcessUsartCommand(USART1_Command usart_cmd)
{
    switch(usart_cmd)
    {
        case USART1_CMD_DOG_SMALL:
            StartDryMode(DOG_SMALL_MODE_COUNTDOWN, DOG_SMALL_MODE_ALARM_TEMP, DOG_SMALL_MODE_PWM_DUTY, "Small dog");
            break;
        case USART1_CMD_DOG_MEDIUM:
            StartDryMode(DOG_MEDIUM_MODE_COUNTDOWN, DOG_MEDIUM_MODE_ALARM_TEMP, DOG_MEDIUM_MODE_PWM_DUTY, "Medium dog");
            break;
        case USART1_CMD_DOG_LARGE:
            StartDryMode(DOG_LARGE_MODE_COUNTDOWN, DOG_LARGE_MODE_ALARM_TEMP, DOG_LARGE_MODE_PWM_DUTY, "Large dog");
            break;
        case USART1_CMD_CAT_SMALL:
            StartDryMode(CAT_SMALL_MODE_COUNTDOWN, CAT_SMALL_MODE_ALARM_TEMP, CAT_SMALL_MODE_PWM_DUTY, "Small cat");
            break;
        case USART1_CMD_CAT_MEDIUM:
            StartDryMode(CAT_MEDIUM_MODE_COUNTDOWN, CAT_MEDIUM_MODE_ALARM_TEMP, CAT_MEDIUM_MODE_PWM_DUTY, "Medium cat");
            break;
        case USART1_CMD_CAT_LARGE:
            StartDryMode(CAT_LARGE_MODE_COUNTDOWN, CAT_LARGE_MODE_ALARM_TEMP, CAT_LARGE_MODE_PWM_DUTY, "Large cat");
            break;
        case USART1_CMD_KEY_UP:
            ApplyKeyAction(KEY_UP);
            break;
        case USART1_CMD_KEY_DOWN:
            ApplyKeyAction(KEY_DOWN);
            break;
        case USART1_CMD_TEMP_UP:
            ApplyKeyAction(KEY_TEMP_UP);
            break;
        case USART1_CMD_TEMP_DOWN:
            ApplyKeyAction(KEY_TEMP_DOWN);
            break;
        case USART1_CMD_SET_PWM:
            fan_pwm_duty = USART1_GetPwmDuty();
            if(fan_pwm_duty > 100U)
            {
                fan_pwm_duty = 100U;
            }
            if(running)
            {
                PWM_SetDuty(fan_pwm_duty);
                printf("Manual PWM set to %d\r\n", fan_pwm_duty);
            }
            else
            {
                printf("Manual PWM saved as %d\r\n", fan_pwm_duty);
            }
            break;
        case USART1_CMD_PET_MONITOR_ON:
            pet_monitor_enabled = 1;
            pet_absence_elapsed_ms = 0;
            pet_absence_alarm_triggered = 0;
            printf("Pet monitor ON\r\n");
            break;
        case USART1_CMD_PET_MONITOR_OFF:
            pet_monitor_enabled = 0;
            pet_absence_elapsed_ms = 0;
            printf("Pet monitor OFF\r\n");
            break;
        default:
            break;
    }
}

static void UpdateSensors(void)
{
    u8 next_temp1;
    u8 next_humi1;
    u8 next_temp2;
    u8 next_humi2;
    u8 sensor1_ok;
    u8 sensor2_ok;

    sensor1_ok = (DHT11_Read_Data(1, &next_temp1, &next_humi1) == 0U);
    sensor2_ok = (DHT11_Read_Data(2, &next_temp2, &next_humi2) == 0U);

    if(sensor1_ok)
    {
        temp1 = next_temp1;
        humi1 = next_humi1;
    }
    else
    {
        printf("DHT11_1 Read Error\r\n");
    }

    if(sensor2_ok)
    {
        temp2 = next_temp2;
        humi2 = next_humi2;
    }
    else
    {
        printf("DHT11_2 Read Error\r\n");
    }

    sensor_data_valid = sensor1_ok && sensor2_ok;
    if(!sensor_data_valid && running)
    {
        sensor_fault_alarm = 1;
        StopDryMode(1, "Sensor fault: heater and fan stopped.");
    }

    printf(
        "dht_1: temp=%d humi=%d | dht_2: temp=%d humi=%d | countdown=%lus | alarm=%d | pwm=%d | pet=%s | monitor=%s\r\n",
        temp1,
        humi1,
        temp2,
        humi2,
        (unsigned long)countdown,
        alarm_temp,
        PWM_GetDuty(),
        PetStatusText(),
        pet_monitor_enabled ? "on" : "off"
    );
    LED_Toggle();
}

static void UpdateControl(void)
{
    if(!running || !sensor_data_valid)
    {
        return;
    }

    if(temp1 > alarm_temp || temp2 > alarm_temp)
    {
        Relay_On();
    }
    else
    {
        Relay_Off();
    }

    if(temp1 > alarm_temp + 5U || temp2 > alarm_temp + 5U)
    {
        Buzzer_On();
    }
    else if(!sensor_fault_alarm && !pet_absence_alarm_triggered)
    {
        Buzzer_Off();
    }
}

static void UpdateCountdown(void)
{
    if(running && countdown > 0U)
    {
        countdown--;
        if(countdown == 0U)
        {
            StopDryMode(0, "Time's up! Heater and fan stopped.");
        }
    }
}

static void UpdatePetMonitor(void)
{
    pet_present = PetSensor_IsPresent();
    pet_status_known = 1;

    if(!pet_monitor_enabled || !running)
    {
        pet_absence_elapsed_ms = 0;
        return;
    }

    if(pet_present)
    {
        pet_absence_elapsed_ms = 0;
        return;
    }

    if(pet_absence_elapsed_ms < PET_SENSOR_ABSENCE_TIMEOUT_MS)
    {
        pet_absence_elapsed_ms += 100U;
    }

    if(pet_absence_elapsed_ms >= PET_SENSOR_ABSENCE_TIMEOUT_MS && !pet_absence_alarm_triggered)
    {
        pet_absence_alarm_triggered = 1;
        StopDryMode(1, "Pet absence alert! Heater and fan stopped.");
    }
}

static void UpdateDisplay(void)
{
    OLED_ShowNum(24, 0, temp1, 2, 16, 1);
    OLED_ShowNum(96, 0, temp2, 2, 16, 1);
    OLED_ShowNum(48, 16, countdown, 4, 16, 1);
    OLED_ShowNum(48, 32, alarm_temp, 2, 16, 1);

    if(Relay_GetStatus())
    {
        OLED_ShowString(64, 48, (u8 *)"ON  ", 16, 1);
    }
    else
    {
        OLED_ShowString(64, 48, (u8 *)"OFF ", 16, 1);
    }
}

static void Hardware_Init(void)
{
    SystemInit();
    delay_init(72);
    LED_Init();
    LED_On();
    USART1_Config();
    OLED_Init();
    Buzzer_Init();
    PWM_Init();
    Relay_Init();
    Key_Init();
    PetSensor_Init();

    printf("Start\r\n");
    while(DHT11_Init(1))
    {
        printf("DHT11_1 Error\r\n");
        delay_ms(1000);
    }
    printf("DHT11_1 OK\r\n");

    while(DHT11_Init(2))
    {
        printf("DHT11_2 Error\r\n");
        delay_ms(1000);
    }
    printf("DHT11_2 OK\r\n");
    printf("USART1 commands: dog_small, dog_medium, dog_large, cat_small, cat_medium, cat_large, key_up, key_down, temp_up, temp_down, pwm=0..100, pet_on, pet_off\r\n");

    OLED_ShowString(0, 0, (u8 *)"1:", 16, 1);
    OLED_ShowChar(24, 0, 'C', 16, 1);
    OLED_ShowString(48, 0, (u8 *)"2:", 16, 1);
    OLED_ShowChar(72, 0, 'C', 16, 1);
    OLED_ShowString(0, 16, (u8 *)"Time:", 16, 1);
    OLED_ShowString(0, 32, (u8 *)"TEMP:", 16, 1);
    OLED_ShowChar(72, 32, 'C', 16, 1);
    OLED_ShowString(0, 48, (u8 *)"RELAY:", 16, 1);
}

int main(void)
{
    u32 now;
    u32 last_sensor;
    u32 last_key;
    u32 last_control;
    u32 last_countdown;
    u32 last_pet_monitor;
    u32 last_display;
    Key_TypeDef key;
    USART1_Command usart_cmd;

    Hardware_Init();
    UpdateSensors();
    now = delay_get_ms();
    last_sensor = now;
    last_key = now;
    last_control = now;
    last_countdown = now;
    last_pet_monitor = now;
    last_display = now;

    while(1)
    {
        now = delay_get_ms();

        usart_cmd = USART1_GetCommand();
        if(usart_cmd != USART1_CMD_NONE)
        {
            ProcessUsartCommand(usart_cmd);
        }

        if((u32)(now - last_key) >= 50U)
        {
            last_key = now;
            key = Key_Scan();
            if(key != KEY_NONE)
            {
                ApplyKeyAction(key);
            }
        }

        if((u32)(now - last_pet_monitor) >= 100U)
        {
            last_pet_monitor = now;
            UpdatePetMonitor();
        }

        if((u32)(now - last_control) >= 100U)
        {
            last_control = now;
            UpdateControl();
        }

        if((u32)(now - last_display) >= 100U)
        {
            last_display = now;
            UpdateDisplay();
        }

        if((u32)(now - last_countdown) >= 1000U)
        {
            last_countdown += 1000U;
            UpdateCountdown();
        }

        if((u32)(now - last_sensor) >= 1000U)
        {
            last_sensor += 1000U;
            UpdateSensors();
        }

        delay_ms(1);
    }
}
