/**********************************************************************************
 * 文件名  ：W5500.c
 * 描述    ：W5500 驱动函数库         
 * 库版本  ：ST_v3.5

 * 淘宝    ：http://yixindianzikeji.taobao.com/
**********************************************************************************/

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_spi.h"		

#include "stm32l4xx_hal_exti.h"	
#include "w5500.h"
#include "string.h"
#include "spi.h"
//#include "log.h"



/*网络参数定义*/
unsigned char Gateway_IP[4];//网关ip
unsigned char Sub_Mask[4];	//子网掩码
unsigned char Phy_Addr[6];	//物理地址
unsigned char IP_Addr[4];	//本机ip地址

unsigned char S0_Port[2];	//端口0的端口号(5000)
unsigned char S0_DIP[4];	// 端口0的目的ip地址
unsigned char S0_DPort[2];	//端口0的目的端口号 (6000)

unsigned char UDP_DIPR[4];	//UDP(¹ã²¥)Ä£Ê½,Ä¿µÄÖ÷»úIPµØÖ·
unsigned char UDP_DPORT[2];	//UDP(¹ã²¥)Ä£Ê½,Ä¿µÄÖ÷»ú¶Ë¿ÚºÅ



/***************----- 端口的运行模式 -----***************/
unsigned char S0_Mode = UDP_MODE;	//端口0的运行模式,0:TCP服务器模式,1:TCP客户端模式,2:UDP(广播)模式
#define TCP_SERVER	0x00	//TCP服务器模式
#define TCP_CLIENT	0x01	//TCP客户端模式 
#define UDP_MODE	0x02	//UDP(广播)模式 


/***************----- 端口的运行状态 -----***************/
unsigned char S0_State =0;	//端口0状态记录,1:端口完成初始化,2端口完成连接(可以正常传输数据) 
#define S_INIT		0x01	//端口完成初始化 
#define S_CONN		0x02	//端口完成连接,可以正常传输数据 

/***************----- 端口收发数据的状态 -----***************/
unsigned char S0_Data;		//端口0接收和发送数据的状态,1:端口接收到数据,2:端口发送数据完成 
#define S_RECEIVE	 0x01	//端口接收到一个数据包 
#define S_TRANSMITOK 0x02	//端口发送一个数据包完成 
unsigned char S1_State =0;
unsigned char S1_Mode = UDP_MODE;
unsigned char S1_Data;

unsigned char S2_State =0;
unsigned char S2_Mode = UDP_MODE;
unsigned char S2_Data;

/***************----- 端口数据缓冲区 -----***************/
unsigned char Rx_Buffer[2048];	//端口接收数据缓冲区 
unsigned char Tx_Buffer[2048];	//端口发送数据缓冲区 

unsigned char W5500_Interrupt=0;	//W5500中断标志(0:无中断,1:有中断)
uint8_t Rec_Flag=0;//接收到数据的标志(0：未接收到，1：接收到)


extern DMA_HandleTypeDef hdma_spi3_tx;
unsigned int global_offset1;
unsigned short global_size;



void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

void SPI_WriteByte(uint8_t Send)
{ 
HAL_SPI_Transmit(&hspi3,&Send,1,0xffff);
while(HAL_SPI_GetState(&hspi3) == HAL_SPI_STATE_BUSY_RX);
SPI3->DR;
}

void SPI_WriteNBytes(uint8_t *Send,uint16_t len)
{ 
HAL_SPI_Transmit(&hspi3,Send,len,0xffff);
while(HAL_SPI_GetState(&hspi3) == HAL_SPI_STATE_BUSY_RX);
SPI3->DR;
}

uint8_t SPI_ReadByte(void)
{ 
uint8_t Rcv=0;
HAL_SPI_Receive(&hspi3,&Rcv,1,0XFFFF);
return Rcv;
}
void SPI_ReadNByte(uint8_t *buf,uint16_t len)
{ 
HAL_SPI_Receive(&hspi3,buf,len,0XFFFF);
return;
}

void SPI_CrisEnter(void)
{
	__set_PRIMASK(1);
}

void SPI_CRISExit(void)
{
	__set_PRIMASK(0);
}

void SPI_CS_Select(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
}
void SPI_CS_Deselect(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_RESET);
}
void W5500_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch(GPIO_Pin)
	{

		case W5500_INT:
		{
			W5500_Interrupt=1;
			break;
		}
		default:
			break;
	}
}

/*******************************************************************************
* 函数名  : W5500_GPIO_Configuration
* 描述    : W5500 GPIO初始化配置
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
//void W5500_GPIO_Configuration(void)
//{
//	GPIO_InitTypeDef  GPIO_InitStructure;
//	EXTI_InitTypeDef  EXTI_InitStructure;	

//	/* W5500_RST引脚初始化配置(PC5) */
//	GPIO_InitStructure.GPIO_Pin  = W5500_RST;
//	GPIO_InitStructure.GPIO_Speed=GPIO_SPEED_FREQ_MEDIUM;
//	GPIO_InitStructure.GPIO_Mode = GPIO_NOPULL;
//	GPIO_Init(W5500_RST_PORT, &GPIO_InitStructure);
//	GPIO_ResetBits(W5500_RST_PORT, W5500_RST);
//	
//	/* W5500_INT引脚初始化配置(PC4) */	
//	GPIO_InitStructure.GPIO_Pin = W5500_INT;
//	GPIO_InitStructure.GPIO_Speed=GPIO_SPEED_FREQ_HIGH;
//	GPIO_InitStructure.GPIO_Mode = GPIO_PULLUP;
//	GPIO_Init(W5500_INT_PORT, &GPIO_InitStructure);
//		
//	/* Connect EXTI Line4 to PC4 */
//	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

//	/* PC4 as W5500 interrupt input */
//	EXTI_InitStructure.EXTI_Line = EXTI_LINE_4;
//	EXTI_InitStructure.EXTI_Mode = EXTI_MODE_INTERRUPT;
//	EXTI_InitStructure.EXTI_Trigger = EXTI_TRIGGER_FALLING;
//	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//	EXTI_Init(&EXTI_InitStructure);
//}

/*******************************************************************************
* 函数名  : W5500_NVIC_Configuration
* 描述    : W5500 接收引脚中断优先级设置
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
//void W5500_NVIC_Configuration(void)
//{
//  	NVIC_InitTypeDef NVIC_InitStructure;

//	/* Enable the EXTI4 Interrupt */
//	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStructure);
//}

/*******************************************************************************
* 函数名  : EXTI4_IRQHandler
* 描述    : 中断线4中断服务函数(W5500 INT引脚中断服务函数)
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//	if(GPIO_Pin==W5500_INT)
//	{
//		W5500_Interrupt=1;
//	}
//}
//void EXTI4_IRQHandler(void)
//{
//	if(EXTI_GetITStatus(EXTI_Line4) != RESET)
//	{
//		EXTI_ClearITPendingBit(EXTI_Line4);
//		W5500_Interrupt=1;
//	}
//}

/*******************************************************************************
* 函数名  : SPI_Configuration
* 描述    : W5500 SPI初始化配置(STM32 SPI1)
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
//void SPI_Configuration(void)
//{
//	GPIO_InitTypeDef 	GPIO_InitStructure;
//	SPI_InitTypeDef   	SPI_InitStructure;	   

//  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);	

//	/* 初始化SCK、MISO、MOSI引脚 */
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

