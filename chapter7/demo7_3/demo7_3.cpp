/* 
  basic full-screen 24-bit color pixel plotting DirectDraw demo
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
#define SCREEN_BPP 32 // bits per pixel

/* basic unsigned types */
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

/* macro */
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
#define _RGB32BIT(a,r,g,b) ((b) + (g << 8) + (r << 16) + (a << 24))

/* initialize a direct draw structure */
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct, 0, sizeof(ddstruct)); ddstruct.dwSize = sizeof(ddstruct); }

/* global variables */
HWND main_window_handle = NULL; // global track main window
HINSTANCE hinstance_app = NULL; // global track hinstance

/* directdraw stuff*/
LPDIRECTDRAW7 lpdd = NULL; // the directdraw object
LPDIRECTDRAWSURFACE7 lpddsprimary = NULL; // the directdraw primary surface
LPDIRECTDRAWSURFACE7 lpddsback = NULL; // the directdraw back surface
LPDIRECTDRAWCLIPPER lpddclipper = NULL; // the directdraw clipper
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

/*
  this function plots a pixel in 32-bit color mode,
  assuming that the caller already locked the surface
  and is sending a pointer and byte pitch to it
*/
inline void Plot_Pixel_32(int x, int y, 
                          int alpha, int red, int green, int blue, 
                          UINT *video_buffer, int lpitch32){

  // first build up color WORD
  UINT pixel = _RGB32BIT(alpha, red, green, blue);

  // write the data
  video_buffer[x + y*lpitch32] = pixel;

}

/* main game loop */
int Game_Main(void* parms = NULL, int num_parms = 0) {

    // for now test if the user is hitting ESC and send WM_CLOSE
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(main_window_handle, WM_CLOSE, 0, 0);
    }

    // plot 1000 random pixels to the primary surface and return
    // clear ddsd and set size, never assume it's clea
    DDRAW_INIT_STRUCT(ddsd);

    if (FAILED(lpddsprimary->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))) {
        return 0;
    }

    // now ddsd.lPitch is valid and so is ddsd.lpSurface

    // make a couple aliases to make code cleaner, so we don't have to cast
    int lpitch32 = (int)(ddsd.lPitch >> 2);
    UINT* video_buffer = (UINT*)ddsd.lpSurface;

    // plot 1000 random pixels with random colors on the
    // primary surface, they will be instantly visible
    for (int index = 0; index < 1000; index++) {
        
        // select random position and color for 640 * 480 * 16
        int x = rand() % SCREEN_WIDTH;
        int y = rand() % SCREEN_HEIGHT;
        int red = rand() % 256;
        int green = rand() % 256;
        int blue = rand() % 256;

        // plot the pixel
        Plot_Pixel_32(x, y, 0, red, green, blue, video_buffer, lpitch32);
    }

    // now unlock the primary surface
    if (FAILED(lpddsprimary->Unlock(NULL))) {
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
        PostQuitMessage(0);
        return 0;
    }

    // set cooperation to full screen 
    if (FAILED(lpdd->SetCooperativeLevel(main_window_handle, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT))) {
        PostQuitMessage(0);
        return 0;
    }

    if (FAILED(lpdd->SetDisplayMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, 0, 0))) {
      PostQuitMessage(0);
      return 0;
    }
    // clear ddsd and set size
    DDRAW_INIT_STRUCT(ddsd);

    // enable valid fields
    ddsd.dwFlags =  DDSD_CAPS;

    // request primary surface
    ddsd.ddsCaps.dwCaps =  DDSCAPS_PRIMARYSURFACE;

    // create the primary surface
    if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL))) {
      PostQuitMessage(0);
      return 0;
    }
    
    return 1;
}

/* 
    this is called after the game is exited and the main event loop 
    while is exited
*/
int Game_Shutdown(void* parms = NULL, int num_parms = 0) {

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
  winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
                              L"Demo7_3",  // title
                              WS_POPUP | WS_VISIBLE,
                              0,0,          // initial x, y
                              GetSystemMetrics(SM_CXSCREEN), // initial width, height
                              GetSystemMetrics(SM_CYSCREEN),   
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