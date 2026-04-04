
#ifndef BLACKOUT
#define BLACKOUT

// default screen size
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP    8  // bits per pixel
#define MAX_COLORS    256

// these read the keyboard asyncharonously
#define KEY_DOWM(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// initializes a direct draw struct
#define DD_INIT_STRUCT(ddstruct)            \
  {                                         \
    memset(&ddstruct, 0, sizeof(ddstruct)); \
    ddstruct.dwSize = sizeof(ddstruct);     \
  }                                         \

// basic unsigned types
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;



#endif