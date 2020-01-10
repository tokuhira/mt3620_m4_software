/*
 * (C) 2005-2019 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "printf.h"
#include "mt3620.h"

#include "os_hal_gpio.h"
#include "os_hal_uart.h"

/******************************************************************************/
/* Configurations */
/******************************************************************************/
static const uint8_t uart_port_num = OS_HAL_UART_ISU0;
static const uint8_t gpio_led_red = OS_HAL_GPIO_8;
static const uint8_t gpio_led_green = OS_HAL_GPIO_9;
static const uint8_t gpio_button_a = OS_HAL_GPIO_12;
static const uint8_t gpio_button_b = OS_HAL_GPIO_13;

#define APP_STACK_SIZE_BYTES (1024 / 4)

/******************************************************************************/
/* Applicaiton Hooks */
/******************************************************************************/
// Hook for "stack over flow".
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
	printf("%s: %s\r\n", __func__, pcTaskName);
}

// Hook for "memory allocation failed".
void vApplicationMallocFailedHook(void)
{
	printf("%s\r\n", __func__);
}
// Hook for "printf".
void _putchar(char character)
{
	mtk_os_hal_uart_put_char(uart_port_num, character);
}

/******************************************************************************/
/* Functions */
/******************************************************************************/
static int gpio_output(u8 gpio_no, u8 level)
{
	int ret;

	ret = mtk_os_hal_gpio_request(gpio_no);
	if (ret != 0) {
		printf("request gpio[%d] fail\r\n", gpio_no);
		return ret;
	}
	mtk_os_hal_gpio_pmx_set_mode(gpio_no, OS_HAL_MODE_6);
	mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_OUTPUT);
	mtk_os_hal_gpio_set_output(gpio_no, level);
	ret = mtk_os_hal_gpio_free(gpio_no);
	if (ret != 0) {
		printf("free gpio[%d] fail\r\n", gpio_no);
		return ret;
	}
	return 0;
}

static int gpio_input(u8 gpio_no, os_hal_gpio_data* pvalue)
{
	u8 ret;

	ret = mtk_os_hal_gpio_request(gpio_no);
	if (ret != 0) {
		printf("request gpio[%d] fail\r\n", gpio_no);
		return ret;
	}
	mtk_os_hal_gpio_pmx_set_mode(gpio_no, OS_HAL_MODE_6);
	mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_INPUT);
	vTaskDelay(pdMS_TO_TICKS(10));

	mtk_os_hal_gpio_get_input(gpio_no, pvalue);

	ret = mtk_os_hal_gpio_free(gpio_no);
	if (ret != 0) {
		printf("free gpio[%d] fail\r\n", gpio_no);
		return ret;
	}

	return 0;
}

static void gpio_task(void* pParameters)
{
	os_hal_gpio_data value = 0;

	printf("GPIO Task Started\r\n");
	while (1) {
		// Get Button_A status and set LED Red.
		gpio_input(gpio_button_a, &value);
		if (value == OS_HAL_GPIO_DATA_HIGH) {
			gpio_output(gpio_led_red, OS_HAL_GPIO_DATA_HIGH);
		} else {
			gpio_output(gpio_led_red, OS_HAL_GPIO_DATA_LOW);
		}

		// Get Button_B status and set LED Green.
		gpio_input(gpio_button_b, &value);
		if (value == OS_HAL_GPIO_DATA_HIGH) {
			gpio_output(gpio_led_green, OS_HAL_GPIO_DATA_LOW);
		} else {
			gpio_output(gpio_led_green, OS_HAL_GPIO_DATA_HIGH);
		}

		// Delay for 100ms
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

_Noreturn void RTCoreMain(void)
{
	// Setup Vector Table
	NVIC_SetupVectorTable();

	// Init UART
	mtk_os_hal_uart_ctlr_init(uart_port_num);
	printf("\r\nFreeRTOS GPIO Demo\r\n");

	// Create GPIO Task
	xTaskCreate(gpio_task, "GPIO Task", APP_STACK_SIZE_BYTES, NULL, 4, NULL);
	vTaskStartScheduler();

	for (;;) {
		__asm__("wfi");
	}
}

