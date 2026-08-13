/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "protocol.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FLAG_COMMS 0x01
#define FLAG_TELEM 0x02
#define FLAG_ALL   (FLAG_COMMS | FLAG_TELEM)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for CommsTask */
osThreadId_t CommsTaskHandle;
const osThreadAttr_t CommsTask_attributes = {
  .name = "CommsTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for TelemetryTask */
osThreadId_t TelemetryTaskHandle;
const osThreadAttr_t TelemetryTask_attributes = {
  .name = "TelemetryTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for HousekeepingTas */
osThreadId_t HousekeepingTasHandle;
const osThreadAttr_t HousekeepingTas_attributes = {
  .name = "HousekeepingTas",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for telemQueue */
osMessageQueueId_t telemQueueHandle;
const osMessageQueueAttr_t telemQueue_attributes = {
  .name = "telemQueue"
};
/* Definitions for WatchdogEvents */
osEventFlagsId_t WatchdogEventsHandle;
const osEventFlagsAttr_t WatchdogEvents_attributes = {
  .name = "WatchdogEvents"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void *argument);
void StartTask03(void *argument);
void StartTask04(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  // Print our startup sequence over USART1
  printf("\r\n======================================\r\n");
  printf("  VELA FLIGHT OBC - BOOT SEQUENCE\r\n");
  printf("======================================\r\n");
  printf("[INIT] Hardware Peripherals OK.\r\n");
  printf("[INIT] Starting FreeRTOS Kernel...\r\n");
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Create the queue(s) */
  telemQueueHandle = osMessageQueueNew (16, 12, &telemQueue_attributes);

  /* Create the thread(s) */
  CommsTaskHandle = osThreadNew(StartDefaultTask, NULL, &CommsTask_attributes);
  TelemetryTaskHandle = osThreadNew(StartTask03, NULL, &TelemetryTask_attributes);
  HousekeepingTasHandle = osThreadNew(StartTask04, NULL, &HousekeepingTas_attributes);

  /* Create the event(s) */
  WatchdogEventsHandle = osEventFlagsNew(&WatchdogEvents_attributes);

  /* Start scheduler */
  osKernelStart();

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
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

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  */
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

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_BLUE_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/**
 * @brief Retargets the C library printf function to the USART1 port.
 */
int _write(int file, char *ptr, int len)
{
    // Transmit the characters over UART1
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  uint8_t sync_buf[2];
  TelemetryFrame rx_frame;

  for(;;)
  {
      osEventFlagsSet(WatchdogEventsHandle, FLAG_COMMS);

      if (HAL_UART_Receive(&huart2, &sync_buf[0], 1, 10) == HAL_OK) {
          if (sync_buf[0] == 0x55) {
              if (HAL_UART_Receive(&huart2, &sync_buf[1], 1, 10) == HAL_OK) {
                  if (sync_buf[1] == 0xAA) {
                      uint8_t *frame_ptr = (uint8_t*)&rx_frame;
                      rx_frame.sync = 0xAA55;
                      if (HAL_UART_Receive(&huart2, frame_ptr + 2, 15, 100) == HAL_OK) {
                          osMessageQueuePut(telemQueueHandle, &rx_frame.payload, 0, osWaitForever);
                      }
                  }
              }
          }
      }
      osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
  TelemetryPayload current_telem;

  for(;;)
  {
      if (osMessageQueueGet(telemQueueHandle, &current_telem, NULL, osWaitForever) == osOK) {
          osEventFlagsSet(WatchdogEventsHandle, FLAG_TELEM);

          // Log the successful reception (Printing the integer radiation count)
          printf("[TELEM] Frame OK. Radiation Hits: %lu\r\n", current_telem.radiation);

          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      }
  }
  /* USER CODE END StartTask03 */
}

/* USER CODE BEGIN Header_StartTask04 */
void StartTask04(void *argument)
{
  /* USER CODE BEGIN StartTask04 */
  uint32_t flags;

  for(;;)
  {
      flags = osEventFlagsWait(WatchdogEventsHandle, FLAG_ALL, osFlagsWaitAll, 2000);

      if (flags == (uint32_t)osErrorTimeout)
      {
          // LOG THE CRITICAL HARDWARE FAILURE
          printf("\r\n======================================\r\n");
          printf("[FATAL] WATCHDOG TIMEOUT DETECTED!\r\n");
          printf("[FATAL] A system task has stopped responding.\r\n");
          printf("[FATAL] Triggering emergency hardware reset...\r\n");
          printf("======================================\r\n\r\n");

          for(int i = 0; i < 20; i++) {
              HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
              HAL_Delay(50);
          }
          NVIC_SystemReset();
      }
      osDelay(500);
  }
  /* USER CODE END StartTask04 */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM11) { HAL_IncTick(); }
}
void Error_Handler(void) { __disable_irq(); while (1) {} }
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif

