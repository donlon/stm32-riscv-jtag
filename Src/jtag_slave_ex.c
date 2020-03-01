#include <stdint.h>
#include <stdbool.h>

#include "debug_defines.h"
#include "gdb_regs.h"
#include "stm32f1xx_hal.h"
#include "jtag_slave_driver.h"
#include "jtag_define.h"


#define get_field(reg, mask) (((reg) & (mask)) / ((mask) & ~((mask) << 1)))
#define set_field(reg, mask, val) (((reg) & ~(mask)) | (((val) * ((mask) & ~((mask) << 1))) & (mask)))


static volatile bool is_busy;
static volatile bool req_valid;

static uint8_t dm_address;
static uint32_t dm_data;
static uint8_t dm_op;
static uint64_t ex_data;
static uint8_t cmderr;

// debug regs
static uint32_t dcsr;

static uint32_t dmstatus;
static uint32_t dmcontrol;
static uint32_t hartinfo;
static uint32_t abstractcs;
//static uint32_t command;
static uint32_t data0;  // arg0/return value
static uint32_t data1;  // arg1
//static uint32_t data2;  // arg2
//static uint32_t progbuf0;
static uint32_t sbcs;
static uint32_t sbaddress0;
static uint32_t sbdata0;


void jtag_slave_ex_init()
{
    dmstatus = DMI_DMSTATUS_IMPEBREAK | DMI_DMSTATUS_ALLHALTED |
               DMI_DMSTATUS_ANYHALTED | DMI_DMSTATUS_AUTHENTICATED;
    dmstatus = set_field(dmstatus, DMI_DMSTATUS_VERSION, DTM_DM_VERSION_013);

    hartinfo = 0;

    sbcs = 0;
    sbcs = set_field(sbcs, DMI_SBCS_SBVERSION, 0x1);
    sbcs = set_field(sbcs, DMI_SBCS_SBACCESS, DMI_SBCS_SBACCESS_32BIT);
    sbcs = set_field(sbcs, DMI_SBCS_SBASIZE, 32);
    sbcs = set_field(sbcs, DMI_SBCS_SBACCESS32, 0x1);

    abstractcs = 0;
    abstractcs = set_field(abstractcs, DMI_ABSTRACTCS_PROGBUFSIZE, 0x1);
    abstractcs = set_field(abstractcs, DMI_ABSTRACTCS_DATACOUNT, 0x3);

    dcsr = (0x3 << 6);
}

bool is_jtag_ex_busy()
{
    return is_busy;
}

void jtag_slave_dm_request(uint64_t request)
{
    is_busy = true;

    dm_address = (request >> DTM_DMI_ADDRESS_OFFSET) & DTM_DMI_ADDRESS_MASK;
    dm_data = (request >> DTM_DMI_DATA_OFFSET) & DTM_DMI_DATA_MASK;
    dm_op = (request >> DTM_DMI_OP_OFFSET) & DTM_DMI_OP_MASK;

    req_valid = true;
}

uint64_t jtag_slave_get_ex_data()
{
    return ex_data;
}

static uint64_t set_ex_op(uint64_t data, uint8_t op)
{
    data = data << DTM_DMI_DATA_OFFSET;
    data |= op;

    return data;
}

static uint32_t read_reg(uint32_t reg)
{
    uint32_t val = 0;

    switch (reg) {
        case 0:
            __asm volatile("MOV %0, r0" : "=r" (val));
            break;

        case 1:
            __asm volatile("MOV %0, r1" : "=r" (val));
            break;
    }

    return val;
}

