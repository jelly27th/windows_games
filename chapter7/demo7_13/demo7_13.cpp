/* 
  32-bit blitting demo
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

// this will hold our little alien
typedef struct ALIEN_OBJ_TYP {
  LPDIRECTDRAWSURFACE7 frames[3]; // 3 frames of animation for complete walk cycle
  int x, y; // position of alien
  int velocity; // x-velocity
  int current_frame; // current frame of animation
  int counter; // used to time animation
} ALIEN_OBJ, *ALIEN_OBJ_PTR;

/* prototypes */
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height);

int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename);

int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap);

int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lppds, int color);

int Scan_Image_Bitmap(BITMAP_FILE_PTR bitmap, LPDIRECTDRAWSURFACE7 lpdds, int cx, int cy);

LPDIRECTDRAWSURFACE7 DDraw_Create_Surface(int width, int height, int mem_flags, int color_key);

int DDraw_Draw_Surface(LPDIRECTDRAWSURFACE7 source, int x, int y, int width, int height, LPDIRECTDRAWSURFACE7 dest, int transparent);

LPDIRECTDRAWCLIPPER DDraw_Attach_Cliper(LPDIRECTDRAWSURFACE7 lpdds, int num_rects, LPRECT clip_list);

int Draw_Text_GDI(char* text, int x, int y, COLORREF color, LPDIRECTDRAWSURFACE7 lpdds);

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

ALIEN_OBJ             aliens[3];             // 3 aliens, one on each level

LPDIRECTDRAWSURFACE7  lpddsbackground = NULL;// this will hold the background image

char buffer[80];                             // general printing buffer

/* function */

/* opens a bitmap file and loads the data into bitmap */
int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename)
{

   int file_handle;  // the file handle

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
      for (int index=0; index < MAX_COLORS_PALETTE; index++)
         {
         // reverse the red and green fields
         int temp_color                = bitmap->palette[index].peRed;
         bitmap->palette[index].peRed  = bitmap->palette[index].peBlue;
         bitmap->palette[index].peBlue = temp_color;
         
         // always set the flags word to this
         bitmap->palette[index].peFlags = PC_NOCOLLAPSE;
         } // end for index

      }

   // finally the image data itself
   _lseek(file_handle,-(int)(bitmap->bitmapinfoheader.biSizeImage),SEEK_END);

   // now read in the image, if the image is 8 or 16 bit then simply read it
   // but if its 24 bit then read it into a temporary area and then convert
   // it to a 16 bit image

   if (bitmap->bitmapinfoheader.biBitCount==8 || bitmap->bitmapinfoheader.biBitCount==16 || 
    bitmap->bitmapinfoheader.biBitCount==24)
   {
      // delete the last image if there was one
      if (bitmap->buffer) free(bitmap->buffer);

      // allocate the memory for the image
      if (!(bitmap->buffer = (UCHAR *)malloc(bitmap->bitmapinfoheader.biSizeImage)))
      {
         _lclose(file_handle);
         return 0;
      }

      // now read it in
      _lread(file_handle,bitmap->buffer,bitmap->bitmapinfoheader.biSizeImage);
   } else {
      // serious problem
      return(0);
   }

   // close the file
   _lclose(file_handle);

   // flip the bitmap
   Flip_Bitmap(bitmap->buffer, 
               bitmap->bitmapinfoheader.biWidth*(bitmap->bitmapinfoheader.biBitCount/8), 
               bitmap->bitmapinfoheader.biHeight);

   // return success
   return(1);
}

/* releases all memory associated with "bitmap" */
int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap)
{

   if (bitmap->buffer) {
      free(bitmap->buffer);
      bitmap->buffer = NULL;
   }

   return(1);
}

/* used to flip bottom-up .BMP images */
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height) {

   UCHAR *buffer; // used to perform the image processing
   int index;     // looping index

   // allocate the temporary buffer
   if (!(buffer = (UCHAR *)malloc(bytes_per_line*height)))
      return(0);

   // copy image to work area
   memcpy(buffer,image,bytes_per_line*height);

   // flip vertically
   for (index=0; index < height; index++)
      memcpy(&image[((height-1) - index)*bytes_per_line],
            &buffer[index*bytes_per_line], bytes_per_line);

   // release the memory
   free(buffer);

   // return success
   return(1);

}

/* creates a clipper from the sent clip list and attaches
   it to the sent surface */
