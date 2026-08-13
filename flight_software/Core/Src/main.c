/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - VELA FLIGHT SOFTWARE (Phase 8)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "protocol.h" /* Auto-generated from Phase 7 */
#include <string.h>

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Definitions for FreeRTOS Tasks */
osThreadId_t commsTaskHandle;
const osThreadAttr_t commsTask_attributes = {
  .name = "commsTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t telemetryTaskHandle;
const osThreadAttr_t telemetryTask_attributes = {
  .name = "telemetryTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t housekeepingTaskHandle;
const osThreadAttr_t housekeepingTask_attributes = {
  .name = "housekeepingTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for FreeRTOS Queue */
osMessageQueueId_t telemQueueHandle;
const osMessageQueueAttr_t telemQueue_attributes = {
  .name = "telemQueue"
};

/* Watchdog Flags */
volatile uint8_t commsAlive = 0;
volatile uint8_t telemetryAlive = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartCommsTask(void *argument);
void StartTelemetryTask(void *argument);
void StartHousekeepingTask(void *argument);

/* USER CODE BEGIN PFP */
/**
 * @brief  Configures the Memory Protection Unit (MPU).
 *         Creates a strict NO-ACCESS zone to catch rogue pointers.
 */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disable MPU before configuring */
  HAL_MPU_Disable();

  /* Set up a memory trap region (Simulating a restricted memory zone) */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x20010000; /* Define a region in SRAM */
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS; /* CRASH ON ACCESS */
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable MPU with default access permitted to other areas */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
/* USER CODE END PFP */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* PHASE 8: Activate Hardware Memory Protection immediately on boot */
  MPU_Config();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* Init scheduler */
  osKernelInitialize();

  /* Create the queue(s) */
  telemQueueHandle = osMessageQueueNew (5, sizeof(TelemetryFrame_t), &telemQueue_attributes);

  /* Create the thread(s) */
  commsTaskHandle = osThreadNew(StartCommsTask, NULL, &commsTask_attributes);
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);
  housekeepingTaskHandle = osThreadNew(StartHousekeepingTask, NULL, &housekeepingTask_attributes);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
  {
  }
}

/* ------------------------------------------------------------------------- */
/*                             FREERTOS TASKS                                */
/* ------------------------------------------------------------------------- */

void StartCommsTask(void *argument)
{
  uint8_t rxByte;
  TelemetryFrame_t frame;

  for(;;)
  {
    /* Read byte by byte hunting for sync */
    if (HAL_UART_Receive(&huart2, &rxByte, 1, 10) == HAL_OK)
    {
      if (rxByte == 0x55) /* First Sync Byte */
      {
        if (HAL_UART_Receive(&huart2, &rxByte, 1, 10) == HAL_OK && rxByte == 0xAA) /* Second Sync Byte */
        {
          /* Sync locked. Read remainder of frame */
          frame.sync = 0xAA55;
          HAL_UART_Receive(&huart2, (uint8_t*)&frame.type, sizeof(TelemetryFrame_t) - 2, 50);
          
          /* Push to Queue for processing */
          osMessageQueuePut(telemQueueHandle, &frame, 0, 0);
          
          /* Report to Watchdog */
          commsAlive = 1;
        }
      }
    }
    osDelay(1); /* Yield to prevent CPU hogging */
  }
}

void StartTelemetryTask(void *argument)
{
  TelemetryFrame_t rxFrame;

  for(;;)
  {
    /* Wait for data to arrive in the queue */
    if (osMessageQueueGet(telemQueueHandle, &rxFrame, NULL, osWaitForever) == osOK)
    {
      /* Toggle LED on STM32 Black Pill (PC13) to indicate successful processing */
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      
      /* Report to Watchdog */
      telemetryAlive = 1;
    }
  }
}

void StartHousekeepingTask(void *argument)
{
  for(;;)
  {
    /* Wait 2 seconds between watchdog checks */
    osDelay(2000);

    if (commsAlive == 0 || telemetryAlive == 0)
    {
      /* FAULT DETECTED: A task starved or froze */
      /* Flash SOS Pattern */
      for (int i = 0; i < 20; i++)
      {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(50); /* Blocking delay during fault condition */
      }
      /* Hard Reboot */
      NVIC_SystemReset();
    }

    /* Reset flags for next cycle */
    commsAlive = 0;
    telemetryAlive = 0;
  }
}

/* ------------------------------------------------------------------------- */
/*                       AUTO-GENERATED CUBEIDE BOILERPLATE                  */
/* ------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
  /* Note: Ensure this matches your STM32F401 Black Pill clock config. 
     If your board crashes immediately, swap this function back to your original auto-generated one! */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}