//	/* 初始化CS引脚 */
//	GPIO_InitStructure.GPIO_Pin = W5500_SCS;
//	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
//	GPIO_Init(W5500_SCS_PORT, &GPIO_InitStructure);
//	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS);

//	/* 初始化配置STM32 SPI1 */
//	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPI设置为双线双向全双工
//	SPI_InitStructure.SPI_Mode=SPI_Mode_Master;							//设置为主SPI
//	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//SPI发送接收8位帧结构
//	SPI_InitStructure.SPI_CPOL=SPI_CPOL_Low;							//时钟悬空低
//	SPI_InitStructure.SPI_CPHA=SPI_CPHA_1Edge;							//数据捕获于第1个时钟沿
//	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft;								//NSS由外部管脚管理
//	SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_2;	//波特率预分频值为2
//	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;					//数据传输从MSB位开始
//	SPI_InitStructure.SPI_CRCPolynomial=7;								//CRC多项式为7
//	SPI_Init(SPI1,&SPI_InitStructure);									//根据SPI_InitStruct中指定的参数初始化外设SPI1寄存器

//	SPI_Cmd(SPI1,ENABLE);	//STM32使能SPI1
//}

/*******************************************************************************
* 函数名  : SPI1_Send_Byte
* 描述    : SPI1发送1个字节数据
* 输入    : dat:待发送的数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void SPI1_Send_Byte(unsigned char dat)
{
	//SPI_I2S_SendData(SPI1,dat);//写1个字节数据
	//while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);//等待数据寄存器空
	SPI_WriteByte(dat);
}

/*******************************************************************************
* 函数名  : SPI1_Send_Short
* 描述    : SPI1发送2个字节数据(16位)
* 输入    : dat:待发送的16位数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void SPI1_Send_Short(unsigned short dat)
{
//	SPI1_Send_Byte(dat/256);//写数据高位
//	SPI1_Send_Byte(dat);	//写数据低位
	SPI_WriteByte(dat/256);
	SPI_WriteByte(dat);
}

void W5500_DMA_Send(uint8_t *buff, uint16_t len)
{
			while(HAL_DMA_GetState(&hdma_spi3_tx) == HAL_DMA_STATE_BUSY);
			//__HAL_DMA_DISABLE(&hdma_spi2_tx);
			HAL_SPI_Transmit_DMA(&hspi3,buff,len);
			//while(hspi2.State == HAL_SPI_STATE_BUSY_TX);
//			__HAL_DMA_ENABLE(&hdma_spi2_tx);			
}

/*******************************************************************************
* 函数名  : Write_W5500_1Byte
* 描述    : 通过SPI1向指定地址寄存器写1个字节数据
* 输入    : reg:16位寄存器地址,dat:待写入的数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_1Byte(unsigned short reg, unsigned char dat)
{
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM1|RWB_WRITE|COMMON_R);//通过SPI1写控制字节,1个字节数据长度,写数据,选择通用寄存器
	SPI1_Send_Byte(dat);//写1个字节数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Write_W5500_2Byte
* 描述    : 通过SPI1向指定地址寄存器写2个字节数据
* 输入    : reg:16位寄存器地址,dat:16位待写入的数据(2个字节)
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_2Byte(unsigned short reg, unsigned short dat)
{
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
		
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM2|RWB_WRITE|COMMON_R);//通过SPI1写控制字节,2个字节数据长度,写数据,选择通用寄存器
	SPI1_Send_Short(dat);//写16位数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Write_W5500_nByte
* 描述    : 通过SPI1向指定地址寄存器写n个字节数据
* 输入    : reg:16位寄存器地址,*dat_ptr:待写入数据缓冲区指针,size:待写入的数据长度
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_nByte(unsigned short reg, unsigned char *dat_ptr, unsigned short size)
{
	unsigned short i;

	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平	
		
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(VDM|RWB_WRITE|COMMON_R);//通过SPI1写控制字节,N个字节数据长度,写数据,选择通用寄存器

	for(i=0;i<size;i++)//循环将缓冲区的size个字节数据写入W5500
	{
		SPI1_Send_Byte(*dat_ptr++);//写一个字节数据
	}

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Write_W5500_SOCK_1Byte
* 描述    : 通过SPI1向指定端口寄存器写1个字节数据
* 输入    : s:端口号,reg:16位寄存器地址,dat:待写入的数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_SOCK_1Byte(SOCKET s, unsigned short reg, unsigned char dat)
{
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平	
		
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM1|RWB_WRITE|(s*0x20+0x08));//通过SPI1写控制字节,1个字节数据长度,写数据,选择端口s的寄存器
	SPI1_Send_Byte(dat);//写1个字节数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Write_W5500_SOCK_2Byte
* 描述    : 通过SPI1向指定端口寄存器写2个字节数据
* 输入    : s:端口号,reg:16位寄存器地址,dat:16位待写入的数据(2个字节)
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_SOCK_2Byte(SOCKET s, unsigned short reg, unsigned short dat)
{
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
			
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM2|RWB_WRITE|(s*0x20+0x08));//通过SPI1写控制字节,2个字节数据长度,写数据,选择端口s的寄存器
	SPI1_Send_Short(dat);//写16位数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Write_W5500_SOCK_4Byte
* 描述    : 通过SPI1向指定端口寄存器写4个字节数据
* 输入    : s:端口号,reg:16位寄存器地址,*dat_ptr:待写入的4个字节缓冲区指针
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_W5500_SOCK_4Byte(SOCKET s, unsigned short reg, unsigned char *dat_ptr)
{
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
			
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM4|RWB_WRITE|(s*0x20+0x08));//通过SPI1写控制字节,4个字节数据长度,写数据,选择端口s的寄存器

	SPI1_Send_Byte(*dat_ptr++);//写第1个字节数据
	SPI1_Send_Byte(*dat_ptr++);//写第2个字节数据
	SPI1_Send_Byte(*dat_ptr++);//写第3个字节数据
	SPI1_Send_Byte(*dat_ptr++);//写第4个字节数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
}

/*******************************************************************************
* 函数名  : Read_W5500_1Byte
* 描述    : 读W5500指定地址寄存器的1个字节数据
* 输入    : reg:16位寄存器地址
* 输出    : 无
* 返回值  : 读取到寄存器的1个字节数据
* 说明    : 无
*******************************************************************************/
unsigned char Read_W5500_1Byte(unsigned short reg)
{
	unsigned char i;

	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
			
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM1|RWB_READ|COMMON_R);//通过SPI1写控制字节,1个字节数据长度,读数据,选择通用寄存器

	i=SPI_ReadByte();
//	printf("i1=%d\n",i);
//	SPI1_Send_Byte(0x00);//发送一个哑数据
//	i=SPI_ReadByte();//读取1个字节数据
//	printf("i2=%d\n",i);

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为高电平
	return i;//返回读取到的寄存器数据
}

/*******************************************************************************
* 函数名  : Read_W5500_SOCK_1Byte
* 描述    : 读W5500指定端口寄存器的1个字节数据
* 输入    : s:端口号,reg:16位寄存器地址
* 输出    : 无
* 返回值  : 读取到寄存器的1个字节数据
* 说明    : 无
*******************************************************************************/
unsigned char Read_W5500_SOCK_1Byte(SOCKET s, unsigned short reg)
{
	unsigned char i;

	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
			
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM1|RWB_READ|(s*0x20+0x08));//通过SPI1写控制字节,1个字节数据长度,读数据,选择端口s的寄存器

	i=SPI_ReadByte();