LPDIRECTDRAWCLIPPER DDraw_Attach_Clipper(LPDIRECTDRAWSURFACE7 lpdds, int num_rects, LPRECT clip_list) {

  int index;                         // looping var
  LPDIRECTDRAWCLIPPER lpddclipper;   // pointer to the newly created dd clipper
  LPRGNDATA region_data;             // pointer to the region data that contains
                                    // the header and clip list
  
  // first create the direct draw clipper
   if (FAILED(lpdd->CreateClipper(0,&lpddclipper,NULL)))
      return(NULL);
   
   // now create the clip list from the sent data

   // first allocate memory for region data
  region_data = (LPRGNDATA)malloc(sizeof(RGNDATAHEADER)+num_rects*sizeof(RECT));

  // now copy the rects into region data
  memcpy(region_data->Buffer, clip_list, sizeof(RECT)*num_rects);

  // set up fields of header
   region_data->rdh.dwSize          = sizeof(RGNDATAHEADER);
   region_data->rdh.iType           = RDH_RECTANGLES;
   region_data->rdh.nCount          = num_rects;
   region_data->rdh.nRgnSize        = num_rects*sizeof(RECT);

   region_data->rdh.rcBound.left    =  64000;
   region_data->rdh.rcBound.top     =  64000;
   region_data->rdh.rcBound.right   = -64000;
   region_data->rdh.rcBound.bottom  = -64000;

   // find bounds of all clipping regions
   for (index=0; index<num_rects; index++) {
    // test if the next rectangle unioned with the current bound is larger
    if (clip_list[index].left < region_data->rdh.rcBound.left)
       region_data->rdh.rcBound.left = clip_list[index].left;

    if (clip_list[index].right > region_data->rdh.rcBound.right)
       region_data->rdh.rcBound.right = clip_list[index].right;

    if (clip_list[index].top < region_data->rdh.rcBound.top)
       region_data->rdh.rcBound.top = clip_list[index].top;

    if (clip_list[index].bottom > region_data->rdh.rcBound.bottom)
       region_data->rdh.rcBound.bottom = clip_list[index].bottom;

    }

    // now we have computed the bounding rectangle region and set up the data
    // now let's set the clipping list

    if (FAILED(lpddclipper->SetClipList(region_data, 0))) {
      // release memory and return error
      free(region_data);
      return(NULL);
    }

   // now attach the clipper to the surface
   if (FAILED(lpdds->SetClipper(lpddclipper))) {
      // release memory and return error
      free(region_data);
      return(NULL);
   }

   // all is well, so release memory and send back the pointer to the new clipper
   free(region_data);
   return(lpddclipper);

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
  return(1);
}

/*
  draw a bob at the x,y defined in the BOB
  on the destination surface defined in dest
*/
int DDraw_Draw_Surface(LPDIRECTDRAWSURFACE7 source, // source surface to draw
                      int x, int y,                 // position to draw at
                      int width, int height,        // size of source surface
                      LPDIRECTDRAWSURFACE7 dest,    // surface to draw the surface on
                      int transparent = 1)          // transparency flag
{
   RECT dest_rect;   // the destination rectangle
   RECT source_rect; // the source rectangle    

   // fill in the destination rect
   dest_rect.left   = x;
   dest_rect.top    = y;
   dest_rect.right  = x+width-1;
   dest_rect.bottom = y+height-1;

   // fill in the source rect
   source_rect.left    = 0;
   source_rect.top     = 0;
   source_rect.right   = width-1;
   source_rect.bottom  = height-1;

   // test transparency flag

   if (transparent) {
      // enable color key blit
      // blt to destination surface
      if (FAILED(dest->Blt(&dest_rect, source,
                        &source_rect,(DDBLT_WAIT | DDBLT_KEYSRC),
                        NULL)))
            return 0;

   } else {
      // perform blit without color key
      // blt to destination surface
      if (FAILED(dest->Blt(&dest_rect, source,
                        &source_rect,(DDBLT_WAIT),
                        NULL)))
            return 0;

   }

return 1;

}

