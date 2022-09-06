update分支为最终版本，加入循环缓冲，同时开启串口四的DMA空闲中断，注意的是在串口4的DMA中断当中推出时需要开启HAL_UART_Receive_DMA(&huart4, DMA_Buffer, DMA_BUFFER_LENGTH);

否则会出现接收异常的情况，该bug目前未解决，在调试中发现。