//	printf("i3=%d\n",i);
//	SPI1_Send_Byte(0x00);//发送一个哑数据
//	i=SPI_ReadByte();//读取1个字节数据
//	printf("i4=%d\n",i);
	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为高电平
	return i;//返回读取到的寄存器数据
}

/*******************************************************************************
* 函数名  : Read_W5500_SOCK_2Byte
* 描述    : 读W5500指定端口寄存器的2个字节数据
* 输入    : s:端口号,reg:16位寄存器地址
* 输出    : 无
* 返回值  : 读取到寄存器的2个字节数据(16位)
* 说明    : 无
*******************************************************************************/
unsigned short Read_W5500_SOCK_2Byte(SOCKET s, unsigned short reg)
{
	unsigned short i;

	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平
			
	SPI1_Send_Short(reg);//通过SPI1写16位寄存器地址
	SPI1_Send_Byte(FDM2|RWB_READ|(s*0x20+0x08));//通过SPI1写控制字节,2个字节数据长度,读数据,选择端口s的寄存器

//	i=SPI_ReadByte();
//	SPI1_Send_Byte(0x00);//发送一个哑数据
	i=SPI_ReadByte();//读取高位数据
//	SPI1_Send_Byte(0x00);//发送一个哑数据
	i*=256;
	i+=SPI_ReadByte();//读取低位数据

	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为高电平
	return i;//返回读取到的寄存器数据
}

/*******************************************************************************
* 函数名  : Read_SOCK_Data_Buffer
* 描述    : 从W5500接收数据缓冲区中读取数据
* 输入    : s:端口号,*dat_ptr:数据保存缓冲区指针
* 输出    : 无
* 返回值  : 读取到的数据长度,rx_size个字节
* 说明    : 无
*******************************************************************************/
unsigned short Read_SOCK_Data_Buffer(SOCKET s, unsigned char *dat_ptr)
{
	
	unsigned short rx_size;
	unsigned short offset, offset1;
	unsigned short i;
	unsigned char j;
	unsigned char size_buff[8]={0,0,0,0,0,0,0,0};
	unsigned char *size_ptr;
	size_ptr=size_buff;
//取得单个包rx_size开始
	rx_size=Read_W5500_SOCK_2Byte(s,Sn_RX_RSR);
	if(rx_size==0) return 0;//没接收到数据则返回
	if(rx_size>1460) rx_size=1460;
	rx_size=8;
	offset=Read_W5500_SOCK_2Byte(s,Sn_RX_RD);
	offset1=offset;
	offset&=(S_RX_SIZE-1);//计算实际的物理地址
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

	SPI1_Send_Short(offset);//写16位地址
	SPI1_Send_Byte(VDM|RWB_READ|(s*0x20+0x18));//写控制字节,N个字节数据长度,读数据,选择端口s的寄存器
	//j=SPI_ReadByte();
	
	if((offset+rx_size)<S_RX_SIZE)//如果最大地址未超过W5500接收缓冲区寄存器的最大地址
	{
		for(i=0;i<rx_size;i++)//循环读取rx_size个字节数据
		{

			//SPI1_Send_Byte(0x00);//发送一个哑数据
			j=SPI_ReadByte();//读取1个字节数据
//			DEBUG_PRINT("%02x ", j);
			*size_ptr=j;//将读取到的数据保存到数据保存缓冲区
			size_ptr++;//数据保存缓冲区指针地址自增1
			
		}

	}
	else//如果最大地址超过W5500接收缓冲区寄存器的最大地址
	{		
		offset=S_RX_SIZE-offset;

		for(i=0;i<=offset;i++)//循环读取出前offset个字节数据
		{

			//SPI1_Send_Byte(0x00);//发送一个哑数据
			j=SPI_ReadByte();//读取1个字节数据
			*size_ptr=j;//将读取到的数据保存到数据保存缓冲区
			size_ptr++;//数据保存缓冲区指针地址自增1
		}

		
		
		GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平

		GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

		SPI1_Send_Short(0x00);//写16位地址
		SPI1_Send_Byte(VDM|RWB_READ|(s*0x20+0x18));//写控制字节,N个字节数据长度,读数据,选择端口s的寄存器
		j=SPI_ReadByte();

		for(;i<rx_size;i++)//循环读取后rx_size-offset个字节数据
		{
			//SPI1_Send_Byte(0x00);//发送一个哑数据
			j=SPI_ReadByte();//读取1个字节数据
			*size_ptr=j;//将读取到的数据保存到数据保存缓冲区
			size_ptr++;//数据保存缓冲区指针地址自增1
		}
	}
	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
	Write_W5500_SOCK_2Byte(s, Sn_RX_RD, offset1);//复位Sn_RX_RD
	//取得单个包的rx_size结束
	
	//实际开始取数据
//	rx_size=Read_W5500_SOCK_2Byte(s,Sn_RX_RSR);
//	if(rx_size==0) return 0;//没接收到数据则返回
//	if(rx_size>1460) rx_size=1460;
	rx_size=size_buff[6]*256+size_buff[7]+8;
	offset=Read_W5500_SOCK_2Byte(s,Sn_RX_RD);
	offset1=offset;
	offset&=(S_RX_SIZE-1);//计算实际的物理地址
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

	SPI1_Send_Short(offset);//写16位地址
	SPI1_Send_Byte(VDM|RWB_READ|(s*0x20+0x18));//写控制字节,N个字节数据长度,读数据,选择端口s的寄存器
	//j=SPI_ReadByte();
	
	if((offset+rx_size)<=S_RX_SIZE)//如果最大地址未超过W5500接收缓冲区寄存器的最大地址
	{
//		for(i=0;i<rx_size;i++)//循环读取rx_size个字节数据
//		{

//			//SPI1_Send_Byte(0x00);//发送一个哑数据
//			j=SPI_ReadByte();//读取1个字节数据
////			DEBUG_PRINT("%02x ", j);
//			*dat_ptr=j;//将读取到的数据保存到数据保存缓冲区
//			dat_ptr++;//数据保存缓冲区指针地址自增1

//		}
			//改写SPI_Read_NBbytes开始
			SPI_ReadNByte(dat_ptr,rx_size);
			//改写SPI_Read_NBbytes结束

	}
	else//如果最大地址超过W5500接收缓冲区寄存器的最大地址
	{		
		offset=S_RX_SIZE-offset;

//		for(i=0;i<=offset;i++)//循环读取出前offset个字节数据
//		{

//			//SPI1_Send_Byte(0x00);//发送一个哑数据
//			j=SPI_ReadByte();//读取1个字节数据
//			*dat_ptr=j;//将读取到的数据保存到数据保存缓冲区
//			dat_ptr++;//数据保存缓冲区指针地址自增1
//		}
		SPI_ReadNByte(dat_ptr,offset+1);


		
		
		GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平

		GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

		SPI1_Send_Short(0x00);//写16位地址
		SPI1_Send_Byte(VDM|RWB_READ|(s*0x20+0x18));//写控制字节,N个字节数据长度,读数据,选择端口s的寄存器
		j=SPI_ReadByte();

//		for(;i<rx_size;i++)//循环读取后rx_size-offset个字节数据
//		{
//			//SPI1_Send_Byte(0x00);//发送一个哑数据
//			j=SPI_ReadByte();//读取1个字节数据
//			*dat_ptr=j;//将读取到的数据保存到数据保存缓冲区
//			dat_ptr++;//数据保存缓冲区指针地址自增1
//		}
		SPI_ReadNByte(dat_ptr+offset+1,rx_size-offset-1);

	}
	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
	//实际取数据结束
	offset1+=rx_size;//更新实际物理地址,即下次读取接收到的数据的起始地址
	Write_W5500_SOCK_2Byte(s, Sn_RX_RD, offset1);
	Write_W5500_SOCK_1Byte(s, Sn_CR, RECV);//发送启动接收命令
	return rx_size;//返回接收到数据的长度
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi!=&hspi3)
	{
		return;
	}
	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
	
	global_offset1+=global_size;//更新实际物理地址,即下次写待发送数据到发送数据缓冲区的起始地址
	global_size=0;
	//DEBUG_PRINT("global_offset1: %d\n",global_offset1);

	Write_W5500_SOCK_2Byte(1, Sn_TX_WR, (unsigned short)global_offset1);
	Write_W5500_SOCK_1Byte(1, Sn_CR, SEND);//发送启动发送命令
}

