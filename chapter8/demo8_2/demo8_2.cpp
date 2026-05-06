/* 
  32-bit line clipping program
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

#define BITMAP_ID 0x4D42 // universal id for a bitmap
#define MAX_COLORS_PALETTE 256 // maximum colors in 256 color palette

/* basic unsigned types */
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

// container structure for bitmaps .BMP file
typedef struct BITMAP_FILE_TAG {
    BITMAPFILEHEADER bitmapfileheader;  // this contains the bitmapfile header
    BITMAPINFOHEADER bitmapinfoheader;  // this is all the info including the palette
    PALETTEENTRY     palette[256];      // we will store the palette here
    UCHAR            *buffer;           // this is a pointer to the data
} BITMAP_FILE, *BITMAP_FILE_PTR;

/* prototypes */
int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds, int color);

int Draw_Line(int x0, int y0, int x1, int y1, UCHAR color, 
                           UCHAR *vb_start, int lpitch);

int Draw_Clip_Line(int x0,int y0, int x1, int y1,UCHAR color, 
                    UCHAR *dest_buffer, int lpitch);

int Clip_Line(int &x1,int &y1,int &x2, int &y2);

/* macro */
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

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
DDSURFACEDESC2 ddsd; // a direct draw surface description structure
DDBLTFX ddbltfx; // used to fill
DDSCAPS2 ddscaps; // a direct draw surface capabilities structure
HRESULT ddrval; // result back from directdraw calls
DWORD start_clock_count = 0; // used for timing

// global clipping region
int min_clip_x = ((SCREEN_WIDTH/2) - 100),      // clipping rectangle 
    max_clip_x = ((SCREEN_WIDTH/2) + 100),
    min_clip_y = ((SCREEN_HEIGHT/2) - 100),
    max_clip_y = ((SCREEN_HEIGHT/2) + 100);

char buffer[80];                             // general printing buffer

/* function */

/* plots a single pixel at x,y with color */
inline int Draw_Pixel(int x, int y, DWORD color, UCHAR *video_buffer, int lpitch)
{
   int pixel_offset = y * ddsd.lPitch + x * 4;
   video_buffer[pixel_offset] = (UCHAR)(color & 0x000000FF);        // Blue
   video_buffer[pixel_offset + 1] = (UCHAR)((color & 0x0000FF00) >> 8); // Green
   video_buffer[pixel_offset + 2] = (UCHAR)((color & 0x00FF0000) >> 16); // Red
   video_buffer[pixel_offset + 3] = (UCHAR)((color & 0xFF000000) >> 24); // Alpha

   return 1;
}

int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,int color) {
  DDBLTFX ddbltfx; // this contains the DDBLTFX structure

  // clear out the structure and set the size field 
  DDRAW_INIT_STRUCT(ddbltfx);

  // set the dwfillcolor field to the desired color
  ddbltfx.dwFillColor = color; 

  // ready to blt to surface
  lpdds->Blt(NULL,       // ptr to dest rectangle
            NULL,       // ptr to source surface, NA            
            NULL,       // ptr to source rectangle, NA
            DDBLT_COLORFILL | DDBLT_WAIT,   // fill and wait                   
            &ddbltfx);  // ptr to DDBLTFX structure

  // return success
  return 1;
}

/* draws a line from xo,yo to x1,y1 using differential error
   terms (based on Bresenahams work) */
int Draw_Line(int x0, int y0, // starting position 
              int x1, int y1, // ending position
              DWORD color,    // color
              UCHAR *vb_start, int lpitch) // video buffer and memory pitch
{

   int dx;             // difference in x's
   int dy;             // difference in y's
   int dx2;            // dx,dy * 2
   int dy2; 
   int x_inc;          // amount in pixel space to move during drawing
   int y_inc;          // amount in pixel space to move during drawing
   int error;          // the discriminant i.e. error i.e. decision variable

   // pre-compute first pixel address in video buffer
   vb_start = vb_start + x0*4 + y0*lpitch;

   // compute horizontal and vertical deltas
   dx = x1-x0;
   dy = y1-y0;

   // test which direction the line is going in i.e. slope angle
   if (dx>=0) {
      x_inc = 4;
   } else {
      x_inc = -4;
      dx    = -dx;  // need absolute value
   }

   // test y component of slope
   if (dy>=0) {
      y_inc = lpitch;
   } else {
      y_inc = -lpitch;
      dy    = -dy;  // need absolute value
   }

   // compute (dx,dy) * 2
   dx2 = dx << 1;
   dy2 = dy << 1;

   // now based on which delta is greater we can draw the line
   if (dx > dy) { /* |slope| <= 1 */

      // initialize error term
      error = dy2 - dx; 

      // draw the line
      for (int index=0; index <= dx; index++) {
         // set the pixel
         *((DWORD*)vb_start) = color;

         // test if error has overflowed
         if (error >= 0) {
            error-=dx2;
            // move to next line
            vb_start+=y_inc;
         }

         // adjust the error term
         error+=dy2;

         // move to the next pixel
         vb_start+=x_inc;
      }

   } else { /* |slope| > 1 */

      // initialize error term
      error = dx2 - dy; 

      // draw the line
      for (int index=0; index <= dy; index++) {
         // set the pixel
         *((DWORD*)vb_start) = color;

         // test if error overflowed
         if (error >= 0) {
            error-=dy2;
            // move to next line
            vb_start+=x_inc;
         }

         // adjust the error term
         error+=dx2;

         // move to the next pixel
         vb_start+=y_inc;
      }
   }
   return 1;
}

