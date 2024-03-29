#pragma once

// Fonts used: Terminus 1.49 - ter-u16b.bdf, ter-u24b.bdf
// - http://terminus-font.sourceforge.net/
// Character encoding is ASCII.

// NOTE: Only characters [32,128) are included due to size constraints.

// Decompressed Terminus font data.
typedef struct _terminus_font {
    // Terminus 8x16, bold
    // Used as the GamePad font. (854x480 -> 106x30)
    unsigned char  ter_u16b[128-32][16];

    // Terminus 12x24, bold
    // Used as the TV font. (1280x720 -> 106x30)
    unsigned short ter_u24b[128-32][24];
} terminus_font;

// The following data is compressed using LZO1X and should be
// decompressed using MiniLZO first. The decompressed data can
// be decoded using the terminus_font struct.
static const unsigned char terminus_lzo1x[1674] = {
    0x12,0x00,0x2f,0x01,0x00,0x18,0xa0,0x00,0x5c,0x00,0x9b,0x01,0x66,0x66,0x66,0x2c,
    0x7d,0x00,0x36,0x01,0x00,0x7f,0x89,0x00,0x36,0x98,0x01,0x0a,0x08,0x08,0x3e,0x6b,
    0x0b,0x0b,0x3e,0x68,0x68,0x6b,0x3e,0x08,0x08,0x83,0x06,0x6b,0x36,0x30,0x0f,0x0e,
    0x0c,0x6c,0xd6,0xdc,0x06,0x07,0x1c,0x36,0x36,0x1c,0x6e,0x3b,0x33,0x33,0x3b,0x6e,
    0xf8,0x0b,0x2c,0x3f,0x01,0x30,0x18,0x0c,0x82,0x00,0x18,0x30,0xbc,0x01,0x40,0x01,
    0x82,0x00,0x18,0x0c,0x28,0xc8,0x01,0x01,0x1c,0x7f,0x1c,0x36,0x2b,0x89,0x02,0x7e,
    0x2e,0x14,0x01,0x70,0x02,0x28,0xc2,0x00,0x00,0x7f,0x32,0x5c,0x03,0xbf,0x00,0x60,
    0x60,0x30,0x7f,0x13,0x0c,0x06,0x06,0xbc,0x01,0x07,0x3e,0x63,0x63,0x73,0x7b,0x6f,
    0x67,0x63,0x63,0x3e,0xde,0x04,0x1c,0x1e,0xa5,0x20,0x7e,0x27,0x7d,0x00,0x60,0x53,
    0x11,0x06,0x03,0x7f,0x28,0x3f,0x00,0x3c,0x60,0x60,0x27,0xbc,0x00,0x07,0x60,0x70,
    0x78,0x6c,0x66,0x63,0x7f,0x60,0x60,0x60,0xc9,0x0f,0x03,0x01,0x00,0x3f,0x54,0x01,
    0xfe,0x03,0x3c,0x06,0x5e,0x01,0x63,0x63,0x27,0xbd,0x00,0x7f,0xe0,0x10,0xd9,0x15,
    0x3e,0x68,0x03,0x29,0x7c,0x00,0x8d,0x01,0x7e,0x51,0x04,0x1e,0x2c,0xe8,0x02,0x29,
    0xc4,0x03,0xdc,0x01,0xc0,0x08,0x94,0x13,0x55,0x26,0x60,0x2a,0xf4,0x03,0x29,0x00,
    0x04,0x8c,0x03,0x6c,0x04,0x28,0x7d,0x01,0x30,0x28,0x7c,0x07,0x5e,0x01,0x73,0x6b,
    0x43,0x00,0x73,0x03,0x7e,0x29,0xfd,0x01,0x7f,0x70,0x00,0xbc,0x01,0x6c,0x17,0x9d,
    0x17,0x3f,0x27,0x7c,0x00,0x45,0x1c,0x03,0x27,0xbf,0x02,0x1f,0x33,0x63,0x82,0x00,
    0x33,0x1f,0x28,0xfd,0x03,0x1f,0x65,0x04,0x7f,0x2d,0x3d,0x00,0x03,0x29,0xfd,0x00,
    0x7b,0x28,0xbc,0x03,0x27,0xb8,0x01,0xdd,0x0d,0x3c,0xc2,0x50,0x18,0x3c,0xbd,0x01,
    0x78,0xbb,0x41,0x33,0x33,0x1e,0xfc,0x05,0x03,0x33,0x1b,0x0f,0x0f,0x1b,0x33,0xfd,
    0x05,0x03,0xe0,0x00,0xdc,0x0d,0x02,0x41,0x63,0x77,0x7f,0x6b,0x29,0x3c,0x01,0x40,
    0x01,0x01,0x67,0x6f,0x7b,0x73,0x27,0x3e,0x00,0x3e,0x63,0xc1,0x00,0x3e,0x28,0x7e,
    0x03,0x63,0x3f,0x2b,0x7c,0x02,0x83,0x00,0x7b,0x3e,0x60,0x29,0x7c,0x00,0x60,0x0e,
    0xff,0x03,0x03,0x03,0x3e,0x29,0xfd,0x07,0xff,0xfd,0x15,0x18,0x28,0x3c,0x03,0x2a,
    0x7c,0x01,0x8c,0x01,0x56,0x65,0x1c,0x1c,0x29,0x3c,0x00,0x02,0x6b,0x7f,0x77,0x63,
    0x41,0xfc,0x01,0x6e,0x03,0x36,0x36,0xfe,0x13,0xc3,0xc3,0x05,0xdc,0x3c,0x29,0x3c,
    0x01,0x7c,0x45,0x78,0x4f,0xfe,0x1b,0x3c,0x0c,0xc0,0x00,0xde,0x23,0x06,0x06,0x4c,
    0x67,0x4c,0x66,0xfe,0x4f,0x3c,0x30,0xc0,0x00,0xbc,0x03,0x01,0x18,0x3c,0x66,0x00,
    0x36,0x01,0x00,0x7f,0x74,0x6d,0x31,0x5f,0x00,0x3e,0x60,0x7e,0x6c,0x50,0x27,0x7c,
    0x05,0x88,0x20,0xfc,0x3f,0xc9,0x20,0x03,0x27,0xfd,0x0b,0x60,0x94,0x05,0x27,0xbc,
    0x00,0xa8,0x28,0x54,0x3e,0xdd,0x03,0x78,0x09,0x27,0x3f,0xa0,0x16,0x27,0x7c,0x00,
    0xdf,0x05,0x60,0x60,0x3e,0x2a,0x7c,0x01,0xdc,0x1f,0x5d,0x1e,0x1c,0x2a,0xfc,0x07,
    0x5e,0x1a,0x70,0x60,0x80,0x00,0x5c,0x22,0xbc,0x05,0x64,0x40,0x27,0x3d,0x06,0x1c,
    0x2d,0xbc,0x08,0x69,0x36,0x6b,0x80,0x00,0x28,0x3c,0x00,0x2a,0x7c,0x01,0x27,0x89,
    0x07,0x3e,0x2d,0x7d,0x00,0x3f,0x27,0x08,0x08,0x27,0x7c,0x02,0xcb,0x2a,0x7b,0x0f,
    0x07,0x28,0x7c,0x08,0x7c,0x03,0x84,0x40,0xdd,0x21,0x0c,0xfd,0x1b,0x78,0x27,0x7d,
    0x00,0x63,0x2d,0x3d,0x04,0x63,0xac,0x3c,0x29,0x3c,0x00,0x79,0x13,0x3e,0x29,0x3f,
    0x00,0x36,0x1c,0x36,0x29,0x7d,0x02,0x63,0x2a,0x7c,0x04,0x7c,0x34,0x2a,0xfd,0x11,
    0x38,0x49,0x0d,0x06,0x7d,0x0d,0x38,0x2b,0xbc,0x16,0x27,0xbd,0x08,0x0e,0x45,0x01,
    0x30,0x79,0x01,0x0e,0x98,0x01,0x01,0xce,0xdb,0x73,0x00,0x20,0x33,0x01,0x00,0x60,
    0x31,0x04,0x00,0x28,0x5c,0x00,0x2b,0x02,0x00,0x01,0x98,0xe4,0x00,0x20,0x09,0x78,
    0x01,0xe6,0x06,0x07,0xfe,0x28,0xf4,0x00,0x28,0x2c,0x00,0x32,0x34,0x02,0x04,0x01,
    0xf8,0x03,0x6c,0x06,0x66,0x00,0x65,0x00,0x6c,0x4e,0x01,0x60,0x06,0x66,0x00,0x66,
    0x03,0x4c,0x01,0x30,0x44,0x02,0x9c,0x5a,0x06,0x1c,0x03,0x36,0x01,0xb6,0x01,0x9c,
    0x00,0xc0,0x44,0x00,0x64,0x04,0x08,0x30,0x00,0x30,0x03,0x98,0x06,0xd8,0x06,0xcc,
    0x03,0x8c,0x31,0x40,0x02,0x02,0x70,0x00,0xd8,0x01,0x8c,0x66,0x00,0x00,0xd8,0x0e,
    0x02,0x06,0x78,0x57,0x04,0x86,0x03,0x06,0x45,0x00,0x86,0x06,0x02,0x06,0x78,0x37,
    0x6c,0x04,0x20,0x09,0x01,0x00,0xc0,0xa6,0x10,0x00,0x18,0x2b,0x04,0x00,0x65,0x02,
    0x60,0x4c,0x03,0x31,0x38,0x1b,0x85,0x03,0x60,0x94,0x17,0x28,0x04,0x00,0xbd,0x18,
    0x18,0x36,0x92,0x01,0x03,0x06,0xbe,0x17,0x07,0xff,0xbe,0x19,0x03,0x06,0x20,0x05,
    0x96,0x07,0x07,0xfe,0x20,0x12,0x44,0x03,0x2a,0x25,0x04,0x30,0x3c,0x98,0x00,0x5c,
    0x0b,0x20,0x1d,0x1c,0x0a,0x32,0xac,0x03,0x46,0x00,0x01,0x80,0x04,0x00,0x2a,0x7c,
    0x07,0x8f,0x2a,0x0c,0x00,0x0c,0x31,0xbe,0x09,0xf8,0x03,0x21,0x47,0x20,0x06,0x06,
    0x07,0x04,0x00,0x08,0x86,0x06,0xc6,0x06,0x66,0x06,0x36,0x06,0x1e,0x06,0x0e,0x66,
    0x02,0x03,0x0c,0x44,0x48,0x32,0xde,0x01,0x70,0x00,0x0d,0x7d,0x6c,0x32,0x1e,0x0c,
    0x03,0xfc,0x38,0x7c,0x01,0x40,0x00,0x9c,0x12,0xdc,0x3d,0x75,0x11,0x06,0x32,0xf4,
    0x03,0xdd,0x05,0x00,0x64,0x00,0x5f,0x05,0xf0,0x03,0x00,0xac,0x01,0x36,0x3c,0x02,
    0x0c,0x06,0x00,0x07,0x00,0x07,0x80,0x06,0xc0,0x06,0x60,0x06,0x30,0x06,0x18,0x06,
    0xce,0x0d,0x07,0xfe,0xb4,0x06,0x33,0xc5,0x05,0x06,0xe7,0x00,0x01,0xfe,0x03,0x28,
    0x30,0x00,0x35,0x7e,0x01,0x03,0xf8,0x64,0x15,0x27,0xbd,0x00,0x06,0x28,0x00,0x00,
    0x34,0xbc,0x00,0x44,0x0f,0xcc,0x1d,0x2d,0x14,0x06,0x35,0xbc,0x06,0x29,0x7c,0x04,
    0x84,0x0a,0x2c,0x34,0x00,0x3c,0xbc,0x00,0x43,0x00,0x0c,0x07,0xf8,0xe4,0x18,0x5d,
    0x0d,0xfc,0x20,0x08,0xbc,0x08,0x20,0x01,0x4c,0x0b,0x32,0xbc,0x00,0x31,0x3c,0x0b,
    0x2e,0x4c,0x07,0x4c,0x00,0xa5,0x74,0xc0,0x0d,0x05,0x03,0x20,0x06,0xec,0x0b,0x3b,
    0x14,0x0c,0x2d,0x44,0x01,0x2d,0xb4,0x01,0x39,0x7c,0x04,0x27,0xad,0x00,0x60,0x3a,
    0xc6,0x03,0x01,0xfc,0x4c,0x3a,0x04,0x03,0x07,0xc3,0x06,0x63,0x06,0x33,0x27,0x07,
    0x00,0x63,0x07,0xc3,0x43,0x1b,0x06,0x07,0xfc,0x3f,0xfd,0x05,0x06,0xdc,0x3d,0xe8,
    0x5c,0x30,0xbc,0x00,0x27,0xcd,0x08,0x03,0x2e,0xfc,0x08,0x5c,0x01,0x38,0x7d,0x01,
    0x00,0x2b,0x04,0x00,0x39,0x3c,0x08,0x2b,0x4d,0x01,0x06,0x3f,0x7c,0x01,0x2a,0xfc,
    0x0b,0x64,0x5a,0x2a,0x35,0x00,0x07,0x20,0x0e,0xbc,0x00,0x3a,0xfc,0x05,0xc5,0x0a,
    0xc6,0x3e,0xbc,0x0c,0x2c,0xcc,0x02,0x2c,0x34,0x05,0x33,0x3d,0x05,0xf8,0x33,0x05,
    0x1e,0x60,0xa4,0xe1,0x30,0xbc,0x00,0x01,0x0f,0xc0,0x03,0x00,0x2f,0x04,0x00,0x7c,
    0xd7,0x64,0xc2,0x33,0x3c,0x02,0x47,0x03,0x86,0x00,0xc6,0x54,0xe9,0x02,0x36,0x00,
    0x1e,0x00,0x0e,0x4d,0x00,0x36,0x4c,0x01,0x01,0xc6,0x01,0x86,0x03,0x33,0x3c,0x02,
    0x2d,0x3d,0x04,0x06,0x3e,0x3e,0x05,0x04,0x01,0x54,0x4d,0x08,0x07,0x07,0x8f,0x06,
    0xdb,0x06,0x73,0x06,0x23,0x06,0x03,0x2c,0x04,0x00,0x39,0x7d,0x04,0x0e,0x5d,0xb2,
    0x36,0x5c,0xb3,0x01,0xc6,0x07,0x86,0x07,0x3b,0x7c,0x04,0x2c,0x7c,0x0a,0x3e,0xfc,
    0x05,0x2a,0xfd,0x08,0x03,0x20,0x0a,0x7c,0x07,0x2f,0x03,0x00,0x66,0x03,0xcc,0x4c,
    0x03,0x2f,0xc8,0x00,0x2f,0x7c,0x01,0x3d,0x3c,0x05,0xbc,0x0b,0xd5,0x0f,0x0c,0xa4,
    0x0a,0x3a,0x7c,0x16,0x29,0x84,0x1e,0x36,0x64,0x26,0x39,0xbc,0x09,0x20,0x01,0x7c,
    0x04,0x28,0x74,0x00,0xa4,0x00,0x25,0x1d,0x26,0xf0,0x44,0x00,0x34,0x7c,0x01,0x2d,
    0x44,0x07,0x4d,0x3c,0x73,0x0e,0x7a,0x07,0x8f,0x0a,0x7c,0x06,0x03,0x0c,0x7e,0x34,
    0x7c,0x01,0xfc,0x0a,0x8d,0x0a,0xf0,0x64,0x01,0x64,0x02,0x34,0x7c,0x07,0x2d,0xbd,
    0x00,0xf0,0x3e,0xbc,0x03,0xe4,0xe1,0x2f,0xe4,0x14,0x35,0x7c,0x0a,0x4d,0x62,0x18,
    0x37,0x05,0x00,0xf8,0x33,0x85,0x16,0x0c,0x84,0xb4,0x26,0xcc,0x26,0x23,0x64,0x26,
    0x64,0xdf,0x64,0x6d,0x33,0x7d,0x01,0xc0,0x37,0x05,0x00,0xf8,0x2f,0x0d,0x1a,0xf0,
    0x6e,0x21,0x06,0x06,0x20,0x29,0xf8,0x2e,0x29,0xcc,0x03,0xb4,0xca,0x20,0x16,0x00,
    0x00,0x5c,0xc6,0x90,0x28,0x5c,0xef,0xfc,0x61,0x4c,0xf1,0x38,0x7c,0x10,0x30,0x9c,
    0x16,0x34,0x7c,0x16,0x33,0x1c,0x0c,0x84,0xc0,0x36,0xfd,0x23,0x06,0x2e,0x1d,0x02,
    0x06,0x3b,0x3c,0x02,0x2f,0x1c,0x0f,0xe7,0xbc,0x06,0x0c,0x03,0x32,0x7d,0x0d,0xc0,
    0x27,0x6c,0x27,0x20,0x04,0x7c,0x0d,0x27,0xdc,0x00,0x2f,0x3d,0x02,0x07,0x25,0xac,
    0x22,0x2d,0x9c,0x22,0x37,0x7c,0x04,0x34,0xfc,0x0b,0x27,0xdd,0x01,0x78,0x20,0x05,
    0xfc,0x17,0xa4,0xbe,0x5c,0x00,0x32,0x1d,0x18,0x00,0xa4,0x69,0x54,0x68,0x2c,0x34,
    0x0b,0x45,0x00,0x06,0x67,0x03,0x8c,0x00,0xcc,0x21,0xa5,0x2b,0x3c,0x4d,0x00,0xcc,
    0x21,0xcc,0x30,0x4c,0x02,0x31,0x3d,0x02,0x78,0x20,0x0d,0x3c,0x1a,0x29,0x5e,0x14,
    0x66,0x06,0x2f,0x04,0x00,0x3b,0xbc,0x00,0x20,0x05,0x7c,0x04,0x37,0x9c,0x16,0x35,
    0x3c,0x2f,0x37,0x7c,0x01,0x2b,0x34,0x18,0x20,0x02,0x7c,0x07,0x2a,0x2c,0x2a,0x31,
    0xbd,0x00,0xe6,0xdc,0xe9,0x3d,0x7c,0x19,0x27,0xdd,0x06,0xfc,0xf4,0x5f,0x44,0x01,
    0xe4,0xc0,0x32,0x7d,0x2b,0x30,0xa4,0x00,0x4d,0x15,0x30,0x2e,0x06,0x00,0x03,0xe0,
    0x39,0x58,0x29,0x30,0x00,0x00,0x3b,0xbc,0x0c,0x28,0x14,0x18,0x3c,0xfc,0x17,0x2f,
    0x7c,0x01,0x29,0x76,0x07,0x03,0xfc,0x3c,0xbc,0x00,0xcc,0x51,0x27,0xd4,0x13,0x3a,
    0x7c,0x07,0x30,0xf8,0x02,0x35,0x7c,0x0d,0x29,0x1c,0x18,0x20,0x04,0xfc,0x35,0x26,
    0x3c,0x3e,0xa5,0x00,0x1c,0x29,0x2f,0x00,0x60,0x01,0xc0,0x20,0x06,0x3c,0x44,0x3a,
    0x7d,0x10,0x1c,0xd4,0xc4,0xa4,0x64,0x2a,0x6c,0x34,0x74,0x0d,0x2c,0x8d,0x03,0x3c,
    0x95,0x26,0xc6,0x20,0x33,0x18,0x47,0x11,0x00,0x00
};