/*******************************************************************************
* 函数名  : Write_SOCK_Data_Buffer_DMA
* 描述    : 通过DMA方式将数据写入W5500的数据发送缓冲区
* 输入    : s:端口号,*dat_ptr:数据保存缓冲区指针,size:待写入数据的长度
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/

unsigned short Write_SOCK_Data_Buffer_DMA(SPI_HandleTypeDef *hspi,SOCKET s, unsigned char *dat_ptr, unsigned short size)
{

	//if(HAL_DMA_GetState(&hdma_spi2_tx) ==HAL_DMA_STATE_BUSY) return size;
	while(HAL_DMA_GetState(&hdma_spi3_tx) ==HAL_DMA_STATE_BUSY);
	unsigned short offset;

	global_size=0;
	//如果是UDP模式,可以在此设置目的主机的IP和端口号
	if((Read_W5500_SOCK_1Byte(s,Sn_MR)&0x0f) != MR_UDP)//如果Socket打开失败
	{	

		Write_W5500_SOCK_4Byte(s, Sn_DIPR, S1_DIP);//设置目的主机IP
		Write_W5500_SOCK_2Byte(s, Sn_DPORTR, S1_DPort[0]*256+S1_DPort[1]);//设置目的主机端口号				
	}


	offset=Read_W5500_SOCK_2Byte(s,Sn_TX_WR);
	global_offset1=offset;
	offset&=(S1_TX_SIZE-1);//计算实际的物理地址
	uint8_t add_buff[size+3];
	add_buff[0]=offset/256;
	add_buff[1]=offset;
	add_buff[2]=VDM|RWB_WRITE|(s*0x20+0x10);
	memcpy(&add_buff[3],dat_ptr,size);
	dat_ptr=add_buff;
	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

//	SPI1_Send_Short(offset);//写16位地址
//	SPI1_Send_Byte(VDM|RWB_WRITE|(s*0x20+0x10));//写控制字节,N个字节数据长度,写数据,选择端口s的寄存器
	
	if((offset+size)<S_TX_SIZE)//如果最大地址未超过W5500发送缓冲区寄存器的最大地址
	{
		global_size=size;
		W5500_DMA_Send(dat_ptr,size+3);//DMA
	}
	else//如果最大地址超过W5500发送缓冲区寄存器的最大地址
	{
		offset=S1_TX_SIZE-offset;
		global_size=offset;
		W5500_DMA_Send(dat_ptr,offset+3);//DMA
//		GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平

//		GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

////		SPI1_Send_Short(0x00);//写16位地址
////		SPI1_Send_Byte(VDM|RWB_WRITE|(s*0x20+0x10));//写控制字节,N个字节数据长度,写数据,选择端口s的寄存器
//		*(dat_ptr+offset-3)=0x0;
//		*(dat_ptr+offset-2)=0x0;
//		*(dat_ptr+offset-1)=VDM|RWB_WRITE|(s*0x20+0x10);
//		
//		W5500_DMA_Send(dat_ptr+offset-3,size-offset+3);//DMA
	}
	//DEBUG_PRINT("global_size:%d\n",global_size);
	return global_size;

//	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
//	
//	global_offset1+=size;//更新实际物理地址,即下次写待发送数据到发送数据缓冲区的起始地址

//	Write_W5500_SOCK_2Byte(s, Sn_TX_WR, global_offset1);
//	Write_W5500_SOCK_1Byte(s, Sn_CR, SEND);//发送启动发送命令				
}

/*******************************************************************************
* 函数名  : Write_SOCK_Data_Buffer
* 描述    : 将数据写入W5500的数据发送缓冲区
* 输入    : s:端口号,*dat_ptr:数据保存缓冲区指针,size:待写入数据的长度
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Write_SOCK_Data_Buffer(SOCKET s, unsigned char *dat_ptr, unsigned short size)
{
	unsigned short offset,offset1;
	while(hspi3.State == HAL_SPI_STATE_BUSY_TX);
	//如果是UDP模式,可以在此设置目的主机的IP和端口号
//	int status = (Read_W5500_SOCK_1Byte(s,Sn_MR)&0x0f);
////	if(status != MR_UDP)//如果Socket打开失败
//	{
//
//		Write_W5500_SOCK_4Byte(s, Sn_DIPR, S0_DIP);//设置目的主机IP
//		Write_W5500_SOCK_2Byte(s, Sn_DPORTR, S0_DPort[0]*256+S0_DPort[1]);//设置目的主机端口号
//	}


	offset=Read_W5500_SOCK_2Byte(s,Sn_TX_WR);
	offset1=offset;
	offset&=(S_TX_SIZE-1);//计算实际的物理地址

	GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

	SPI1_Send_Short(offset);//写16位地址
	SPI1_Send_Byte(VDM|RWB_WRITE|(s*0x20+0x10));//写控制字节,N个字节数据长度,写数据,选择端口s的寄存器

	if((offset+size)<S_TX_SIZE)//如果最大地址未超过W5500发送缓冲区寄存器的最大地址
	{
//		for(i=0;i<size;i++)//循环写入size个字节数据
//		{
//			SPI1_Send_Byte(*dat_ptr++);//写入一个字节的数据
//		}
		SPI_WriteNBytes(dat_ptr,size);
	}
	else//如果最大地址超过W5500发送缓冲区寄存器的最大地址
	{
		offset=S_TX_SIZE-offset;
//		for(i=0;i<offset;i++)//循环写入前offset个字节数据
//		{
//			SPI1_Send_Byte(*dat_ptr++);//写入一个字节的数据
//		}
		SPI_WriteNBytes(dat_ptr,offset);
		GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平

		GPIO_ResetBits(W5500_SCS_PORT, W5500_SCS);//置W5500的SCS为低电平

		SPI1_Send_Short(0x00);//写16位地址
		SPI1_Send_Byte(VDM|RWB_WRITE|(s*0x20+0x10));//写控制字节,N个字节数据长度,写数据,选择端口s的寄存器

//		for(;i<size;i++)//循环写入size-offset个字节数据
//		{
//			SPI1_Send_Byte(*dat_ptr++);//写入一个字节的数据
//		}
		SPI_WriteNBytes(&dat_ptr[offset],size-offset);
	}
	GPIO_SetBits(W5500_SCS_PORT, W5500_SCS); //置W5500的SCS为高电平
	
	offset1+=size;//更新实际物理地址,即下次写待发送数据到发送数据缓冲区的起始地址
	Write_W5500_SOCK_2Byte(s, Sn_TX_WR, offset1);
	Write_W5500_SOCK_1Byte(s, Sn_CR, SEND);//发送启动发送命令				
}


/*******************************************************************************
* 函数名  : W5500_Hardware_Reset
* 描述    : 硬件复位W5500
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : W5500的复位引脚保持低电平至少500us以上,才能重围W5500
*******************************************************************************/
void W5500_Hardware_Reset(void)
{
	GPIO_ResetBits(W5500_RST_PORT, W5500_RST);//复位引脚拉低
	HAL_Delay(500);
	GPIO_SetBits(W5500_RST_PORT, W5500_RST);//复位引脚拉高
	HAL_Delay(200);
	while((Read_W5500_1Byte(PHYCFGR)&LINK)==0);//等待以太网连接完成
	printf("w5500 Ethernet link complete !\r\n");
	//uint8_t PHY=Read_W5500_1Byte(PHYCFGR);
	//printf("PHYCFGR: %d\n",PHY);
}

