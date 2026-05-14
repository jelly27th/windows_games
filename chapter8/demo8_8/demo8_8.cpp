/* 
  32-bit quad drawing demo
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

const double PI = 3.1415926535;

// fixed point mathematics constants
#define FIXP16_SHIFT     16
#define FIXP16_MAG       65536
#define FIXP16_DP_MASK   0x0000ffff
#define FIXP16_WP_MASK   0xffff0000
#define FIXP16_ROUND_UP  0x00008000

/* basic unsigned types */
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;

// a 2D vertex
typedef struct VERTEX2DI_TYP {
   int x,y; // the vertex
} VERTEX2DI, *VERTEX2DI_PTR;

// a 2D vertex
typedef struct VERTEX2DF_TYP {
	float x,y; // the vertex
} VERTEX2DF, *VERTEX2DF_PTR;

// a 2D polygon
typedef struct POLYGON2D_TYP {
        int state;        // state of polygon
        int num_verts;    // number of vertices
        int x0,y0;        // position of center of polygon  
        int xv,yv;        // initial velocity
        DWORD color;      // could be index or PALETTENTRY
        VERTEX2DF *vlist; // pointer to vertex list
 } POLYGON2D, *POLYGON2D_PTR;

/* prototypes */

int DDraw_Fill_Surface(LPDIRECTDRAWSURFACE7 lpdds,int color);

