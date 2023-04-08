#pragma once

/**
 * Read OTP and SEEPROM.
 *
 * If an error occurs, a message will be displayed and the
 * user will be prompted to press a button.
 *
 * @param buf Buffer (must be 0x600 bytes)
 * @param index Row index for error messages
 * @return 0 on success; non-zero on error.
 */
int read_otp_seeprom(void *buf, int index);

void option_DumpOtpAndSeeprom(void);