/*******************************************************************************
* 函数名  : W5500_Init
* 描述    : 初始化W5500寄存器函数
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 在使用W5500之前，先对W5500初始化
*******************************************************************************/
void W5500_Init(void)
{
	uint8_t i=0;

	Write_W5500_1Byte(MR, RST);//软件复位W5500,置1有效,复位后自动清0
	HAL_Delay(10);//延时10ms,自己定义该函数

	//设置网关(Gateway)的IP地址,Gateway_IP为4字节unsigned char数组,自己定义 
	//使用网关可以使通信突破子网的局限，通过网关可以访问到其它子网或进入Internet
	Write_W5500_nByte(GAR, Gateway_IP, 4);
			
	//设置子网掩码(MASK)值,SUB_MASK为4字节unsigned char数组,自己定义
	//子网掩码用于子网运算
	Write_W5500_nByte(SUBR,Sub_Mask,4);		
	
	//设置物理地址,PHY_ADDR为6字节unsigned char数组,自己定义,用于唯一标识网络设备的物理地址值
	//该地址值需要到IEEE申请，按照OUI的规定，前3个字节为厂商代码，后三个字节为产品序号
	//如果自己定义物理地址，注意第一个字节必须为偶数
	Write_W5500_nByte(SHAR,Phy_Addr,6);		

	//设置本机的IP地址,IP_ADDR为4字节unsigned char数组,自己定义
	//注意，网关IP必须与本机IP属于同一个子网，否则本机将无法找到网关
	Write_W5500_nByte(SIPR,IP_Addr,4);		
	
	//设置发送缓冲区和接收缓冲区的大小，参考W5500数据手册
	Write_W5500_SOCK_1Byte(0,Sn_RXBUF_SIZE, 0x08);//Socket Rx memory size=8k
	Write_W5500_SOCK_1Byte(0,Sn_TXBUF_SIZE, 0x08);//Socket Tx mempry size=8k
	
	Write_W5500_SOCK_1Byte(1,Sn_RXBUF_SIZE, 0x02);//Socket Rx memory size=2k
	Write_W5500_SOCK_1Byte(1,Sn_TXBUF_SIZE, 0x02);//Socket Tx mempry size=2k
	for(i=2;i<8;i++)
	{
		Write_W5500_SOCK_1Byte(i,Sn_RXBUF_SIZE, 0x01);//Socket Rx memory size=1k
		Write_W5500_SOCK_1Byte(i,Sn_TXBUF_SIZE, 0x01);//Socket Tx mempry size=1k
	}

	//设置重试时间，默认为2000(200ms) 
	//每一单位数值为100微秒,初始化时值设为2000(0x07D0),等于200毫秒
	Write_W5500_2Byte(RTR, 0x07d0);

	//设置重试次数，默认为8次 
	//如果重发的次数超过设定值,则产生超时中断(相关的端口中断寄存器中的Sn_IR 超时位(TIMEOUT)置“1”)
	Write_W5500_1Byte(RCR,8);

	//启动中断，参考W5500数据手册确定自己需要的中断类型
	//IMR_CONFLICT是IP地址冲突异常中断,IMR_UNREACH是UDP通信时，地址无法到达的异常中断
	//其它是Socket事件中断，根据需要添加
	Write_W5500_1Byte(IMR,IM_IR7 | IM_IR6);
	Write_W5500_1Byte(SIMR,S0_IMR);
	Write_W5500_SOCK_1Byte(0, Sn_IMR, IMR_SENDOK | IMR_TIMEOUT | IMR_RECV | IMR_DISCON | IMR_CON);

}

/*******************************************************************************
* 函数名  : Detect_Gateway
* 描述    : 检查网关服务器
* 输入    : 无
* 输出    : 无
* 返回值  : 成功返回TRUE(0xFF),失败返回FALSE(0x00)
* 说明    : 无
*******************************************************************************/
unsigned char Detect_Gateway(void)
{
	unsigned char ip_adde[4];
	ip_adde[0]=IP_Addr[0]+1;
	ip_adde[1]=IP_Addr[1]+1;
	ip_adde[2]=IP_Addr[2]+1;
	ip_adde[3]=IP_Addr[3]+1;

	//检查网关及获取网关的物理地址
	Write_W5500_SOCK_4Byte(0,Sn_DIPR,ip_adde);//向目的地址寄存器写入与本机IP不同的IP值
	Write_W5500_SOCK_1Byte(0,Sn_MR,MR_UDP);//设置socket为TCP模式
	Write_W5500_SOCK_1Byte(0,Sn_CR,OPEN);//打开Socket	
	HAL_Delay(5);//延时5ms
	Write_W5500_SOCK_4Byte(1,Sn_DIPR,ip_adde);//向目的地址寄存器写入与本机IP不同的IP值
	Write_W5500_SOCK_1Byte(1,Sn_MR,MR_UDP);//设置socket为TCP模式
	Write_W5500_SOCK_1Byte(1,Sn_CR,OPEN);//打开Socket	
	HAL_Delay(5);//延时5ms 	 	
//	Write_W5500_SOCK_4Byte(2,Sn_DIPR,ip_adde);//向目的地址寄存器写入与本机IP不同的IP值
//	Write_W5500_SOCK_1Byte(2,Sn_MR,MR_UDP);//设置socket为TCP模式
//	Write_W5500_SOCK_1Byte(2,Sn_CR,OPEN);//打开Socket	
//	HAL_Delay(5);//延时5ms 
	//printf("Sn_SR %d\n",Read_W5500_SOCK_1Byte(0,Sn_SR));
	if(Read_W5500_SOCK_1Byte(0,Sn_SR) != SOCK_INIT)//如果socket打开失败
	{
		//printf("sock falied!\n");
		Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);//打开不成功,关闭Socket
		return FALSE;//返回FALSE(0x00)
	}
		if(Read_W5500_SOCK_1Byte(1,Sn_SR) != SOCK_INIT)//如果socket打开失败
	{
		//printf("sock falied!\n");
		Write_W5500_SOCK_1Byte(1,Sn_CR,CLOSE);//打开不成功,关闭Socket
		return FALSE;//返回FALSE(0x00)
	}
