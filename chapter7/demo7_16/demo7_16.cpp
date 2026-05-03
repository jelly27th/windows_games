/* 
  32-bit color rotation demo
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
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height);

int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename);

int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap);

int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds, int color);

int Rotate_Colors(int start_index, int end_index);

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

BITMAP_FILE           bitmap;                // holds the bitmap

char buffer[80];                             // general printing buffer

int green_id = -1, red_id = -1;              // these hold the ids of blinkers

/* function */

/* rotates the color between start and end */
int Rotate_Colors(int start_index, int end_index) {

  int colors = end_index - start_index + 1;

  static PALETTEENTRY work_pal[MAX_COLORS_PALETTE]; // working palette

  // get the color palette
  for (int i = 0; i < colors; i++) {
      work_pal[i] = bitmap.palette[start_index + i];
  }

  // shift the colors
  PALETTEENTRY last_color = work_pal[colors - 1];
  for (int i = colors - 1; i > 0; i--) {
    work_pal[i] = work_pal[i - 1];
  }
  work_pal[0] = last_color;

  // update shadow palette
  for (int i = 0; i < colors; i++) {
    bitmap.palette[start_index + i] = work_pal[i];
  }

  return 1;

}

/* opens a bitmap file and loads the data into bitmap */
int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename)
{

   int file_handle; // the file handle

   UCHAR   *temp_buffer = NULL; // used to convert 24 bit images to 16 bit
   OFSTRUCT file_data;          // the file data information

   // open the file if it exists
   if ((file_handle = OpenFile(filename,&file_data,OF_READ))==-1)
      return(0);

   // now load the bitmap file header
   _lread(file_handle, &bitmap->bitmapfileheader,sizeof(BITMAPFILEHEADER));

   // test if this is a bitmap file
   if (bitmap->bitmapfileheader.bfType!=BITMAP_ID) {
      _lclose(file_handle);
      return(0);
   }

   // now we know this is a bitmap, so read in all the sections

   // first the bitmap infoheader

   // now load the bitmap file header
   _lread(file_handle, &bitmap->bitmapinfoheader,sizeof(BITMAPINFOHEADER));

   // now load the color palette if there is one
   if (bitmap->bitmapinfoheader.biBitCount == 8) {
      _lread(file_handle, &bitmap->palette,MAX_COLORS_PALETTE*sizeof(PALETTEENTRY));

      // now set all the flags in the palette correctly and fix the reversed 
      // BGR RGBQUAD data format
      for (int index=0; index < MAX_COLORS_PALETTE; index++) {
         // reverse the red and green fields
         int temp_color                = bitmap->palette[index].peRed;
         bitmap->palette[index].peRed  = bitmap->palette[index].peBlue;
         bitmap->palette[index].peBlue = temp_color;
         
         // always set the flags word to this
         bitmap->palette[index].peFlags = PC_NOCOLLAPSE;
      }

   }

   // finally the image data itself
   _lseek(file_handle,-(int)(bitmap->bitmapinfoheader.biSizeImage),SEEK_END);

   // now read in the image, if the image is 8 or 16 bit then simply read it
   // but if its 24 bit then read it into a temporary area and then convert
   // it to a 16 bit image

   if (bitmap->bitmapinfoheader.biBitCount==8 || bitmap->bitmapinfoheader.biBitCount==16 || 
      bitmap->bitmapinfoheader.biBitCount==24) {
      // delete the last image if there was one
      if (bitmap->buffer) free(bitmap->buffer);

      // allocate the memory for the image
      if (!(bitmap->buffer = (UCHAR *)malloc(bitmap->bitmapinfoheader.biSizeImage))) {
         _lclose(file_handle);
         return 0;
      }

      // now read it in
      _lread(file_handle,bitmap->buffer,bitmap->bitmapinfoheader.biSizeImage);

   } else {
      // serious problem
      return 0;
   }

   // close the file
   _lclose(file_handle);

   // flip the bitmap
   Flip_Bitmap(bitmap->buffer, 
               bitmap->bitmapinfoheader.biWidth*(bitmap->bitmapinfoheader.biBitCount/8), 
               bitmap->bitmapinfoheader.biHeight);

   // return success
   return 1;

}

/* releases all memory associated with "bitmap" */
int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap) {

   if (bitmap->buffer) {
      free(bitmap->buffer);
      bitmap->buffer = NULL;
   }

   return 1;

}

/* used to flip bottom-up .BMP images */
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height) {

   UCHAR *buffer; // used to perform the image processing

   // allocate the temporary buffer
   if (!(buffer = (UCHAR *)malloc(bytes_per_line*height)))
      return 0;

   // copy image to work area
   memcpy(buffer,image,bytes_per_line*height);

   // flip vertically
   for (int index=0; index < height; index++)
      memcpy(&image[((height-1) - index)*bytes_per_line],
            &buffer[index*bytes_per_line], bytes_per_line);

   // release the memory
   free(buffer);

   // return success
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

   // copy the bitmap image to the primary buffer line by line
   // note this is a good candidate operation to make into a function - hint!

   // lock the primary surface
   lpddsprimary->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

   // get video pointer to primary surfce
   UCHAR *primary_buffer = (UCHAR *)ddsd.lpSurface;       

   // test if memory is linear
   if (ddsd.lPitch == SCREEN_WIDTH) {
      // copy memory from double buffer to primary buffer
      memcpy((void *)primary_buffer, (void *)bitmap.buffer, SCREEN_WIDTH*SCREEN_HEIGHT);
   } else { // non-linear

      // make copy of source and destination addresses
      UCHAR *dest_ptr = primary_buffer;
      UCHAR *src_ptr  = bitmap.buffer;

      // memory is non-linear, copy line by line
      for (int y=0; y < SCREEN_HEIGHT; y++) {

         for (int x=0; x < SCREEN_WIDTH; x++) {
            // 8-bit index
            UCHAR color_index = src_ptr[y * SCREEN_WIDTH + x];
            
            // Retrieve colors from the color palette
            PALETTEENTRY color = bitmap.palette[color_index];
            
            int dst_offset = y * ddsd.lPitch + x * 4;
            
            dest_ptr[dst_offset] = color.peBlue;      // B
            dest_ptr[dst_offset + 1] = color.peGreen; // G
            dest_ptr[dst_offset + 2] = color.peRed;   // R
            dest_ptr[dst_offset + 3] = 255; // Alpha:
         }
      }
   }

   // now unlock the primary surface
   if (FAILED(lpddsprimary->Unlock(NULL)))
      return(0);

  // animate the waterfall, the water color entries are from 133-143
  Rotate_Colors(133,143);

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



// load the 8-bit image
if (!Load_Bitmap_File(&bitmap,(char*)"../resource/waterfall8.bmp"))
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

   // unload the bitmap file, we no longer need it
   Unload_Bitmap_File(&bitmap);
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
                              L"Demo7_16",  // title
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