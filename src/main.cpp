/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"

// ----------------------------------------------------------------------------
//
// Standalone STM32F1 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

void delay() {
	volatile uint32_t i;
	for (i = 0; i != 0x500000; i++);
}


void usartInit() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	// Tx
	GPIO_InitTypeDef initStruct;
	initStruct.GPIO_Pin = GPIO_Pin_9;
	initStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	initStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &initStruct);

	// Rx
	initStruct.GPIO_Pin = GPIO_Pin_10;
	initStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	initStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &initStruct);

	USART_InitTypeDef usartInitStruct;
	usartInitStruct.USART_BaudRate = 115200;
	usartInitStruct.USART_WordLength = USART_WordLength_8b;
	usartInitStruct.USART_StopBits = USART_StopBits_1;
	usartInitStruct.USART_Parity = USART_Parity_No;
	usartInitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usartInitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &usartInitStruct);

	USART_Cmd(USART1, ENABLE);
}


void send(char* charBuffer, unsigned int count) {
	while (count--) {
		USART_SendData(USART1, *charBuffer++);
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
	}
}

char buffer[30] = {'\0'};
int bufferSize;


int main(int argc, char* argv[]) {
	usartInit();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef gpioStruct;
	gpioStruct.GPIO_Pin = GPIO_Pin_5;
	gpioStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpioStruct);

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitTypeDef adcStruct;
	adcStruct.ADC_Mode = ADC_Mode_Independent;
	adcStruct.ADC_ScanConvMode = DISABLE;
	adcStruct.ADC_ContinuousConvMode = ENABLE;
	adcStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	adcStruct.ADC_DataAlign = ADC_DataAlign_Right;
	adcStruct.ADC_NbrOfChannel = 1;

	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_28Cycles5);
	ADC_Init(ADC1, &adcStruct);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));

	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);


	GPIO_InitTypeDef initStruct;
	initStruct.GPIO_Pin = GPIO_Pin_6;
	initStruct.GPIO_Mode = GPIO_Mode_IPD;
	initStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &initStruct);

	initStruct.GPIO_Pin = GPIO_Pin_7;
	initStruct.GPIO_Mode = GPIO_Mode_IPD;
	initStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &initStruct);


	int adc = 0;
	float voltage = 0;
	float voltagePrev = 0;

	bool enc1 = 0;
	bool enc2 = 0;
	bool enc1Prev = 0;
	bool enc2Prev = 0;
	int steps = 0;
	int prevSteps = 0;
	int degree = 0;

	while (1) {
		enc1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
		enc2 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7);

		if ((enc1 != enc1Prev) || (enc2 != enc2Prev)) {
			if (!enc1Prev && !enc2Prev) {
				if (enc1 && !enc2) {
					steps++;
				}
				else if (!enc1 && enc2) {
					steps--;
				}
			}
		}

		enc1Prev = enc1;
		enc2Prev = enc2;

		if (steps != prevSteps) {
			degree = steps * 14;
			bufferSize = sprintf(buffer, "%d\r\n", degree);
			send(buffer, bufferSize);
		}
		prevSteps = steps;



		adc = ADC_GetConversionValue(ADC1);
		voltage = ((float)adc / 4096) * 3.3;

		if (abs(voltage - voltagePrev) > 0.01) {
			bufferSize = sprintf(buffer, "%.1f\r\n", voltage);
			send(buffer, bufferSize);
		}

		voltagePrev = voltage;
//		delay();
	}
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