//		if(Read_W5500_SOCK_1Byte(2,Sn_SR) != SOCK_INIT)//如果socket打开失败
//	{
//		//printf("sock falied!\n");
//		Write_W5500_SOCK_1Byte(2,Sn_CR,CLOSE);//打开不成功,关闭Socket
//		return FALSE;//返回FALSE(0x00)
//	}

	Write_W5500_SOCK_1Byte(0,Sn_CR,CONNECT);//设置Socket为Connect模式	
	Write_W5500_SOCK_1Byte(1,Sn_CR,CONNECT);//设置Socket为Connect模式
//	Write_W5500_SOCK_1Byte(2,Sn_CR,CONNECT);	

	do
	{
		uint8_t j=0;
		uint8_t m=0;
//		uint8_t k=0;
		j=Read_W5500_SOCK_1Byte(0,Sn_IR);//读取Socket0中断标志寄存器
		m=Read_W5500_SOCK_1Byte(1,Sn_IR);//读取Socket0中断标志寄存器
//		k=Read_W5500_SOCK_1Byte(2,Sn_IR);
		if(j!=0)
		Write_W5500_SOCK_1Byte(0,Sn_IR,j);
		HAL_Delay(5);//延时5ms 
		if(m!=0)
		Write_W5500_SOCK_1Byte(1,Sn_IR,m);
		HAL_Delay(5);//延时5ms 
//		Write_W5500_SOCK_1Byte(2,Sn_IR,k);
//		HAL_Delay(5);//延时5ms 
		if(((j&IR_TIMEOUT) == IR_TIMEOUT)||((m&IR_TIMEOUT) == IR_TIMEOUT))
		{
			return FALSE;	
		}
		else if((Read_W5500_SOCK_1Byte(0,Sn_DHAR) != 0xff)||(Read_W5500_SOCK_1Byte(1,Sn_DHAR) != 0xff))
		{
			Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);//关闭Socket
			Write_W5500_SOCK_1Byte(1,Sn_CR,CLOSE);//关闭Socket
//			Write_W5500_SOCK_1Byte(2,Sn_CR,CLOSE);//关闭Socket
			return TRUE;							
		}
	}while(1);
}

/*******************************************************************************
* 函数名  : Socket_Init
* 描述    : 指定Socket(0~7)初始化
* 输入    : s:待初始化的端口
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Socket_Init(SOCKET s)
{
	//设置分片长度，参考W5500数据手册，该值可以不修改	
	Write_W5500_SOCK_2Byte(s, Sn_MSSR, 1460);//最大分片字节数=1460(0x5b4)
//	Write_W5500_SOCK_2Byte(1, Sn_MSSR, 1460);//最大分片字节数=1460(0x5b4)
//	Write_W5500_SOCK_2Byte(2, Sn_MSSR, 1460);//最大分片字节数=1460(0x5b4)
	//设置指定端口
	switch(s)
	{
		case 0:
			//设置端口0的端口号
			Write_W5500_SOCK_2Byte(0, Sn_PORT, S0_Port[0]*256+S0_Port[1]);		
			Write_W5500_SOCK_4Byte(0, Sn_DIPR, S0_DIP);//设置目的主机IP
			Write_W5500_SOCK_2Byte(0, Sn_DPORTR, S0_DPort[0]*256+S0_DPort[1]);//设置目的主机端口号			
			break;

		case 1:
//			Write_W5500_SOCK_2Byte(1, Sn_PORT, S1_Port[0]*256+S1_Port[1]);
//			Write_W5500_SOCK_4Byte(1, Sn_DIPR, S1_DIP);//设置目的主机IP
//			Write_W5500_SOCK_2Byte(1, Sn_DPORTR, S1_DPort[0]*256+S1_DPort[1]);//设置目的主机端口号
			break;

		case 2:
			break;
		case 3:
			break;

		case 4:
			break;

		case 5:
			break;

		case 6:
			break;

		case 7:
			break;

		default:
			break;
	}
}

/*******************************************************************************
* 函数名  : Socket_Connect
* 描述    : 设置指定Socket(0~7)为客户端与远程服务器连接
* 输入    : s:待设定的端口
* 输出    : 无
* 返回值  : 成功返回TRUE(0xFF),失败返回FALSE(0x00)
* 说明    : 当本机Socket工作在客户端模式时,引用该程序,与远程服务器建立连接
*			如果启动连接后出现超时中断，则与服务器连接失败,需要重新调用该程序连接
*			该程序每调用一次,就与服务器产生一次连接
*******************************************************************************/
unsigned char Socket_Connect(SOCKET s)
{
	Write_W5500_SOCK_1Byte(s,Sn_MR,MR_TCP);//设置socket为TCP模式
	Write_W5500_SOCK_1Byte(s,Sn_CR,OPEN);//打开Socket
	HAL_Delay(5);//延时5ms
	if(Read_W5500_SOCK_1Byte(s,Sn_SR)!=SOCK_INIT)//如果socket打开失败
	{
		Write_W5500_SOCK_1Byte(s,Sn_CR,CLOSE);//打开不成功,关闭Socket
		return FALSE;//返回FALSE(0x00)
	}
	Write_W5500_SOCK_1Byte(s,Sn_CR,CONNECT);//设置Socket为Connect模式
	return TRUE;//返回TRUE,设置成功
}

/*******************************************************************************
* 函数名  : Socket_Listen
* 描述    : 设置指定Socket(0~7)作为服务器等待远程主机的连接
* 输入    : s:待设定的端口
* 输出    : 无
* 返回值  : 成功返回TRUE(0xFF),失败返回FALSE(0x00)
* 说明    : 当本机Socket工作在服务器模式时,引用该程序,等等远程主机的连接
*			该程序只调用一次,就使W5500设置为服务器模式
*******************************************************************************/
unsigned char Socket_Listen(SOCKET s)
{
	Write_W5500_SOCK_1Byte(s,Sn_MR,MR_TCP);//设置socket为TCP模式 
	Write_W5500_SOCK_1Byte(s,Sn_CR,OPEN);//打开Socket	
	HAL_Delay(5);//延时5ms
	if(Read_W5500_SOCK_1Byte(s,Sn_SR)!=SOCK_INIT)//如果socket打开失败
	{
		Write_W5500_SOCK_1Byte(s,Sn_CR,CLOSE);//打开不成功,关闭Socket
		return FALSE;//返回FALSE(0x00)
	}	
	Write_W5500_SOCK_1Byte(s,Sn_CR,LISTEN);//设置Socket为侦听模式	
	HAL_Delay(5);//延时5ms
	if(Read_W5500_SOCK_1Byte(s,Sn_SR)!=SOCK_LISTEN)//如果socket设置失败
	{
		Write_W5500_SOCK_1Byte(s,Sn_CR,CLOSE);//设置不成功,关闭Socket
		return FALSE;//返回FALSE(0x00)
	}

	return TRUE;

	//至此完成了Socket的打开和设置侦听工作,至于远程客户端是否与它建立连接,则需要等待Socket中断，
	//以判断Socket的连接是否成功。参考W5500数据手册的Socket中断状态
	//在服务器侦听模式不需要设置目的IP和目的端口号
}

