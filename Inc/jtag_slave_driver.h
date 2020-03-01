#ifndef _JTAG_SLAVE_DRIVER_H_
#define _JTAG_SLAVE_DRIVER_H_


#define JTAG_CLK_PIN                 GPIO_PIN_7
#define JTAG_CLK_PIN_PORT            GPIOA
#define JTAG_CLK_PIN_CLOCK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define JTAG_CLK_PIN_IRQ             EXTI9_5_IRQn

#define JTAG_TMS_PIN                 GPIO_PIN_6
#define JTAG_TMS_PIN_PORT            GPIOA
#define JTAG_TMS_PIN_CLOCK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define JTAG_TDO_PIN                 GPIO_PIN_5
#define JTAG_TDO_PIN_PORT            GPIOA
#define JTAG_TDO_PIN_CLOCK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define JTAG_TDI_PIN                 GPIO_PIN_4
#define JTAG_TDI_PIN_PORT            GPIOA
#define JTAG_TDI_PIN_CLOCK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

// TDI read
#define PIN_TDI_DATA() \
    HAL_GPIO_ReadPin(JTAG_TDI_PIN_PORT, JTAG_TDI_PIN) ? 1 : 0

// TMS read
#define PIN_TMS_DATA() \
    HAL_GPIO_ReadPin(JTAG_TMS_PIN_PORT, JTAG_TMS_PIN) ? 1 : 0

// CLK read
#define PIN_CLK_DATA() \
    HAL_GPIO_ReadPin(JTAG_CLK_PIN_PORT, JTAG_CLK_PIN) ? 1 : 0

// TDO set
#define PIN_TDO_SET() \
    HAL_GPIO_WritePin(JTAG_TDO_PIN_PORT, JTAG_TDO_PIN, GPIO_PIN_SET)

// TDO clr
#define PIN_TDO_CLR() \
    HAL_GPIO_WritePin(JTAG_TDO_PIN_PORT, JTAG_TDO_PIN, GPIO_PIN_RESET)


void jtag_slave_driver_init();

#endif
