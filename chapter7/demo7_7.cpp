/* 
  16-bit surface to surface blitter demo
*/

#define WIN32_LEAN_AND_MEAN  // just say no to MFC

#define INITGUID // make sure the directx GUIDs are included

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <iostream>
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>

#include <ddraw.h> // include directdraw

#define WINDOW_CLASS_NAME L"WINCLASS1"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP 16 // bits per pixel

/* basic unsigned types */
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

/* macro */
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

/* this build a 16 bit color value in 5.5.5 format (1-bit alpha mode) */
#define _RGB16BIT555(r,g,b) ((b & 0x1f) + ((g & 0x1f) << 5) + ((r & 0x1f) << 10))

/* this build a 16 bit color value in 5.6.5 format (green dominate mode) */
#define _RGB16BIT565(r, g, b) ((b & 0x1f) + ((g & 0x3f) << 5) + ((r & 0x1f) << 11))

// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
#define _RGB32BIT(a,r,g,b) ((b) + (g << 8) + (r << 16) + (a << 24))

/* initialize a direct draw structure */
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct, 0, sizeof(ddstruct)); ddstruct.dwSize = sizeof(ddstruct); }

/* global variables */
HWND main_window_handle = NULL; // global track main window
int window_closed = 0; // track if window is closed
HINSTANCE hinstance_app = NULL; // global track hinstance

/* directdraw stuff*/
LPDIRECTDRAW7 lpdd = NULL;                   // the directdraw object
LPDIRECTDRAWSURFACE7 lpddsprimary = NULL;    // the directdraw primary surface
LPDIRECTDRAWSURFACE7 lpddsback = NULL;       // the directdraw back surface
LPDIRECTDRAWPALETTE   lpddpal      = NULL;   // a pointer to the created dd palette
LPDIRECTDRAWCLIPPER lpddclipper = NULL;      // the directdraw clipper
PALETTEENTRY          palette[256];          // color palette
PALETTEENTRY          save_palette[256];     // used to save palettes
DDSURFACEDESC2 ddsd; // a direct draw surface description structure
DDBLTFX ddbltfx; // used to fill
DDSCAPS2 ddscaps; // a direct draw surface capabilities structure
HRESULT ddrval; // result back from directdraw calls
DWORD start_clock_count = 0; // used for timing

char buffer[80]; // general printing buffer

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  
  PAINTSTRUCT ps;
  HDC hdc;
  char buffer[80]; // used to print strings

  switch (msg) {
    case WM_CREATE: {
      // do initialization stuff here

      return 0;
    } break;
    
    case WM_PAINT: {
      // simple validate the window
      hdc = BeginPaint(hwnd, &ps);
      // you would do your painting here

      EndPaint(hwnd, &ps);
      return 0;
    } break;

    case WM_DESTROY: {
      // kill the application, this sends a WM_QUIT message
      PostQuitMessage(0);
      return 0;
    } break;

    default:
      break;
  }

  // process any message that we didn't take care of
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* main game loop */
int Game_Main(void* parms = NULL, int num_parms = 0) {

   RECT source_rect; // used to hold the destination RECT
   RECT dest_rect; // used to hold the destination RECT

   // make sure this isn't executed again
   if (window_closed) return 0;

    // for now test if the user is hitting ESC and send WM_CLOSE
    if (KEYDOWN(VK_ESCAPE)) {
      PostMessage(main_window_handle, WM_CLOSE, 0, 0);
      window_closed = 1;
    }

    // get a random rectangle for source
    int x1 = rand()%SCREEN_WIDTH;
    int y1 = rand()%SCREEN_HEIGHT;
    int x2 = rand()%SCREEN_WIDTH;
    int y2 = rand()%SCREEN_HEIGHT;

	// get a random rectangle for destination
    int x3 = rand()%SCREEN_WIDTH;
    int y3 = rand()%SCREEN_HEIGHT;
    int x4 = rand()%SCREEN_WIDTH;
    int y4 = rand()%SCREEN_HEIGHT;
    // now set up the RECT structure to fill the region from
    // (x1,y1) to (x2,y2) on the source surface
    source_rect.left = x1;
    source_rect.top = y1;
    source_rect.right = x2;
    source_rect.bottom = y2;
    // (x3,y3) to (x4,y4) on the destination surface
    dest_rect.left = x3;
    dest_rect.top = y3;
    dest_rect.right = x4;
    dest_rect.bottom = y4;

    // make the blitter call
    if (FAILED(lpddsprimary->Blt(&dest_rect, // pointer to the dest RECT
                                  lpddsback, // pointer to source surface
                                  &source_rect, // pointer to source RECT
                                  DDBLT_WAIT, // control flags
                                  NULL // pointer to DDBLTFX holding info
                                  ))) {
        return 0;
    }

    // return success or failure or your own return code here
    return 1;
}