/*******************************************************************************
* 函数名  : Socket_UDP
* 描述    : 设置指定Socket(0~7)为UDP模式
* 输入    : s:待设定的端口
* 输出    : 无
* 返回值  : 成功返回TRUE(0xFF),失败返回FALSE(0x00)
* 说明    : 如果Socket工作在UDP模式,引用该程序,在UDP模式下,Socket通信不需要建立连接
*			该程序只调用一次，就使W5500设置为UDP模式
*******************************************************************************/
unsigned char Socket_UDP(SOCKET s)
{
	Write_W5500_SOCK_1Byte(s,Sn_MR,MR_UDP);//设置Socket为UDP模式*/
	Write_W5500_SOCK_1Byte(s,Sn_CR,OPEN);//打开Socket*/

	
	HAL_Delay(5);//延时5ms

	if(Read_W5500_SOCK_1Byte(s,Sn_SR)!=SOCK_UDP)//如果Socket打开失败
	{
		Write_W5500_SOCK_1Byte(s,Sn_CR,CLOSE);//打开不成功,关闭Socket
		//printf("Socket_UDP failed!\n");
		return FALSE;//返回FALSE(0x00)
	}
	else
	{
		//printf("UDP Success!\n");
		return TRUE;
	}

	//至此完成了Socket的打开和UDP模式设置,在这种模式下它不需要与远程主机建立连接
	//因为Socket不需要建立连接,所以在发送数据前都可以设置目的主机IP和目的Socket的端口号
	//如果目的主机IP和目的Socket的端口号是固定的,在运行过程中没有改变,那么也可以在这里设置
}

