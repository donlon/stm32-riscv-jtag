#ifndef _JTAG_DEFINE_H_
#define _JTAG_DEFINE_H_

#define DBUS_IDLE_CYCLES  (5)

#define IDCODE            (0x1e200a6f)
#define DTMCS             ((DBUS_IDLE_CYCLES << 12) | (DTM_DMI_ADDRESS_LENGTH << 4) | 0x1)
#define BUSY_RESPONSE     (0x3)

#endif
