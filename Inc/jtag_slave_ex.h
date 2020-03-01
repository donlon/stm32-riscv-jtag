#ifndef _JTAG_SLAVE_EX_H_
#define _JTAG_SLAVE_EX_H_

void jtag_slave_ex_init();
bool is_jtag_ex_busy();
void jtag_slave_dm_request(uint64_t request);
uint64_t jtag_slave_get_ex_data();
void jtag_slave_run();

#endif