/* extracts a bitmap out of a bitmap file */
int Scan_Image_Bitmap(BITMAP_FILE_PTR bitmap,     // bitmap file to scan image data from
                      LPDIRECTDRAWSURFACE7 lpdds, // surface to hold data
                      int cx, int cy)             // cell to scan image from
{
   DDSURFACEDESC2 ddsd = {0};  // direct draw surface description
   ddsd.dwSize = sizeof(ddsd);
    
   if (FAILED(lpdds->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL))) {
        return 0;
   }
    
    int cell_width = ddsd.dwWidth;
    int cell_height = ddsd.dwHeight;
    int bmp_width = bitmap->bitmapinfoheader.biWidth;
    int bmp_bpp = bitmap->bitmapinfoheader.biBitCount;
    
    // compute position to start scanning bits from
    int start_x = cx * (cell_width + 1) + 1;
    int start_y = cy * (cell_height + 1) + 1;
    
    // assign a pointer to the memory surface for manipulation
    UCHAR* dest_ptr = (UCHAR*)ddsd.lpSurface;
    
    if (bmp_bpp == 8) {
        // iterate thru each scanline and copy bitmap
        for (int y = 0; y < cell_height; y++) {
            for (int x = 0; x < cell_width; x++) {
                int src_x = start_x + x;
                int src_y = start_y + y;
                
                // 8-bit index
                UCHAR color_index = bitmap->buffer[src_y * bmp_width + src_x];
                
                // Retrieve colors from the color palette
                PALETTEENTRY color = bitmap->palette[color_index];
                
                int dst_offset = y * ddsd.lPitch + x * 4;
                
                dest_ptr[dst_offset] = color.peBlue;      // B
                dest_ptr[dst_offset + 1] = color.peGreen; // G
                dest_ptr[dst_offset + 2] = color.peRed;   // R
                
                // Alpha: If the index is 0 and the color is pure black, 
                // it is transparent
                if (color_index == 0 && color.peRed == 0 && 
                    color.peGreen == 0 && color.peBlue == 0) {
                    dest_ptr[dst_offset + 3] = 0;
                } else {
                    dest_ptr[dst_offset + 3] = 255;
                }
            }
        }
    }
    else {
        lpdds->Unlock(NULL);
        return 0;
    }
    
    lpdds->Unlock(NULL);
    return 1;

}