void Draw_Top_Tri(int x1,int y1,int x2,int y2, int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

void Draw_Bottom_Tri(int x1,int y1, int x2,int y2, int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

void Draw_Top_TriFP(int x1,int y1,int x2,int y2, int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

void Draw_Bottom_TriFP(int x1,int y1, int x2,int y2, int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

void Draw_Triangle_2D(int x1,int y1,int x2,int y2,int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

void Draw_TriangleFP_2D(int x1,int y1,int x2,int y2,int x3,int y3,int color,UCHAR *dest_buffer, int mempitch);

inline void Draw_QuadFP_2D(int x0,int y0,int x1,int y1,
                           int x2,int y2,int x3, int y3,
                           int color,UCHAR *dest_buffer, int mempitch);

int Translate_Polygon2D(POLYGON2D_PTR poly, int dx, int dy);

int Rotate_Polygon2D(POLYGON2D_PTR poly, int theta);

int Scale_Polygon2D(POLYGON2D_PTR poly, float sx, float sy);

int Draw_Text_GDI(char *text, int x,int y,
                  COLORREF color, LPDIRECTDRAWSURFACE7 lpdds);

/* macro */
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// this builds a 32 bit color value in A.8.8.8 format (8-bit alpha mode)
#define _RGB32BIT(a,r,g,b) ((b) + (g << 8) + (r << 16) + (a << 24))

/* initialize a direct draw structure */
#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct, 0, sizeof(ddstruct)); ddstruct.dwSize = sizeof(ddstruct); }

// some math macros
#define DEG_TO_RAD(ang) ((ang)*PI/180)
#define RAD_TO_DEG(rads) ((rads)*180/PI)

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
int min_clip_x = 0,      // clipping rectangle 
    max_clip_x = SCREEN_WIDTH - 1,
    min_clip_y = 0,
    max_clip_y = SCREEN_HEIGHT - 1;

char buffer[80];                             // general printing buffer

// storage for our lookup tables
float cos_look[360];
float sin_look[360];

POLYGON2D object; // the ship

/* function */

/* draws a 2D quadrilateral */
inline void Draw_QuadFP_2D(int x0,int y0,
                    int x1,int y1,
                    int x2,int y2,
                    int x3, int y3,
                    int color,
                    UCHAR *dest_buffer, int mempitch)
{
   // simply call the triangle function 2x, let it do all the work
   Draw_TriangleFP_2D(x0,y0,x1,y1,x3,y3,color,dest_buffer,mempitch);
   Draw_TriangleFP_2D(x1,y1,x2,y2,x3,y3,color,dest_buffer,mempitch);

}

/* draws a triangle that has a flat top using fixed point math */
void Draw_Top_TriFP(int x1,int y1,
                    int x2,int y2, 
                    int x3,int y3,
                    int color, 
                    UCHAR *dest_buffer, int mempitch)
{

   int dx_right,    // the dx/dy ratio of the right edge of line
      dx_left,     // the dx/dy ratio of the left edge of line
      xs,xe,       // the starting and ending points of the edges
      height;      // the height of the triangle

   int temp_x,        // used during sorting as temps
      temp_y,
      right,         // used by clipping
      left;

   UCHAR  *dest_addr;

   // test for degenerate
   if (y1==y3 || y2==y3)
      return;

   // test order of x1 and x2
   if (x2 < x1) {
      temp_x = x2;
      x2     = x1;
      x1     = temp_x;
   }

   // compute delta's
   height = y3-y1;

   dx_left  = ((x3-x1)<<FIXP16_SHIFT)/height;
   dx_right = ((x3-x2)<<FIXP16_SHIFT)/height;

   // set starting points
   xs = (x1<<FIXP16_SHIFT);
   xe = (x2<<FIXP16_SHIFT);

   // perform y clipping
   if (y1<min_clip_y) {
      // compute new xs and ys
      xs = xs+dx_left*(-y1+min_clip_y);
      xe = xe+dx_right*(-y1+min_clip_y);

      // reset y1
      y1=min_clip_y;
   }

   if (y3>max_clip_y)
      y3=max_clip_y;

   // compute starting address in video memory
   dest_addr = dest_buffer+y1*mempitch;

   // test if x clipping is needed
   if (x1>=min_clip_x && x1<=max_clip_x &&
      x2>=min_clip_x && x2<=max_clip_x &&
      x3>=min_clip_x && x3<=max_clip_x) {
      
      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {
         int start_x = (xs + FIXP16_ROUND_UP) >> FIXP16_SHIFT;
         int end_x   = (xe + FIXP16_ROUND_UP) >> FIXP16_SHIFT;
         int width = end_x - start_x + 1;
      
         if (width > 0) {

            UCHAR *line_ptr = dest_addr + start_x * 4;
            UCHAR *color_bytes = (UCHAR *)&color;
            
            for (int i = 0; i < width; i++) {
                  line_ptr[0] = color_bytes[0];  // blue
                  line_ptr[1] = color_bytes[1];  // green
                  line_ptr[2] = color_bytes[2];  // red
                  line_ptr[3] = color_bytes[3];  // Alpha
                  line_ptr += 4;
            }
         }

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;
      }

   } else {
      // clip x axis with slower version

      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {
         // do x clip
         left  = ((xs+FIXP16_ROUND_UP)>>16);
         right = ((xe+FIXP16_ROUND_UP)>>16);

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         // clip line
         if (left < min_clip_x) {
            left = min_clip_x;

            if (right < min_clip_x)
               continue;
         }

         if (right > max_clip_x) {
            right = max_clip_x;

            if (left > max_clip_x)
               continue;
         }

         int pixel_count = right - left + 1;
         if (pixel_count > 0) {
            UCHAR *pixel_ptr = dest_addr + left * 4; 
            
            for (int i = 0; i < pixel_count; i++) {
                  pixel_ptr[0] = (color >> 0) & 0xFF;  // blue
                  pixel_ptr[1] = (color >> 8) & 0xFF;  // green
                  pixel_ptr[2] = (color >> 16) & 0xFF;  // red
                  pixel_ptr[3] = (color >> 24) & 0xFF;  // Alpha
                  pixel_ptr += 4;
            }
         }
      }
   }

}

/* draws a triangle that has a flat bottom using fixed point math*/
void Draw_Bottom_TriFP(int x1,int y1, 
                       int x2,int y2, 
                       int x3,int y3,
                       int color,
                       UCHAR *dest_buffer, int mempitch)
{

   int dx_right,    // the dx/dy ratio of the right edge of line
      dx_left,     // the dx/dy ratio of the left edge of line
      xs,xe,       // the starting and ending points of the edges
      height;      // the height of the triangle

   int temp_x,        // used during sorting as temps
      temp_y,
      right,         // used by clipping
      left;

   UCHAR  *dest_addr;

   if (y1==y2 || y1==y3)
      return;

   // test order of x1 and x2
   if (x3 < x2) {
      temp_x = x2;
      x2     = x3;
      x3     = temp_x;

   }

   // compute delta's
   height = y3-y1;

   dx_left  = ((x2-x1)<<FIXP16_SHIFT)/height;
   dx_right = ((x3-x1)<<FIXP16_SHIFT)/height;

   // set starting points
   xs = (x1<<FIXP16_SHIFT);
   xe = (x1<<FIXP16_SHIFT); 

   // perform y clipping
   if (y1<min_clip_y) {
      // compute new xs and ys
      xs = xs+dx_left*(-y1+min_clip_y);
      xe = xe+dx_right*(-y1+min_clip_y);

      // reset y1
      y1=min_clip_y;

   }

   if (y3>max_clip_y)
      y3=max_clip_y;

   // compute starting address in video memory
   dest_addr = dest_buffer+y1*mempitch;

   // test if x clipping is needed
   if (x1>=min_clip_x && x1<=max_clip_x &&
      x2>=min_clip_x && x2<=max_clip_x &&
      x3>=min_clip_x && x3<=max_clip_x) {
      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {

         int start_x = (xs + FIXP16_ROUND_UP) >> FIXP16_SHIFT;
         int end_x   = (xe + FIXP16_ROUND_UP) >> FIXP16_SHIFT;
         int width = end_x - start_x + 1;
      
         if (width > 0) {
            UCHAR *line_ptr = dest_addr + start_x * 4;
            UCHAR *color_bytes = (UCHAR *)&color;
            
            for (int i = 0; i < width; i++) {
                  line_ptr[0] = color_bytes[0];  // blue
                  line_ptr[1] = color_bytes[1];  // green
                  line_ptr[2] = color_bytes[2];  // red
                  line_ptr[3] = color_bytes[3];  // Alpha
                  line_ptr += 4;
            }
         }
         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         }

      } else {
      // clip x axis with slower version

      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {
         // do x clip
         left  = ((xs+FIXP16_ROUND_UP)>>FIXP16_SHIFT);
         right = ((xe+FIXP16_ROUND_UP)>>FIXP16_SHIFT);

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         // clip line
         if (left < min_clip_x) {
            left = min_clip_x;

            if (right < min_clip_x)
               continue;
         }

         if (right > max_clip_x) {
            right = max_clip_x;

            if (left > max_clip_x)
               continue;
         }

         int pixel_count = right - left + 1;
         if (pixel_count > 0) {
            UCHAR *pixel_ptr = dest_addr + left * 4; 
            
            for (int i = 0; i < pixel_count; i++) {
               pixel_ptr[0] = (color >> 0) & 0xFF;  // blue
               pixel_ptr[1] = (color >> 8) & 0xFF;  // green
               pixel_ptr[2] = (color >> 16) & 0xFF;  // red
               pixel_ptr[3] = (color >> 24) & 0xFF;  // Alpha
               pixel_ptr += 4;
            }
         }
      }
   }

}

/* draws a triangle that has a flat top */
void Draw_Top_Tri(int x1,int y1, 
                  int x2,int y2, 
                  int x3,int y3,
                  int color, 
                  UCHAR *dest_buffer, int mempitch)
{

   float dx_right,    // the dx/dy ratio of the right edge of line
         dx_left,     // the dx/dy ratio of the left edge of line
         xs,xe,       // the starting and ending points of the edges
         height;      // the height of the triangle

   int temp_x,        // used during sorting as temps
      temp_y,
      right,         // used by clipping
      left;

   // destination address of next scanline
   UCHAR  *dest_addr = NULL;

   // test order of x1 and x2
   if (x2 < x1) {
      temp_x = x2;
      x2     = x1;
      x1     = temp_x;
   }

   // compute delta's
   height = y3-y1;

   dx_left  = (x3-x1)/height;
   dx_right = (x3-x2)/height;

   // set starting points
   xs = (float)x1;
   xe = (float)x2+(float)0.5;

   // perform y clipping
   if (y1 < min_clip_y) {
      // compute new xs and ys
      xs = xs+dx_left*(float)(-y1+min_clip_y);
      xe = xe+dx_right*(float)(-y1+min_clip_y);

      // reset y1
      y1=min_clip_y;
   }

   if (y3>max_clip_y)
      y3=max_clip_y;

   // compute starting address in video memory
   dest_addr = dest_buffer+y1*mempitch;

   // test if x clipping is needed
   if (x1>=min_clip_x && x1<=max_clip_x &&
      x2>=min_clip_x && x2<=max_clip_x &&
      x3>=min_clip_x && x3<=max_clip_x) {
      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {

         int start_x = xs;
         int end_x   = xe;
         int width = end_x - start_x + 1;
      
         if (width > 0) {

            UCHAR *line_ptr = dest_addr + start_x * 4;
            UCHAR *color_bytes = (UCHAR *)&color;
            
            for (int i = 0; i < width; i++) {
                  line_ptr[0] = color_bytes[0];  // blue
                  line_ptr[1] = color_bytes[1];  // green
                  line_ptr[2] = color_bytes[2];  // red
                  line_ptr[3] = color_bytes[3];  // Alpha
                  line_ptr += 4;
            }
         }

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

      }

   } else {
      // clip x axis with slower version

      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {
         // do x clip
         left  = (int)xs;
         right = (int)xe;

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         // clip line
         if (left < min_clip_x) {
            left = min_clip_x;

            if (right < min_clip_x)
               continue;
         }

         if (right > max_clip_x) {
            right = max_clip_x;

            if (left > max_clip_x)
               continue;
         }

         int pixel_count = right - left + 1;
         if (pixel_count > 0) {
            UCHAR *pixel_ptr = dest_addr + left * 4; 
            
            for (int i = 0; i < pixel_count; i++) {
                  pixel_ptr[0] = (color >> 0) & 0xFF;  // blue
                  pixel_ptr[1] = (color >> 8) & 0xFF;  // green
                  pixel_ptr[2] = (color >> 16) & 0xFF;  // red
                  pixel_ptr[3] = (color >> 24) & 0xFF;  // Alpha
                  pixel_ptr += 4;
            }
         }
      }

   }

}

/* draws a triangle that has a flat bottom */
void Draw_Bottom_Tri(int x1,int y1, 
                     int x2,int y2, 
                     int x3,int y3,
                     int color,
                     UCHAR *dest_buffer, int mempitch)
{

   float dx_right,    // the dx/dy ratio of the right edge of line
         dx_left,     // the dx/dy ratio of the left edge of line
         xs,xe,       // the starting and ending points of the edges
         height;      // the height of the triangle

   int temp_x,        // used during sorting as temps
      temp_y,
      right,         // used by clipping
      left;

   // destination address of next scanline
   UCHAR  *dest_addr;

   // test order of x1 and x2
   if (x3 < x2) {
      temp_x = x2;
      x2     = x3;
      x3     = temp_x;
   }

   // compute delta's
   height = y3-y1;

   dx_left  = (x2-x1)/height;
   dx_right = (x3-x1)/height;

   // set starting points
   xs = (float)x1;
   xe = (float)x1; // +(float)0.5;

   // perform y clipping
   if (y1<min_clip_y) {
      // compute new xs and ys
      xs = xs+dx_left*(float)(-y1+min_clip_y);
      xe = xe+dx_right*(float)(-y1+min_clip_y);

      // reset y1
      y1=min_clip_y;

   }

   if (y3>max_clip_y)
      y3=max_clip_y;

   // compute starting address in video memory
   dest_addr = dest_buffer+y1*mempitch;

   // test if x clipping is needed
   if (x1>=min_clip_x && x1<=max_clip_x &&
      x2>=min_clip_x && x2<=max_clip_x &&
      x3>=min_clip_x && x3<=max_clip_x) {
      // draw the triangle
      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {

            int start_x = xs;
            int end_x   = xe;
            int width = end_x - start_x + 1;
         
            if (width > 0) {

               UCHAR *line_ptr = dest_addr + start_x * 4;
               UCHAR *color_bytes = (UCHAR *)&color;
               
               for (int i = 0; i < width; i++) {
                     line_ptr[0] = color_bytes[0];  // blue
                     line_ptr[1] = color_bytes[1];  // green
                     line_ptr[2] = color_bytes[2];  // red
                     line_ptr[3] = color_bytes[3];  // Alpha
                     line_ptr += 4;
               }
            }

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         }

   } else {
      // clip x axis with slower version

      // draw the triangle

      for (temp_y=y1; temp_y<=y3; temp_y++,dest_addr+=mempitch) {
         // do x clip
         left  = (int)xs;
         right = (int)xe;

         // adjust starting point and ending point
         xs+=dx_left;
         xe+=dx_right;

         // clip line
         if (left < min_clip_x) {
            left = min_clip_x;

            if (right < min_clip_x)
               continue;
         }

         if (right > max_clip_x) {
            right = max_clip_x;

            if (left > max_clip_x)
               continue;
         }

         int pixel_count = right - left + 1;
         if (pixel_count > 0) {
            UCHAR *pixel_ptr = dest_addr + left * 4; 
            
            for (int i = 0; i < pixel_count; i++) {
                  pixel_ptr[0] = (color >> 0) & 0xFF;  // blue
                  pixel_ptr[1] = (color >> 8) & 0xFF;  // green
                  pixel_ptr[2] = (color >> 16) & 0xFF;  // red
                  pixel_ptr[3] = (color >> 24) & 0xFF;  // Alpha
                  pixel_ptr += 4;
            }
         }
      }

   }
}

/* draws a triangle on the destination buffer using fixed point
   it decomposes all triangles into a pair of flat top, flat bottom*/
void Draw_TriangleFP_2D(int x1,int y1,
                        int x2,int y2,
                        int x3,int y3,
                        int color,
    	   			    UCHAR *dest_buffer, int mempitch)
{

   int temp_x, // used for sorting
      temp_y,
      new_x;

   // test for h lines and v lines
   if ((x1==x2 && x2==x3)  ||  (y1==y2 && y2==y3))
      return;

   // sort p1,p2,p3 in ascending y order
   if (y2<y1) {
      temp_x = x2;
      temp_y = y2;
      x2     = x1;
      y2     = y1;
      x1     = temp_x;
      y1     = temp_y;
   }

   // now we know that p1 and p2 are in order
   if (y3<y1) {
      temp_x = x3;
      temp_y = y3;
      x3     = x1;
      y3     = y1;
      x1     = temp_x;
      y1     = temp_y;
   } 

   // finally test y3 against y2
   if (y3<y2) {
      temp_x = x3;
      temp_y = y3;
      x3     = x2;
      y3     = y2;
      x2     = temp_x;
      y2     = temp_y;
   }

   // do trivial rejection tests for clipping
   if ( y3<min_clip_y || y1>max_clip_y ||
      (x1<min_clip_x && x2<min_clip_x && x3<min_clip_x) ||
      (x1>max_clip_x && x2>max_clip_x && x3>max_clip_x) )
      return;

   // test if top of triangle is flat
   if (y1==y2) {
      Draw_Top_TriFP(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
   } else if (y2==y3) {
      /* bottom is flat */
      Draw_Bottom_TriFP(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
   } else {
      // general triangle that's needs to be broken up along long edge
      new_x = x1 + (int)(0.5+(float)(y2-y1)*(float)(x3-x1)/(float)(y3-y1));

      // draw each sub-triangle
      Draw_Bottom_TriFP(x1,y1,new_x,y2,x2,y2,color, dest_buffer, mempitch);
      Draw_Top_TriFP(x2,y2,new_x,y2,x3,y3,color, dest_buffer, mempitch);
   }
}

/* draws a triangle on the destination buffer
   it decomposes all triangles into a pair of flat top, flat bottom */
void Draw_Triangle_2D(int x1,int y1,
                      int x2,int y2,
                      int x3,int y3,
                      int color,
					  UCHAR *dest_buffer, int mempitch)
{

   int temp_x, // used for sorting
      temp_y,
      new_x;

   // test for h lines and v lines
   if ((x1==x2 && x2==x3)  ||  (y1==y2 && y2==y3))
      return;

   // sort p1,p2,p3 in ascending y order
   if (y2<y1) {
      temp_x = x2;
      temp_y = y2;
      x2     = x1;
      y2     = y1;
      x1     = temp_x;
      y1     = temp_y;
   }

   // now we know that p1 and p2 are in order
   if (y3<y1) {
      temp_x = x3;
      temp_y = y3;
      x3     = x1;
      y3     = y1;
      x1     = temp_x;
      y1     = temp_y;
   }

   // finally test y3 against y2
   if (y3<y2) {
      temp_x = x3;
      temp_y = y3;
      x3     = x2;
      y3     = y2;
      x2     = temp_x;
      y2     = temp_y;
   }

   // do trivial rejection tests for clipping
   if ( y3<min_clip_y || y1>max_clip_y ||
      (x1<min_clip_x && x2<min_clip_x && x3<min_clip_x) ||
      (x1>max_clip_x && x2>max_clip_x && x3>max_clip_x) )
      return;

   // test if top of triangle is flat
   if (y1==y2) {
      Draw_Top_Tri(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
   } else if (y2==y3) {
      /* bottom is flat*/
      Draw_Bottom_Tri(x1,y1,x2,y2,x3,y3,color, dest_buffer, mempitch);
   } else {
      // general triangle that's needs to be broken up along long edge
      new_x = x1 + (int)(0.5+(float)(y2-y1)*(float)(x3-x1)/(float)(y3-y1));

      // draw each sub-triangle
      Draw_Bottom_Tri(x1,y1,new_x,y2,x2,y2,color, dest_buffer, mempitch);
      Draw_Top_Tri(x2,y2,new_x,y2,x3,y3,color, dest_buffer, mempitch);
   }
}

/* translates the center of a polygon */
int Translate_Polygon2D(POLYGON2D_PTR poly, int dx, int dy) {

   // test for valid pointer
   if (!poly)
      return 0;

   // translate
   poly->x0+=dx;
   poly->y0+=dy;

   return 1;

} 

/* rotates the local coordinates of the polygon */
int Rotate_Polygon2D(POLYGON2D_PTR poly, int theta) {

   // test for valid pointer
   if (!poly) return(0);

	// test for negative rotation angle
	if (theta < 0)
	   theta+=360;
   // loop and rotate each point, very crude, no lookup!!!
   for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {

      // perform rotation
      float xr = (float)poly->vlist[curr_vert].x*cos_look[theta] - 
                     (float)poly->vlist[curr_vert].y*sin_look[theta];

      float yr = (float)poly->vlist[curr_vert].x*sin_look[theta] + 
                     (float)poly->vlist[curr_vert].y*cos_look[theta];

      // store result back
      poly->vlist[curr_vert].x = xr;
      poly->vlist[curr_vert].y = yr;

   }
   return 1;
}

/* scalesthe local coordinates of the polygon */
int Scale_Polygon2D(POLYGON2D_PTR poly, float sx, float sy) {

   // test for valid pointer
   if (!poly) return(0);

   // loop and scale each point
   for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {
      // scale and store result back
      poly->vlist[curr_vert].x *= sx;
      poly->vlist[curr_vert].y *= sy;

   }
   return 1;
}

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

   // clear out the back buffer
   DDraw_Fill_Surface(lpddsback, 0);

   DDRAW_INIT_STRUCT(ddsd);

   // lock the primary surface
   lpddsback->Lock(NULL,&ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,NULL);

   DWORD color = _RGB32BIT(0,rand()%256,rand()%256,rand()%256);

   // generate vertices, note these can be anywhere in space, but they must be in CW order
   int x0     = rand()%SCREEN_WIDTH;
   int y0     = rand()%SCREEN_HEIGHT;
   int width  = rand()%SCREEN_WIDTH;
   int height = rand()%SCREEN_HEIGHT;

   // draw the triangle
   Draw_QuadFP_2D(x0,y0,
                  x0+width,y0,
                  x0+width, y0+height, 
                  x0, y0+height,
                  color,(UCHAR *)ddsd.lpSurface, ddsd.lPitch);

   // unlock primary buffer
   if (FAILED(lpddsback->Unlock(NULL)))
      return(0);

   // draw the text
   color = _RGB32BIT(0,255,255,255);
   Draw_Text_GDI((char*)"Press <ESC> to exit.", 8,8,color, lpddsback);

   // perform the flip
   while (FAILED(lpddsprimary->Flip(NULL, DDFLIP_WAIT)));
   
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

    // seed random number generator
    srand(GetTickCount());

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
   ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

   // set the backbuffer count field to 1, use 2 for triple buffering
   ddsd.dwBackBufferCount = 1;

   // request a complex, flippable
   ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

   // create the primary surface
   if (FAILED(lpdd->CreateSurface(&ddsd, &lpddsprimary, NULL)))
      return(0);

   // now query for attached surface from the primary surface

   // this line is needed by the call
   ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

   // get the attached back buffer surface
   if (FAILED(lpddsprimary->GetAttachedSurface(&ddsd.ddsCaps, &lpddsback)))
      return(0);

   // clean the surface
   DDraw_Fill_Surface(lpddsprimary,0);
   DDraw_Fill_Surface(lpddsback, 0 );

   // define points of object (must be convex)
   VERTEX2DF object_vertices[4] = {-100,-100, 100,-100, 100,100, -100, 100};

   // initialize polygon object
   object.state       = 1;   // turn it on
   object.num_verts   = 4;  
   object.x0          = SCREEN_WIDTH/2; // position it
   object.y0          = SCREEN_HEIGHT/2;
   object.xv          = 0;
   object.yv          = 0;
   object.color       = 1; // animated green
   object.vlist       = new VERTEX2DF [object.num_verts];

   for (int index = 0; index < object.num_verts; index++)
      object.vlist[index] = object_vertices[index];

   // create sin/cos lookup table

   // generate the tables
   for (int ang = 0; ang < 360; ang++) {
      // convert ang to radians
      float theta = (float)ang*PI/(float)180;

      // insert next entry into table
      cos_look[ang] = cos(theta);
      sin_look[ang] = sin(theta);
   }

   // hide the cursor
   ShowCursor(0);

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

	// now the back buffer surface
	if (lpddsback) {
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
                              L"Demo8_8",  // title
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