/*******************************************************************************
* 函数名  : W5500_Interrupt_Process
* 描述    : W5500中断处理程序框架
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void W5500_Interrupt_Process(void)
{
	
	//中断方式
	unsigned char i,j;

IntDispose:
	W5500_Interrupt=0;//清零中断标志
	i = Read_W5500_1Byte(IR);//读取中断标志寄存器

	Write_W5500_1Byte(IR, (i&0xf0));//回写清除中断标志

	if((i & CONFLICT) == CONFLICT)//IP地址冲突异常处理
	{
		 //自己添加代码
		printf("ip conflict!\r\n");
	}

	if((i & UNREACH) == UNREACH)//UDP模式下地址无法到达异常处理
	{
		//自己添加代码
		printf("address can't reach!\r\n");
	}

	i=Read_W5500_1Byte(SIR);//读取端口中断标志寄存器
	if((i & S0_INT) == S0_INT)//Socket0事件处理 
	{
		//printf("Socket event process!\n");
		j=Read_W5500_SOCK_1Byte(0,Sn_IR);//读取Socket0中断标志寄存器
		Write_W5500_SOCK_1Byte(0,Sn_IR,j);
		if(j&IR_CON)//在TCP模式下,Socket0成功连接 
		{
			//printf("Socket Connect Success!\n");
			S0_State|=S_CONN;//网络连接状态0x02,端口完成连接，可以正常传输数据
		}
		if(j&IR_DISCON)//在TCP模式下Socket断开连接处理
		{
			//DEBUG_PRINT("TCP 1 Socket disconnect!\n");
			Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);//关闭端口,等待重新打开连接 
			Socket_Init(0);		//指定Socket(0~7)初始化,初始化端口0
			S0_State=0;//网络连接状态0x00,端口连接失败
		}
		if(j&IR_SEND_OK)//Socket0数据发送完成,可以再次启动S_tx_process()函数发送数据 
		{
			printf("data SEND_OK\n\r");
			S0_Data|=S_TRANSMITOK;//端口发送一个数据包完成 
		}
		if(j&IR_RECV)//Socket接收到数据,可以启动S_rx_process()函数 
		{
			printf("data RECV_OK!\n\r");
			S0_Data|=S_RECEIVE;//端口接收到一个数据包
		}
		if(j&IR_TIMEOUT)//Socket连接或数据传输超时处理 
		{
			//DEBUG_PRINT("Socket timeout!\n");
			Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);// 关闭端口,等待重新打开连接 
			S0_State=0;//网络连接状态0x00,端口连接失败
		}
	}
	if(Read_W5500_1Byte(SIR) != 0)
	{		
		//printf("IntDispose\n");
		goto IntDispose;
	}
}
//void Dport_Set(void)
//{
//		Write_W5500_SOCK_4Byte(0, Sn_DIPR, S0_DIP);//设置目的主机IP
//		Write_W5500_SOCK_2Byte(0, Sn_DPORTR, S0_DPort[0]*256+S0_DPort[1]);//设置目的主机端口号
//		Write_W5500_SOCK_4Byte(1, Sn_DIPR, S1_DIP);//设置目的主机IP
//		Write_W5500_SOCK_2Byte(1, Sn_DPORTR, S1_DPort[0]*256+S1_DPort[1]);//设置目的主机端口号	
//		Write_W5500_SOCK_4Byte(2, Sn_DIPR, S2_DIP);//设置目的主机IP
//		Write_W5500_SOCK_2Byte(2, Sn_DPORTR, S2_DPort[0]*256+S2_DPort[1]);//设置目的主机端口号		
//}

void W5500_Initialization(void)
{
	W5500_Init();	
	Detect_Gateway();	
	Socket_Init(0);
	//Socket_Init(1);
//	Socket_Init(2);
	//Dport_Set();
}


void Load_Net_Parameters(void)
{
	Gateway_IP[0] = 192;
	Gateway_IP[1] = 168;
	Gateway_IP[2] = 100;
	Gateway_IP[3] = 1;

	Sub_Mask[0]=255;
	Sub_Mask[1]=255;
	Sub_Mask[2]=255;
	Sub_Mask[3]=0;

	Phy_Addr[0]=0x0c;
	Phy_Addr[1]=0x29;
	Phy_Addr[2]=0xab;
	Phy_Addr[3]=0x7c;
	Phy_Addr[4]=0x00;
	Phy_Addr[5]=0x05;

	IP_Addr[0]=192;
	IP_Addr[1]=168;
	IP_Addr[2]=100;
	IP_Addr[3]=10;

	S0_Port[0] = 0x13;//5000 
	S0_Port[1] = 0x88;

	S0_DIP[0]=192;
	S0_DIP[1]=168;
	S0_DIP[2]=100;
	S0_DIP[3]=7;

	S0_DPort[0] = 0x22;//8899
	S0_DPort[1] = 0xC3;

//	UDP_DIPR[0] = 192;
//	UDP_DIPR[1] = 168;
//	UDP_DIPR[2] = 100;
//	UDP_DIPR[3] = 10;
//
//	UDP_DPORT[0] = 0x17;
//	UDP_DPORT[1] = 0x70;

	S0_Mode=UDP_MODE;
}


void Process_Socket_Data(SOCKET s)
{
	unsigned short size;
	size=Read_SOCK_Data_Buffer(s, Rx_Buffer);
	//DEBUG_PRINT("size=%d\n",size);
	
	for(int i=0;i<size;i++){
		printf("%c ",Rx_Buffer[i]);
	}
	S0_DIP[0] = Rx_Buffer[0];
	//DEBUG_PRINT("UDP_DIPR:%d\n",Rx_Buffer[0]);
	S0_DIP[1] = Rx_Buffer[1];
	S0_DIP[2] = Rx_Buffer[2];
	S0_DIP[3] = Rx_Buffer[3];

	S0_DPort[0] = Rx_Buffer[4];
	S0_DPort[1] = Rx_Buffer[5];
	memcpy(Tx_Buffer, Rx_Buffer+8, size-8);
	Tx_Buffer[size-8] = 0xFF;
	Write_SOCK_Data_Buffer(s, Tx_Buffer, size-7);
}

void W5500_Socket_Set(void)
{
	if(S0_State==0)//???ú03?ê??ˉ????
	{
		if(S0_Mode==TCP_SERVER)//TCP·t???÷?￡ê? 
		{
			if(Socket_Listen(0)==TRUE)
				S0_State=S_INIT;
			else
			{
				//DEBUG_PRINT("ERROR1\n");
				S0_State=0;
			}
		}
		else if(S0_Mode==TCP_CLIENT)//TCP?í?§???￡ê? 
		{
			if(Socket_Connect(0)==TRUE)
				S0_State=S_INIT;
			else
			{
				//DEBUG_PRINT("ERROR2\n");
				S0_State=0;
			}
		}
		else//UDP?￡ê? 
		{
			if(Socket_UDP(0)==TRUE)
			{
				S0_State=S_INIT|S_CONN;
				printf("socket open success!\r\n");
				//DEBUG_PRINT("Socket_UDP Success!\n");
			}
			else
			{
				//DEBUG_PRINT("ERROR3\n");
				S0_State=0;
			}
		}
	}
	Write_W5500_1Byte(PHYCFGR,0x4a);
	Write_W5500_1Byte(PHYCFGR,0xca);
	//uint8_t PHY=Read_W5500_1Byte(PHYCFGR);
	//DEBUG_PRINT("PHYCFGR: %x\n",PHY);
}
void W5500_Socket1_Set(void)
{
	if(S1_State==0)//???ú03?ê??ˉ????
	{
		if(S1_Mode==TCP_SERVER)//TCP·t???÷?￡ê? 
		{
			if(Socket_Listen(1)==TRUE)
				S1_State=S_INIT;
			else
				S1_State=0;
		}
		else if(S1_Mode==TCP_CLIENT)//TCP?í?§???￡ê? 
		{
			if(Socket_Connect(1)==TRUE)
				S1_State=S_INIT;
			else
				S1_State=0;
		}
		else//UDP?￡ê? 
		{
			if(Socket_UDP(1)==TRUE)
			{
				S1_State=S_INIT|S_CONN;
				//DEBUG_PRINT("Socket_UDP Success!\n");
			}
			else
			{
				S1_State=0;
			}
		}
	}
	 Write_W5500_1Byte(PHYCFGR,0x4a);
	Write_W5500_1Byte(PHYCFGR,0xca);
	//uint8_t PHY=Read_W5500_1Byte(PHYCFGR);
	//DEBUG_PRINT("PHYCFGR: %x\n",PHY);
}

void W5500_Send_Data(unsigned char buffer[],uint16_t length)
{
	//DEBUG_PRINT("S0_State=%d,S0_Data=%d.\n",S0_State,S0_Data);
	//if(S0_State == (S_INIT|S_CONN))
		{
				S0_Data&=~S_TRANSMITOK;
				memcpy(Tx_Buffer, buffer,length);	
			Write_SOCK_Data_Buffer(0, Tx_Buffer, length);
		}
}

//void W5500_Send_HeartBeat(unsigned char buffer[],uint16_t length)
//{
//	if(S2_State == (S_INIT|S_CONN))
//		{
//				S2_Data&=~S_TRANSMITOK;
//				memcpy(Tx_Buffer, buffer,length);	
//			Write_SOCK_Heart(2, Tx_Buffer, length);
//		}
//}
unsigned short W5500_Send_Data_DMA(SPI_HandleTypeDef *hspi,unsigned char buffer[],uint16_t length)
{
	unsigned short size = 0;
	if(S1_State == (S_INIT|S_CONN))
		{
				S1_Data &= ~S_TRANSMITOK;
				//memcpy(Tx_Buffer, buffer,length);	
			unsigned short size = Write_SOCK_Data_Buffer_DMA(hspi,1, buffer, length);
		}
		return size;
}

uint16_t W5500_Rec_Listen()//中断接收方式
{
			if(W5500_Interrupt)	
		{
			W5500_Interrupt_Process();
		}

		if((S0_Data & S_RECEIVE) == S_RECEIVE)
		{
			S0_Data&=~S_RECEIVE;
			unsigned short size;
			size=Read_SOCK_Data_Buffer(0, Rx_Buffer);
			//Process_Socket_Data(0);
			Rec_Flag=1;
			//RSP_Config_REC();
			return size;
		}else
		{
			return 0;
		}
}
uint16_t W5500_Rec_Listen_NO_INT()//轮询方式
{
	while((HAL_DMA_GetState(&hdma_spi3_tx) ==HAL_DMA_STATE_BUSY)||(hspi3.State == HAL_SPI_STATE_BUSY_TX));
	unsigned char i,j;
	i=Read_W5500_1Byte(SIR);//读取端口中断标志寄存器
	if((i & S0_INT) == S0_INT)//Socket0事件处理 
	{
		//printf("Socket event process!\n");
		j=Read_W5500_SOCK_1Byte(0,Sn_IR);//读取Socket0中断标志寄存器
		Write_W5500_SOCK_1Byte(0,Sn_IR,j);
		if(j&IR_CON)//在TCP模式下,Socket0成功连接 
		{
			//printf("Socket Connect Success!\n");
			S0_State|=S_CONN;//网络连接状态0x02,端口完成连接，可以正常传输数据
		}
		if(j&IR_DISCON)//在TCP模式下Socket断开连接处理
		{
			//DEBUG_PRINT("TCP 2 Socket disconnect!\n");
			Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);//关闭端口,等待重新打开连接 
			Socket_Init(0);		//指定Socket(0~7)初始化,初始化端口0
			S0_State=0;//网络连接状态0x00,端口连接失败
		}
		if(j&IR_SEND_OK)//Socket0数据发送完成,可以再次启动S_tx_process()函数发送数据 
		{
			//printf("tag SEND_OK\n");
			S0_Data|=S_TRANSMITOK;//端口发送一个数据包完成 
		}
		if(j&IR_RECV)//Socket接收到数据,可以启动S_rx_process()函数 
		{
			//printf("tag RECV!\n");
			S0_Data|=S_RECEIVE;//端口接收到一个数据包
		}
//		if(j&IR_TIMEOUT)//Socket连接或数据传输超时处理 
//		{
//			//printf("Socket timeout!\n");
//			Write_W5500_SOCK_1Byte(0,Sn_CR,CLOSE);// 关闭端口,等待重新打开连接 
//			S0_State=0;//网络连接状态0x00,端口连接失败
//		}
	}
	if((S0_Data & S_RECEIVE) == S_RECEIVE)
	{
		S0_Data&=~S_RECEIVE;
		uint16_t size;
		size=Read_SOCK_Data_Buffer(0, Rx_Buffer);
		//Process_Socket_Data(0);
		Rec_Flag=1;
		// DEBUG_PRINT("size=%d\r\n", size);
		//RSP_Config_REC();
		return size;
	}	else
	{
		return 0;
	}
}
void Socket0_Buffer_Empty(void)
{
	unsigned short current_offset=Read_W5500_SOCK_2Byte(0,Sn_RX_WR);
	Write_W5500_SOCK_2Byte(0, Sn_RX_RD, current_offset);
	Write_W5500_SOCK_2Byte(0, Sn_RX_RSR, 0);
	Write_W5500_SOCK_1Byte(0, Sn_CR, RECV);
}
void Socket_DPort_Change(SOCKET s,int Dport)
{
		Write_W5500_SOCK_2Byte(s, Sn_DPORTR, Dport);//设置目的主机端口
}
void Socket_DIP_Change(SOCKET s,unsigned char *DIP)//传入IP数组
{
	Write_W5500_SOCK_4Byte(s, Sn_DIPR, DIP);//设置目的主机IP
}