void jtag_slave_run()
{
    if (req_valid) {
        // read
        if (dm_op == DTM_DMI_OP_READ) {
            switch (dm_address) {
                case DMI_DMSTATUS:
                    ex_data = dmstatus;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_DMCONTROL:
                    ex_data = dmcontrol;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_HARTINFO:
                    ex_data = hartinfo;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_SBCS:
                    ex_data = sbcs;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_ABSTRACTCS:
                    ex_data = abstractcs;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_DATA0:
                    ex_data = data0;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    is_busy = false;
                    req_valid = false;
                    break;

                case DMI_SBDATA0:
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 1);
                    ex_data = sbdata0;
                    ex_data = set_ex_op(ex_data, DTM_DMI_OP_SUCC);
                    if (get_field(sbcs, DMI_SBCS_SBAUTOINCREMENT))
                        sbaddress0 += 4;
                    if (get_field(sbcs, DMI_SBCS_SBREADONDATA))
                        sbdata0 = (*((uint32_t *)sbaddress0));
                    is_busy = false;
                    req_valid = false;
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 0);
                    break;

                default:
                    ex_data = 0x0;
                    is_busy = false;
                    req_valid = false;
                    break;
            }
        // write
        } else if (dm_op == DTM_DMI_OP_WRITE) {
            switch (dm_address) {
                case DMI_DMCONTROL:
                    // 复位DM模块
                    if (get_field(dm_data, DMI_DMCONTROL_DMACTIVE) == 0) {
                        jtag_slave_ex_init();
                    }
                    dm_data &= ~(DMI_DMCONTROL_HARTSELHI | DMI_DMCONTROL_HARTSELLO);
                    // 只有一个hart
                    dm_data |= 1 << DMI_DMCONTROL_HARTSELLO_OFFSET;
                    dmcontrol = dm_data;
                    is_busy = false;
                    req_valid = false;
                    ex_data = 0x0;
                    break;

                case DMI_COMMAND:
                    abstractcs = set_field(abstractcs, DMI_ABSTRACTCS_BUSY, 1);
                    // access register
                    if (get_field(dm_data, DMI_COMMAND_CMDTYPE) == DMI_COMMAND_CMDTYPE_REG) {
                        // 寄存器位数大于32
                        if (get_field(dm_data, DMI_COMMAND_AARSIZE) > 2) {
                            cmderr = 2;
                        } else {
                            cmderr = 0;
                        }
                        abstractcs = set_field(abstractcs, DMI_ABSTRACTCS_CMDERR, cmderr);
                        if (cmderr != 0)
                            goto cmd_end;
                        // 读写操作
                        if (!get_field(dm_data, DMI_COMMAND_POSTEXEC)) {
                            // 读寄存器
                            if (!get_field(dm_data, DMI_COMMAND_WRITE)) {
                                if (get_field(dm_data, DMI_COMMAND_REGNO) == CSR_DCSR)
                                    data0 = dcsr;
                                else
                                    data0 = read_reg(get_field(dm_data, DMI_COMMAND_REGNO));
                            // 写寄存器
                            } else {
                                
                            }
                        // 执行progbuf里的指令
                        } else {
                            
                        }
                    // access memory
                    } else if (get_field(dm_data, DMI_COMMAND_CMDTYPE) == DMI_COMMAND_CMDTYPE_MEM) {
                        
                    // quick access
                    } else {
                        
                    }
cmd_end:
                    is_busy = false;
                    req_valid = false;
                    ex_data = 0x0;
                    abstractcs = set_field(abstractcs, DMI_ABSTRACTCS_BUSY, 0);
                    break;

                case DMI_DATA0:
                    data0 = dm_data;
                    is_busy = false;
                    req_valid = false;
                    ex_data = 0x0;
                    break;

                case DMI_DATA1:
                    data1 = dm_data;
                    is_busy = false;
                    req_valid = false;
                    ex_data = 0x0;
                    break;

                case DMI_SBCS:
                    sbcs = dm_data;
                    is_busy = false;
                    req_valid = false;
                    ex_data = 0x0;
                    break;

                case DMI_SBADDRESS0:
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 1);
                    sbaddress0 = dm_data;
                    if (get_field(sbcs, DMI_SBCS_SBREADONADDR))
                        sbdata0 = (*((uint32_t *)sbaddress0));
                    is_busy = false;
                    req_valid = false;
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 0);
                    ex_data = 0x0;
                    break;

                case DMI_SBDATA0:
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 1);
                    *((uint32_t *)sbaddress0) = dm_data;
                    if (get_field(sbcs, DMI_SBCS_SBAUTOINCREMENT))
                        sbaddress0 += 4;
                    is_busy = false;
                    req_valid = false;
                    sbcs = set_field(sbcs, DMI_SBCS_SBBUSY, 0);
                    ex_data = 0x0;
                    break;

                default:
                    ex_data = 0x0;
                    is_busy = false;
                    req_valid = false;
                    break;
            }
        // nop
        } else {
            ex_data = 0x0;
            is_busy = false;
            req_valid = false;
        }
    }
}