/* draws a clipped line */
int Draw_Clip_Line(int x0,int y0, int x1, int y1, DWORD color, 
                    UCHAR *dest_buffer, int lpitch)
{

   int cxs, cys, cxe, cye;

   // clip and draw each line
   cxs = x0;
   cys = y0;
   cxe = x1;
   cye = y1;

   // clip the line
   if (Clip_Line(cxs,cys,cxe,cye)) {
      Draw_Line(cxs, cys, cxe,cye,color,dest_buffer,lpitch);
   }

   return 1;

}

/* clips the sent line using the globally defined clipping region */
int Clip_Line(int &x1,int &y1,int &x2, int &y2) {
   // internal clipping codes
   #define CLIP_CODE_C  0x0000
   #define CLIP_CODE_N  0x0008
   #define CLIP_CODE_S  0x0004
   #define CLIP_CODE_E  0x0002
   #define CLIP_CODE_W  0x0001

   #define CLIP_CODE_NE 0x000a
   #define CLIP_CODE_SE 0x0006
   #define CLIP_CODE_NW 0x0009 
   #define CLIP_CODE_SW 0x0005

   int xc1=x1, 
      yc1=y1, 
      xc2=x2, 
      yc2=y2;

   int p1_code=0, 
      p2_code=0;

   // determine codes for p1 and p2
   if (y1 < min_clip_y)
      p1_code|=CLIP_CODE_N;
   else if (y1 > max_clip_y)
      p1_code|=CLIP_CODE_S;

   if (x1 < min_clip_x)
      p1_code|=CLIP_CODE_W;
   else if (x1 > max_clip_x)
      p1_code|=CLIP_CODE_E;

   if (y2 < min_clip_y)
      p2_code|=CLIP_CODE_N;
   else if (y2 > max_clip_y)
      p2_code|=CLIP_CODE_S;

   if (x2 < min_clip_x)
      p2_code|=CLIP_CODE_W;
   else if (x2 > max_clip_x)
      p2_code|=CLIP_CODE_E;

   // try and trivially reject
   if ((p1_code & p2_code)) 
      return 0;

   // test for totally visible, if so leave points untouched
   if (p1_code==0 && p2_code==0)
      return 1;

   // determine end clip point for p1
   switch(p1_code) {
      case CLIP_CODE_C: break;

      case CLIP_CODE_N:
      {
         yc1 = min_clip_y;
         xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);
      } break;
      case CLIP_CODE_S:
      {
         yc1 = max_clip_y;
         xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);
      } break;

      case CLIP_CODE_W:
      {
         xc1 = min_clip_x;
         yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);
      } break;
         
      case CLIP_CODE_E:
      {
         xc1 = max_clip_x;
         yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
      } break;

      // these cases are more complex, must compute 2 intersections
      case CLIP_CODE_NE:
      {
         // north hline intersection
         yc1 = min_clip_y;
         xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);

         // test if intersection is valid, of so then done, else compute next
         if (xc1 < min_clip_x || xc1 > max_clip_x) {
            // east vline intersection
            xc1 = max_clip_x;
            yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
         }
      } break;
      
      case CLIP_CODE_SE:
      {
         // south hline intersection
         yc1 = max_clip_y;
         xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	

         // test if intersection is valid, of so then done, else compute next
         if (xc1 < min_clip_x || xc1 > max_clip_x) {
            // east vline intersection
            xc1 = max_clip_x;
            yc1 = y1 + 0.5+(max_clip_x-x1)*(y2-y1)/(x2-x1);
         }

      } break;
         
      case CLIP_CODE_NW: 
      {
         // north hline intersection
         yc1 = min_clip_y;
         xc1 = x1 + 0.5+(min_clip_y-y1)*(x2-x1)/(y2-y1);
         
         // test if intersection is valid, of so then done, else compute next
         if (xc1 < min_clip_x || xc1 > max_clip_x) {
            xc1 = min_clip_x;
            yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
         }

      } break;
         
      case CLIP_CODE_SW:
      {
         // south hline intersection
         yc1 = max_clip_y;
         xc1 = x1 + 0.5+(max_clip_y-y1)*(x2-x1)/(y2-y1);	
         
         // test if intersection is valid, of so then done, else compute next
         if (xc1 < min_clip_x || xc1 > max_clip_x) {
            xc1 = min_clip_x;
            yc1 = y1 + 0.5+(min_clip_x-x1)*(y2-y1)/(x2-x1);	
         }

      } break;

      default:break;

      }

   // determine clip point for p2
   switch(p2_code) {
      case CLIP_CODE_C: break;

      case CLIP_CODE_N:  
      {
         yc2 = min_clip_y;
         xc2 = x2 + (min_clip_y-y2)*(x1-x2)/(y1-y2);
      } break;

      case CLIP_CODE_S:
      {
         yc2 = max_clip_y;
         xc2 = x2 + (max_clip_y-y2)*(x1-x2)/(y1-y2);
      } break;

      case CLIP_CODE_W:
      {
         xc2 = min_clip_x;
         yc2 = y2 + (min_clip_x-x2)*(y1-y2)/(x1-x2);
      } break;
         
      case CLIP_CODE_E:
      {
         xc2 = max_clip_x;
         yc2 = y2 + (max_clip_x-x2)*(y1-y2)/(x1-x2);
      } break;

         // these cases are more complex, must compute 2 intersections
      case CLIP_CODE_NE:
      {
         // north hline intersection
         yc2 = min_clip_y;
         xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

         // test if intersection is valid, of so then done, else compute next
         if (xc2 < min_clip_x || xc2 > max_clip_x) {
            // east vline intersection
            xc2 = max_clip_x;
            yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
         }

      } break;
      
      case CLIP_CODE_SE:
      {
         // south hline intersection
         yc2 = max_clip_y;
         xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	

         // test if intersection is valid, of so then done, else compute next
         if (xc2 < min_clip_x || xc2 > max_clip_x)
            {
         // east vline intersection
         xc2 = max_clip_x;
         yc2 = y2 + 0.5+(max_clip_x-x2)*(y1-y2)/(x1-x2);
         }

      } break;
         
      case CLIP_CODE_NW: 
      {
         // north hline intersection
         yc2 = min_clip_y;
         xc2 = x2 + 0.5+(min_clip_y-y2)*(x1-x2)/(y1-y2);

         // test if intersection is valid, of so then done, else compute next
         if (xc2 < min_clip_x || xc2 > max_clip_x)
            {
         xc2 = min_clip_x;
            yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
         }

      } break;
         
      case CLIP_CODE_SW:
      {
         // south hline intersection
         yc2 = max_clip_y;
         xc2 = x2 + 0.5+(max_clip_y-y2)*(x1-x2)/(y1-y2);	
         
         // test if intersection is valid, of so then done, else compute next
         if (xc2 < min_clip_x || xc2 > max_clip_x) {
            xc2 = min_clip_x;
            yc2 = y2 + 0.5+(min_clip_x-x2)*(y1-y2)/(x1-x2);	
         }
      } break;
      
      default:break;

      }

   // do bounds check
   if ((xc1 < min_clip_x) || (xc1 > max_clip_x) ||
      (yc1 < min_clip_y) || (yc1 > max_clip_y) ||
      (xc2 < min_clip_x) || (xc2 > max_clip_x) ||
      (yc2 < min_clip_y) || (yc2 > max_clip_y) )
      {
      return 0;
      } // end if

   // store vars back
   x1 = xc1;
   y1 = yc1;
   x2 = xc2;
   y2 = yc2;

   return 1;

}

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
      PostMessage(main_window_handle, WM_CLOSE, 0, 0);
      window_closed = 1;
    }


   DDRAW_INIT_STRUCT(ddsd);

   // lock the primary surface
   lpddsprimary->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

   // draw 1000 random lines
   for (int index=0; index < 1000; index++) {
      DWORD color = _RGB32BIT(255,rand()%256,rand()%256,rand()%256);
      Draw_Clip_Line(rand()%SCREEN_WIDTH, rand()%SCREEN_HEIGHT,
                     rand()%SCREEN_WIDTH, rand()%SCREEN_HEIGHT,
                     color,
                     (UCHAR *)ddsd.lpSurface, ddsd.lPitch);

    }

   // now unlock the primary surface
   if (FAILED(lpddsprimary->Unlock(NULL)))
      return(0);

   // wait a sec
   Sleep(33);

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
   ddsd.dwFlags = DDSD_CAPS;

   // request primary surface
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

   // create the primary surface
   if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL)))
      return(0);

   // clean the surface
   DDraw_Fill_Surface(lpddsprimary,0);

   // return success or failure or your own return code here
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
                              L"Demo8_1",  // title
                              WS_POPUP | WS_VISIBLE,
                              0,0,          // initial x, y
                              SCREEN_WIDTH, 
                              SCREEN_HEIGHT, // initial width, height 
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