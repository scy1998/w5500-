/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/*例程网络参数*/
//网关�????????????192.168.1.1
//掩码:	255.255.255.0
//物理地址�????????????0C 29 AB 7C 00 01
//本机IP地址:192.168.1.199
//端口0的端口号�????????????5000
//端口0的目的IP地址�????????????192.168.1.190
//端口0的目的端口号�????????????6000

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */


//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
//	if(htim->Instance == TIM2){
//
//	Timer2_Counter++;
//	W5500_Send_Delay_Counter++;
//	}
//}
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
unsigned int Timer2_Counter=0; //定时器计数变�???????????(s)
unsigned int W5500_Send_Delay_Counter=0; //w5500发�?�延时计数变�???????????(s)
uint8_t  buf[] = "hello";


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//GPIO外部中断处理回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == GPIO_PIN_6){
		W5500_Interrupt=1;
	}
}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
//	if(huart->Instance = huart4){
//		Write_SOCK_Data_Buffer(0, Tx_Buffer, 23)
//	}
//}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
//{
//    HAL_UART_Transmit(&huart4, (uint8_t *)aRxBuffer, 1,0xFFFF);
//    HAL_UART_Receive_IT(&huart4, (uint8_t *)aRxBuffer, 1);
//}



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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI3_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
//  HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)&htim2); //�???????????启定时器2中断

  printf("uart init success\n\r");

  Load_Net_Parameters();		//装载网络参数
  	W5500_Hardware_Reset();		//硬件复位W5500
  	W5500_Initialization();		//W5500初始货配�????????????
  	printf("w5500 init success\n\r");
  	W5500_Socket_Set();//W5500端口初始化配�?

  	__HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);
  	//__HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
  	__HAL_UART_CLEAR_IDLEFLAG(&huart4);
  	HAL_UART_Receive_DMA(&huart4, DMA_Buffer, DMA_BUFFER_LENGTH);
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//	  HAL_Delay(2000);
//	  HAL_UART_Transmit(&huart4, buf, sizeof(buf), 200);


//	  		if(W5500_Interrupt)//处理W5500中断
//	  		{
//	  			W5500_Interrupt_Process();//W5500中断处理程序框架
//	  		}
//	  		if((S0_Data & S_RECEIVE) == S_RECEIVE)//如果Socket0接收到数�?
//	  		{
//	  			S0_Data&=~S_RECEIVE;
//	  			Process_Socket_Data(0);//W5500接收并发送接收到的数�?
//	  		}



//	  		HAL_Delay(1000);
//	  			if(S0_State == (S_INIT|S_CONN))
//	  			{
//	  				S0_Data&=~S_TRANSMITOK;
//	  				memcpy(Tx_Buffer, "Welcome To NiRenElec!\r\n", 23);
//	  				Write_SOCK_Data_Buffer(0, Tx_Buffer, 23);//指定Socket(0~7)发�?�数据处�?,端口0发�??23字节数据
//	  				printf("data send success!\n\r");
//	  			}



  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 12;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
