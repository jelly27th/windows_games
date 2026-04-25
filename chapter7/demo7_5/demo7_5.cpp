/* 
  32-bit page flipping demo
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

/* initialize a direct draw structure */
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct, 0, sizeof(ddstruct)); ddstruct.dwSize = sizeof(ddstruct); }

/* global variables */
HWND main_window_handle = NULL; // global track main window
HINSTANCE hinstance_app = NULL; // global track hinstance
int window_closed = 0; // track if window is closed

/* directdraw stuff*/
LPDIRECTDRAW7 lpdd = NULL; // the directdraw object
LPDIRECTDRAWSURFACE7 lpddsprimary = NULL; // the directdraw primary surface
LPDIRECTDRAWSURFACE7 lpddsback = NULL; // the directdraw back surface
LPDIRECTDRAWPALETTE   lpddpal      = NULL;   // a pointer to the created dd palette
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

/* main game loop */
int Game_Main(void* parms = NULL, int num_parms = 0) {

   // make sure this isn't executed again
   if (window_closed) return 0;

    // for now test if the user is hitting ESC and send WM_CLOSE
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(main_window_handle, WM_CLOSE, 0, 0);
      window_closed = 1;
    }

    /* copy the double buffer into the back buffer */
    DDRAW_INIT_STRUCT(ddsd);

    // lock the back buffer
    if (FAILED(lpddsback->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL))) {
        return 0;
    }

    // alias pointer to back buffer surface
    UCHAR* back_buffer = (UCHAR*)ddsd.lpSurface;

    // teset if memory is linear
    if (SCREEN_WIDTH == ddsd.lPitch)
    {
      // copy memory from double buffer to back buffer
      memset(back_buffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
    }
    else // no linear
    {
      // make copy of vedio pointer
      UCHAR* dest_ptr = back_buffer;

      // clear out memory one line at a time
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        memset(dest_ptr, 0, SCREEN_WIDTH*4);// clear line
        
        // advance pointers to next line
        dest_ptr += ddsd.lPitch;

      }
    }

    // draw the next frame into the back buffer, notice that we
    // must use the lpitch since it's a surface and may not be linear

    // plot 5000 random pixels
    for (int index=0; index < 5000; index++) {
        int   x   = rand()%SCREEN_WIDTH;
        int   y   = rand()%SCREEN_HEIGHT;
        // 32 bit = 4 byte / pixel
        int pixel_offset = 4*x + y*ddsd.lPitch;
        back_buffer[pixel_offset] = rand() % 256; // Blue
        back_buffer[pixel_offset + 1] = rand() % 256; // Green
        back_buffer[pixel_offset + 2] = rand() % 256; // Red
        back_buffer[pixel_offset+ 3] = 255; // Alpha
    }

    // unlock the back buffer
    if (FAILED(lpddsback->Unlock(NULL))) {
      return 0;
    }

    // perform the flip
    while (FAILED(lpddsprimary->Flip(NULL, DDFLIP_WAIT)));

    Sleep(500); // 500 sec

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
    if (FAILED(lpdd->SetCooperativeLevel(main_window_handle, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | 
	                                                         DDSCL_ALLOWMODEX | DDSCL_ALLOWREBOOT))) {
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
    ddsd.dwFlags =  DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

    // set the backbuffer count field to 1, use 2 for triple buffering
    ddsd.dwBackBufferCount = 1;

    // request a complex, flipable
    ddsd.ddsCaps.dwCaps =  DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    // create the primary surface
    if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL))) {
      PostQuitMessage(0);
      return 0;
    }
    
    // now query for attached surface from the primary surface

    // this line is needed by the call
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

    // get the attached back buffer surface
    if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback))) {
      return 0;
    }

    return 1;
}

/* 
    this is called after the game is exited and the main event loop 
    while is exited
*/
int Game_Shutdown(void* parms = NULL, int num_parms = 0) {

    // first the palette
    if (lpddpal)
    {
      lpddpal->Release();
      lpddpal = NULL;
    }

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
                              L"Demo7_5",  // title
                              WS_POPUP | WS_VISIBLE,
                              0,0,          // initial x, y
                              GetSystemMetrics(SM_CXSCREEN), 
							                GetSystemMetrics(SM_CYSCREEN), // initial width, height    
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