/* creates an offscreen plain surface */
LPDIRECTDRAWSURFACE7 DDraw_Create_Surface(int width, int height, int mem_flags, int color_key = 0)
{
   DDSURFACEDESC2 ddsd;         // working description
   LPDIRECTDRAWSURFACE7 lpdds;  // temporary surface

   // set to access caps, width, and height
   memset(&ddsd,0,sizeof(ddsd));
   ddsd.dwSize  = sizeof(ddsd);
   ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

   // set dimensions of the new bitmap surface
   ddsd.dwWidth  =  width;
   ddsd.dwHeight =  height;

   // set surface to offscreen plain
   ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

   // create the surface
   if (FAILED(lpdd->CreateSurface(&ddsd,&lpdds,NULL)))
   	return(NULL);

   // test if user wants a color key
   if (color_key >= 0) {
      // set color key to color 0
      DDCOLORKEY color_key; // used to set color key
      color_key.dwColorSpaceLowValue  = 0;
      color_key.dwColorSpaceHighValue = 0;

      // now set the color key for source blitting
      lpdds->SetColorKey(DDCKEY_SRCBLT, &color_key);
   }
   
   // return surface
   return(lpdds);

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

/*
  draws the sent text on the sent surface 
  using color index as the color in the palette
*/
int Draw_Text_GDI(char *text, int x,int y,COLORREF color, LPDIRECTDRAWSURFACE7 lpdds) {
  HDC xdc; // the working dc

   // get the dc from surface
   if (FAILED(lpdds->GetDC(&xdc)))
      return(0);

   // set the colors for the text up
   SetTextColor(xdc,color);

   // set background mode to transparent so black isn't copied
   SetBkMode(xdc, TRANSPARENT);

   // draw the text a
   TextOutA(xdc,x,y,text,strlen(text));

   // release the dc
   lpdds->ReleaseDC(xdc);

   // return success
   return(1);

}

/* main game loop */
int Game_Main(void* parms = NULL, int num_parms = 0) {
   
  // lookup for proper walking sequence
   static int animation_seq[4] = {0,1,0,2};

   // make sure this isn't executed again
   if (window_closed) return 0;

    // for now test if the user is hitting ESC and send WM_CLOSE
    if (KEYDOWN(VK_ESCAPE)) {
      PostMessage(main_window_handle, WM_CLOSE, 0, 0);
      window_closed = 1;
    }

   // copy background to back buffer
   DDraw_Draw_Surface(lpddsbackground,0,0, SCREEN_WIDTH,SCREEN_HEIGHT, lpddsback,0);    

   // move objects around

   for (int index=0; index < 3; index++) {
      // move each object to the right at its given velocity
      aliens[index].x++; // =aliens[index].velocity;

      // test if off screen edge, and wrap around
      if (aliens[index].x > SCREEN_WIDTH)
         aliens[index].x = - 80;

      // animate bot
      if (++aliens[index].counter >= (8 - aliens[index].velocity)) {
         // reset counter
         aliens[index].counter = 0;

         // advance to next frame
         if (++aliens[index].current_frame > 3)
            aliens[index].current_frame = 0;
   
      }

   }

   // draw all the bots
   for (int index=0; index < 3; index++) {
    // draw objects
    DDraw_Draw_Surface(aliens[index].frames[animation_seq[aliens[index].current_frame]], 
                       aliens[index].x, aliens[index].y,
                       72,80,
                       lpddsback);
   }

   // flip pages
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

   // request a complex, flippable
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

    // create the primary surface
    if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL))) {
      return 0;
    }

    // now query for attached surface from the primary surface

   // this line is needed by the call
   ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

   // get the attached back buffer surface
   if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback)))
   return(0);

   // set clipper up on back buffer since that's where well clip
   RECT screen_rect= {0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1};
   lpddclipper = DDraw_Attach_Clipper(lpddsback,1,&screen_rect);

   // load the 24-bit image
   if (!Load_Bitmap_File(&bitmap,(char*)"../resource/alley24.bmp"))
      return(0);

   // clean the surfaces
   DDraw_Fill_Surface(lpddsprimary,0);
   DDraw_Fill_Surface(lpddsback,0);

   // create the buffer to hold the background
   lpddsbackground = DDraw_Create_Surface(640,480,0,-1);

   // copy the background bitmap image to the background surface 

   // lock the surface
   lpddsbackground->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

   // get video pointer to primary surfce
   UCHAR *image_buffer = (UCHAR *)ddsd.lpSurface;       

   // test if memory is linear
   if (ddsd.lPitch == SCREEN_WIDTH) {
      // copy memory from double buffer to primary buffer
      memcpy((void *)image_buffer, (void *)bitmap.buffer, SCREEN_WIDTH*SCREEN_HEIGHT*4);
   }
   else { // non-linear

      // make copy of source and destination addresses
      UCHAR *dest_ptr = image_buffer;
      UCHAR *src_ptr  = bitmap.buffer;

      // memory is non-linear, copy line by line
      for (int y=0; y < SCREEN_HEIGHT; y++) {
         // copy line
         for (int x = 0; x < SCREEN_WIDTH; x++) {

            UCHAR blue = src_ptr[y*SCREEN_WIDTH*3+x*3 + 0];
            UCHAR green = src_ptr[y*SCREEN_WIDTH*3+x*3 + 1];
            UCHAR red = src_ptr[y*SCREEN_WIDTH*3+x*3 + 2];
            int pixel_offset = y * ddsd.lPitch + x * 4;

            dest_ptr[pixel_offset] = blue;
            dest_ptr[pixel_offset+1] = green;
            dest_ptr[pixel_offset+2] = red;
            dest_ptr[pixel_offset+3] = 0; // alpha
         }
      }

   }
   // now unlock the primary surface
   if (FAILED(lpddsbackground->Unlock(NULL)))
      return(0);

   // unload the bitmap file, we no longer need it
   Unload_Bitmap_File(&bitmap);

   // seed random number generator
   srand(GetTickCount());

   // initialize all the aliens

   // alien on level 1 of complex
   aliens[0].x              = rand()%SCREEN_WIDTH;
   aliens[0].y              = 116 - 72;                  
   aliens[0].velocity       = 2+rand()%4;
   aliens[0].current_frame  = 0;             
   aliens[0].counter        = 0;       

   // alien on level 2 of complex
   aliens[1].x              = rand()%SCREEN_WIDTH;
   aliens[1].y              = 246 - 72;                  
   aliens[1].velocity       = 2+rand()%4;
   aliens[1].current_frame  = 0;             
   aliens[1].counter        = 0;  

   // alien on level 3 of complex
   aliens[2].x              = rand()%SCREEN_WIDTH;
   aliens[2].y              = 382 - 72;                  
   aliens[2].velocity       = 2+rand()%4;
   aliens[2].current_frame  = 0;             
   aliens[2].counter        = 0;  

   // now load the bitmap containing the alien imagery
   // then scan the images out into the surfaces of alien[0]
   // and copy then into the other two, be careful of reference counts!

   // load the 24-bit image
   if (!Load_Bitmap_File(&bitmap,(char*)"../resource/dedsp0.bmp"))
      return(0);

   // create each surface and load bits
   for (int index = 0; index < 3; index++) {
      // create surface to hold image
      aliens[0].frames[index] = DDraw_Create_Surface(72,80,0);

      // now load bits...
      Scan_Image_Bitmap(&bitmap,                 // bitmap file to scan image data from
                        aliens[0].frames[index], // surface to hold data
                        index, 0);               // cell to scan image from    

   }
   // unload the bitmap file, we no longer need it
   Unload_Bitmap_File(&bitmap);

   // now for the tricky part. There is no need to create more surfaces with the same
   // data, so I'm going to copy the surface pointers member for member to each alien
   // however, be careful, since the reference counts do NOT go up, you still only need
   // to release() each surface once!

   for (int index = 0; index < 3; index++)
      aliens[1].frames[index] = aliens[2].frames[index] = aliens[0].frames[index];

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
                              L"Demo7_13",  // title
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