/* 
  8-bit software bitmap clipper demo
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
#define SCREEN_BPP 8 // bits per pixel

/* basic unsigned types */
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

// the happy face structure
typedef struct HAPPY_FACE_TYP {
  int x, y; // position of happy face
  int xv, yv; // velocity of happy face
} HAPPY_FACE, *HAPPY_FACE_PTR;

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

/* prototypes */
void Blit_Clipped(int x, int y, 
                 int width, int height,
                 UCHAR* bitmap,
                 UCHAR* video_buffer, int mempitch);

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

// a low tech bitmap that uses palette entry 1 for the color
UCHAR happy_bitmap[64] = {0,0,0,0,0,0,0,0,
                          0,0,1,1,1,1,0,0,
                          0,1,0,1,1,0,1,0,
                          0,1,1,1,1,1,1,0,
                          0,1,0,1,1,0,1,0,
                          0,1,1,0,0,1,1,0,
                          0,0,1,1,1,1,0,0,
                          0,0,0,0,0,0,0,0};

UCHAR sad_bitmap[64] = {0,0,0,0,0,0,0,0,
                        0,0,1,1,1,1,0,0,
                        0,1,0,1,1,0,1,0,
                        0,1,1,1,1,1,1,0,
                        0,1,1,0,0,1,1,0,
                        0,1,0,1,1,0,1,0,
                        0,0,1,1,1,1,0,0,
                        0,0,0,0,0,0,0,0};

HAPPY_FACE happy_faces[100]; // this holds all the happu faces

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

   DDBLTFX ddbltfx; // the blitter fx structure
   static int feeling_counter = 0; // tracks how we feel
   static int happy = 1; // let's start off being happy

   // make sure this isn't executed again
   if (window_closed) return 0;

    // for now test if the user is hitting ESC and send WM_CLOSE
    if (KEYDOWN(VK_ESCAPE)) {
      PostMessage(main_window_handle, WM_CLOSE, 0, 0);
      window_closed = 1;
    }

    // use the blitter to erase the back buffer
    // first initialize the DDBLTFX structure
    DDRAW_INIT_STRUCT(ddbltfx);
    // now set the color word info to the color we desire
    ddbltfx.dwFillColor = 0;

    // make the blitter call
    if (FAILED(lpddsback->Blt(NULL, // pointer to the dest RECT
                              NULL, // pointer to source surface
                              NULL, // pointer to source RECT
                              DDBLT_COLORFILL | DDBLT_WAIT, // control flags
                              &ddbltfx // pointer to DDBLTFX holding info
                                  ))) {
        return 0;
    }

    // initialize ddsd
    DDRAW_INIT_STRUCT(ddsd);

    // lock the back buffer surface
    if (FAILED(lpddsback->Lock(NULL,
                              &ddsd,
                              DDLOCK_SURFACEMEMORYPTR | DDBLT_WAIT,
                              &ddbltfx 
                                  ))) {
      return 0;
    }

    // increment how we feel    
    if (++feeling_counter > 200) {
      feeling_counter = 0;
      happy = -happy;
    }                   

    // draw all the happy face
    for (int face = 0; face < 1000; face++) {
      // are we happy or sad ?
      if (1 == happy) {
        // we are happy
        Blit_Clipped(happy_faces[face].x,
                    happy_faces[face].y,
                    8,8,
                    happy_bitmap,
                    (UCHAR*)ddsd.lpSurface,
                    ddsd.lPitch);
      } else {
        // we must be sad
        Blit_Clipped(happy_faces[face].x,
                    happy_faces[face].y,
                    8,8,
                    sad_bitmap,
                    (UCHAR*)ddsd.lpSurface,
                    ddsd.lPitch);
      }
    }

    // move all happy faces
    for (int face = 0; face < 100; face++){
      // move
      happy_faces[face].x += happy_faces[face].xv;
      happy_faces[face].y += happy_faces[face].yv;

      // check for off screen
      if (happy_faces[face].x > SCREEN_WIDTH) {
        happy_faces[face].x = -8;
      } else if (happy_faces[face].x < -8) {
        happy_faces[face].x = SCREEN_WIDTH;
      }

      if (happy_faces[face].y > SCREEN_HEIGHT) {
        happy_faces[face].y = -8;
      } else if (happy_faces[face].y < -8) {
        happy_faces[face].y = SCREEN_HEIGHT;
      }
    }

    // unlock surface
    if (FAILED(lpddsback->Unlock(NULL))) {
      return 0;
    }

    // flip the pages
    while (FAILED(lpddsprimary->Flip(NULL, DDFLIP_WAIT)));

    // wait a sec
    Sleep(30);

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
    
    // build up the palette data array
    for (int color=1; color < 255; color++) {
        // fill with random RGB values
        palette[color].peRed   = rand()%256;
        palette[color].peGreen = rand()%256;
        palette[color].peBlue  = rand()%256;

        // set flags field to PC_NOCOLLAPSE
        palette[color].peFlags = PC_NOCOLLAPSE;
    }

    // now fill in entry 0 and 255 with black and white
    palette[0].peRed     = 0;
    palette[0].peGreen   = 0;
    palette[0].peBlue    = 0;
    palette[0].peFlags   = PC_NOCOLLAPSE;

    palette[255].peRed   = 255;
    palette[255].peGreen = 255;
    palette[255].peBlue  = 255;
    palette[255].peFlags = PC_NOCOLLAPSE;

	// make color 1 yellow
	palette[1].peRed     = 255;
	palette[1].peGreen   = 255;
	palette[1].peBlue    = 0;
	palette[1].peFlags   = PC_NOCOLLAPSE;
    // create the palette object
    if (FAILED(lpdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | 
                                    DDPCAPS_INITIALIZE, 
                                    palette,&lpddpal, NULL)))
    return(0);

    // finally attach the palette to the primary surface
    if (FAILED(lpddsprimary->SetPalette(lpddpal)))
      return(0);
    
    // initialize all the happy faces
    for (int face = 0; face < 100; face++){
      // set random position
      happy_faces[face].x = rand()%SCREEN_WIDTH;
      happy_faces[face].y = rand()%SCREEN_HEIGHT;

      // set random velocity (-2,+2)
      happy_faces[face].xv = -2 + rand()%5;
      happy_faces[face].yv = -2 + rand()%5;
    }
    return 1;
}