/*  
    this is call once after the initial window is created  and
    before the main game loop is entered
*/
int Game_Init(void* parms = NULL, int num_parms = 0) {

    // create IDirectDraw instance 7.0 object and test for error
    if (FAILED(DirectDrawCreateEx(NULL, (void**)&lpdd, IID_IDirectDraw7, NULL))) {
        return 0;
    }

    // set cooperation to full screen 
    if (FAILED(lpdd->SetCooperativeLevel(main_window_handle, 
                                         DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | 
	                                       DDSCL_ALLOWMODEX | DDSCL_ALLOWREBOOT))) {
        return 0;
    }

    if (FAILED(lpdd->SetDisplayMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, 0, 0))) {
      return 0;
    }

    // clear ddsd and set size
    DDRAW_INIT_STRUCT(ddsd);

    // enable valid fields
    ddsd.dwFlags =  DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

    // set the backbuffer count field to 1, use 2 for triple buffering
	ddsd.dwBackBufferCount = 1;
    // request a complex, flipable
    ddsd.ddsCaps.dwCaps =  DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    // create the primary surface
    if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL))) {
      return 0;
    }
   // now query for attached surface from the primary surface
   
   // this line is need by the call
   ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
   
   // get the attached back buffer surface
   if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback))) {
   	 return 0;
   }
   
   // draw a green gradient in back buffer
   DDRAW_INIT_STRUCT(ddsd);
   
   // lock the back buffer
   if (FAILED(lpddsback->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))) {
    return 0;
   }
	
   // get alias to start of surface memory for fast addressing
   USHORT* video_buffer = (USHORT*)ddsd.lpSurface;
   
   // draw the gradient
   for (int index_y = 0; index_y < SCREEN_HEIGHT; index_y++) {
      // build color word up
      DWORD color = _RGB16BIT565(0, (index_y >> 3), 0);

      // replicate color in upper and lower 16 bits of 32-bits word
      color = (color) | (color << 16);

      // now color has two pixel in it in 16.16 or RGB.RGB format, use
      // a DWORD or 32-bit copy to move the bytes into the next video
      // line, we'll need inline assembly though...

      // draw next line, use a little inline asm baby!
      #ifdef _MSC_VER
        // Visual Studio
        __asm {
            CLD                       ; clear direction of copy to forward  
            MOV EAX, color            ; color goes here 
            MOV ECX, (SCREEN_WIDTH/2) ; number of DWORDS goes here 
            MOV EDI, video_buffer     ; address of line to move data
            REP STOSD                 ; send the pentium X on its way
        }
      #elif defined(__GNUC__) && defined(__i386__)
       // GCC x86
        asm volatile (
            "cld\n\t"
            "rep stosl"
            : 
            : "a"(color), "c"((SCREEN_WIDTH/2)), "D"(video_buffer)
            : "memory", "cc"
        );
      #endif

      // now advance video buffer to next line
      video_buffer += (ddsd.lPitch >> 1);
   }

   if (FAILED(lpddsback->Unlock(NULL))) {
    return 0;
   }
    
    return 1;
}

/* 
    this is called after the game is exited and the main event loop 
    while is exited
*/
int Game_Shutdown(void* parms = NULL, int num_parms = 0) {
    
    // now the back buffer surface
    if (lpddsback)
    {
    lpddsback->Release();
    lpddsback = NULL;
    } 

    // now the primary surface
    if (lpddsprimary) {
      lpddsprimary->Release();
      lpddsprimary = NULL;
    }    

    // now blow away the IDirectDraw4 interface
    if (lpdd) {
        lpdd->Release();
        lpdd = NULL;
    }

    return 1;
}

/* main entry point */
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance,
                   LPSTR lpcmdline, int ncmdshow) {
  WNDCLASSEX winclass;
  HWND hwnd;
  MSG msg;
  HDC hdc;

  // first fill in the window class structure
  winclass.cbSize = sizeof(WNDCLASSEX);
  winclass.style = CS_DBLCLKS | CS_OWNDC | 
                   CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc = WindowProc;
  winclass.cbClsExtra = 0;
  winclass.cbWndExtra = 0;
  winclass.hInstance = hinstance;
  winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  winclass.lpszMenuName = NULL;
  winclass.lpszClassName = WINDOW_CLASS_NAME;
  winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  // save instance in global
  hinstance_app = hinstance;
  
  // register the window class
  if (!RegisterClassEx(&winclass)) {
    return 0;
  }

  // create the window
  if (!(hwnd = CreateWindowEx((DWORD)NULL,           // extended style
                              WINDOW_CLASS_NAME,    // class
                              L"Demo7_7",  // title
                              WS_POPUP | WS_VISIBLE,
                              0,0,          // initial x, y
                              SCREEN_WIDTH, SCREEN_HEIGHT, // initial width, height   
                              NULL,       // handle to parent
                              NULL,       // handle to menu
                              hinstance,  // histance of this application
                              NULL)))     // extra creation parms
  {
    return 0;
  }

  // save the main window handle
  main_window_handle = hwnd;

  // initialize the game
  Game_Init();

  // enter main event loop, but this time we use PeekMessage()
  // instead of GetMessage() to retrieve messages
  while (TRUE) {

    // test if there is a message in queue, if so get it
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // test if this is a quit
      if (WM_QUIT == msg.message) break;
      // translate any accelerator keys
      TranslateMessage(&msg);

      // send the message to the window proc
      DispatchMessage(&msg);

    }

    /* main game processing goes here */
    Game_Main();

  }

   // close dowm the game here
   Game_Shutdown();

  return msg.wParam;
}