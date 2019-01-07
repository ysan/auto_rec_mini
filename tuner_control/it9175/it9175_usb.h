/* fsusb2i   (c) 2015 trinity19683
  IT9175 USB commands
  it9175_usb.h
  2015-12-25
*/
#pragma once
#include <stdint.h>

int it9175_ctrl_msg(void* const, const uint8_t cmd, const uint8_t mbox, const uint8_t wrlen, const uint8_t rdlen);
int it9175_usbSetTimeout(void* const pst);

/*EOF*/