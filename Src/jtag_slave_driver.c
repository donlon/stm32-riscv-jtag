#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"
#include "jtag_slave_driver.h"
#include "jtag_slave_ex.h"
#include "jtag_define.h"
#include "debug_defines.h"

typedef enum {
    TEST_LOGIC_RESET = 0,
    RUN_TEST_IDLE,
    // DR
    SELECT_DR,
    CAPTURE_DR,
    SHIFT_DR,
    EXIT1_DR,
    PAUSE_DR,
    EXIT2_DR,
    UPDATE_DR,
    // IR
    SELECT_IR,
    CAPTURE_IR,
    SHIFT_IR,
    EXIT1_IR,
    PAUSE_IR,
    EXIT2_IR,
    UPDATE_IR,
} jtag_state_t;


static jtag_state_t jtag_state;
static uint64_t shift_data;
static uint8_t ir_reg;
static uint8_t dm_reg;
static uint8_t shift_out_bit;


static void gpio_init()
{
    GPIO_InitTypeDef   GPIO_InitStructure;

    JTAG_CLK_PIN_CLOCK_ENABLE();
    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = JTAG_CLK_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(JTAG_CLK_PIN_PORT, &GPIO_InitStructure);
    HAL_NVIC_SetPriority(JTAG_CLK_PIN_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(JTAG_CLK_PIN_IRQ);

    JTAG_TMS_PIN_CLOCK_ENABLE();
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = JTAG_TMS_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(JTAG_TMS_PIN_PORT, &GPIO_InitStructure);

    JTAG_TDI_PIN_CLOCK_ENABLE();
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = JTAG_TDI_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(JTAG_TDI_PIN_PORT, &GPIO_InitStructure);

    JTAG_TDO_PIN_CLOCK_ENABLE();
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = JTAG_TDO_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(JTAG_TDO_PIN_PORT, &GPIO_InitStructure);
}

void jtag_slave_driver_init()
{
    gpio_init();

    jtag_state = TEST_LOGIC_RESET;
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == JTAG_CLK_PIN) {
        __HAL_GPIO_EXTI_CLEAR_IT(JTAG_CLK_PIN);

        // posedge clk
        if (PIN_CLK_DATA() == GPIO_PIN_SET) {
            switch (jtag_state) {
                case TEST_LOGIC_RESET:
                    ir_reg = DTM_IDCODE;
                    shift_data = 0;
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? TEST_LOGIC_RESET : RUN_TEST_IDLE;
                    break;

                case RUN_TEST_IDLE:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? SELECT_DR        : RUN_TEST_IDLE;
                    break;

                case SELECT_DR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? SELECT_IR        : CAPTURE_DR;
                    break;

                case CAPTURE_DR:
                    switch (ir_reg) {
                        case DTM_BYPASS:
                            shift_data = 0x00;
                            break;

                        case DTM_IDCODE:
                            shift_data = IDCODE;
                            break;

                        case DTM_DTMCS:
                            shift_data = DTMCS;
                            break;

                        case DTM_DMI:
                            if (is_jtag_ex_busy()) {
                                shift_data = BUSY_RESPONSE;
                            } else {
                                shift_data = (((uint64_t)dm_reg) << DTM_DMI_ADDRESS_OFFSET) | jtag_slave_get_ex_data();
                            }
                            break;

                        default:
                            shift_data = 0x00;
                            break;
                    }
                    shift_out_bit = shift_data & 0x1;
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT1_DR         : SHIFT_DR;
                    break;

                case SHIFT_DR:
                    switch (ir_reg) {
                        case DTM_BYPASS:
                            shift_data = PIN_TDI_DATA();
                            shift_out_bit = PIN_TDI_DATA();
                            break;

                        case DTM_IDCODE:
                            shift_data = shift_data >> 1;
                            shift_out_bit = shift_data & 0x1;
                            if (PIN_TDI_DATA())
                                shift_data |= (uint64_t)(0x1 << (DTM_DMI_DATA_LENGTH - 1));
                            break;

                        case DTM_DTMCS:
                            shift_data = shift_data >> 1;
                            shift_out_bit = shift_data & 0x1;
                            if (PIN_TDI_DATA())
                                shift_data |= (uint64_t)(0x1 << (DTM_DMI_DATA_LENGTH - 1));
                            break;

                        case DTM_DMI:
                            shift_data = shift_data >> 1;
                            shift_out_bit = shift_data & 0x1;
                            if (PIN_TDI_DATA())
                                shift_data |= ((uint64_t)(0x1)) << (DTM_DMI_ADDRESS_LENGTH + DTM_DMI_DATA_LENGTH + DTM_DMI_OP_LENGTH - 1);
                            break;
                    }
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT1_DR         : SHIFT_DR;
                    break;

                case EXIT1_DR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? UPDATE_DR        : PAUSE_DR;
                    break;

                case PAUSE_DR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT2_DR         : PAUSE_DR;
                    break;

                case EXIT2_DR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? UPDATE_DR        : SHIFT_DR;
                    break;

                case UPDATE_DR:
                    if (ir_reg == DTM_DMI) {
                        if (!is_jtag_ex_busy()) {
                            dm_reg = (shift_data >> DTM_DMI_ADDRESS_OFFSET) & DTM_DMI_ADDRESS_MASK;
                            jtag_slave_dm_request(shift_data);
                        }
                    }
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? SELECT_DR        : RUN_TEST_IDLE;
                    break;

                case SELECT_IR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? TEST_LOGIC_RESET : CAPTURE_IR;
                    break;

                case CAPTURE_IR:
                    shift_data = 0x1;
                    shift_out_bit = shift_data & 0x1;
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT1_IR         : SHIFT_IR;
                    break;

                case SHIFT_IR:
                    shift_data = shift_data >> 1;
                    shift_out_bit = shift_data & 0x1;
                    if (PIN_TDI_DATA())
                        shift_data |= 0x1 << (IR_LENGTH - 1);
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT1_IR         : SHIFT_IR;
                    break;

                case EXIT1_IR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? UPDATE_IR        : PAUSE_IR;
                    break;

                case PAUSE_IR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? EXIT2_IR         : PAUSE_IR;
                    break;

                case EXIT2_IR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? UPDATE_IR        : SHIFT_IR;
                    break;

                case UPDATE_IR:
                    jtag_state = (jtag_state_t)(PIN_TMS_DATA()) ? SELECT_DR        : RUN_TEST_IDLE;
                    break;
            }
        // negedge
        } else {
            switch (jtag_state) {
                case TEST_LOGIC_RESET:
                    ir_reg = DTM_IDCODE;
                    break;

                case UPDATE_IR:
                    ir_reg = shift_data & DTM_REG_MASK;
                    break;

                case SHIFT_IR:
                case SHIFT_DR:
                    if (shift_out_bit & 0x1) {
                        PIN_TDO_SET();
                    } else {
                        PIN_TDO_CLR();
                    }
                    break;

                default:
                    PIN_TDO_CLR();
                    break;
            }
        }
    }
}