/* 
    this is called after the game is exited and the main event loop 
    while is exited
*/
int Game_Shutdown(void* parms = NULL, int num_parms = 0) {
    
   // now the back buffer surface
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
                              L"Demo7_8",  // title
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

/*
  this function blits and clips bitmap the image sent in bitmaps to the 
  destination surface pointed to by video_buffer. the function assumes a
  640 * 480 * 8 mode
*/
void Blit_Clipped(int x, int y, // position to draw bitmap
                 int width, int height, // size of bitmap in pixels
                 UCHAR* bitmap, // pointer to bitmap data
                 UCHAR* video_buffer, // pointer to video buffer surface
                 int mempitch) // video pitch per line
{

  // first do tricial rejections of bitmap, it is totally incvisible?
  if (x + width < 0 || x >= SCREEN_WIDTH || y + height < 0 || y >= SCREEN_HEIGHT)
  {
    return;
  }

  // clip souce rectangle
  // pre-compute the bounding rect to make life easy
  int x1 = x;
  int y1 = y;
  int x2 = x + width - 1;
  int y2 = y + height - 1;

  // upper left hand corner first
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;

  // now lower right hand corner
  if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
  if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;

  // now we know to draw only the portions of the bitmap from (x1,y1) to (x2,y2)
  // compute offsets into bitmap on x,y axes, we need this to compute starting point
  // to rasterize from
  int x_off = x1 - x;
  int y_off = y1 - y;

  // compute number of colums and rows to blit
  int dx = x2 - x1 + 1;
  int dy = y2 - y1 + 1;

  // compute the starting point in the video buffer
  video_buffer += (x1+ y1*mempitch);

  // compute starting point in the bitmap to scan data from
  bitmap += (y_off * width + x_off);

  // at this point bitmap is pointing to the first pixel in the bitmap that needs to
  // be blitted, and video_buffer is pointing to the memory location on the destination
  // buffer to put it, so now enter rasterizer loop
  UCHAR pixel;

  for (int index_y = 0; index_y < dy; index_y++) {
    // inner loop, where the action takes place
    for (int index_x = 0; index_x < dx; index_x++) {
      // read pixel from source bitmap, test for transparency and plot
      if ((pixel = bitmap[index_x])) video_buffer[index_x] = pixel;

      // advance pointers
      video_buffer += mempitch; // bytes per scanline
      bitmap += width; // bytes per row in bitmap
    }
  }
}