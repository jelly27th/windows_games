/* 
  32-bit general polygon fill demo
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

void Draw_Filled_Polygon2D(POLYGON2D_PTR poly, UCHAR *vbuffer, int mempitch);

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

/* draws a general n sided polygon */
void Draw_Filled_Polygon2D(POLYGON2D_PTR poly, UCHAR *vbuffer, int mempitch)
{ 

   int ydiff1, ydiff2,         // difference between starting x and ending x
      xdiff1, xdiff2,         // difference between starting y and ending y
      start,                  // starting offset of line between edges
      length,                 // distance from edge 1 to edge 2
      errorterm1, errorterm2, // error terms for edges 1 & 2
      offset1, offset2,       // offset of current pixel in edges 1 & 2
      count1, count2,         // increment count for edges 1 & 2
      xunit1, xunit2;         // unit to advance x offset for edges 1 & 2

   // initialize count of number of edges drawn:
   int edgecount = poly->num_verts-1;

   // determine which vertex is at top of polygon:

   int firstvert=0;         // start by assuming vertex 0 is at top

   int min_y=poly->vlist[0].y; // find y coordinate of vertex 0

   for (int index=1; index < poly->num_verts; index++) {  
      // Search thru vertices
      if ((poly->vlist[index].y) < min_y) {  
         // is another vertex higher?
         firstvert=index;                   
         min_y=poly->vlist[index].y;
      }
   }

   // finding starting and ending vertices of first two edges:
   int startvert1=firstvert;      // get starting vertex of edge 1
   int startvert2=firstvert;      // get starting vertex of edge 2
   int xstart1=poly->vlist[startvert1].x+poly->x0;
   int ystart1=poly->vlist[startvert1].y+poly->y0;
   int xstart2=poly->vlist[startvert2].x+poly->x0;
   int ystart2=poly->vlist[startvert2].y+poly->y0;
   int endvert1=startvert1-1;           // get ending vertex of edge 1

   if (endvert1 < 0) 
      endvert1=poly->num_verts-1;    // check for wrap

   int xend1=poly->vlist[endvert1].x+poly->x0;      // get x & y coordinates
   int yend1=poly->vlist[endvert1].y+poly->y0;      // of ending vertices
   int endvert2=startvert2+1;           // get ending vertex of edge 2

   if (endvert2==(poly->num_verts)) 
      endvert2=0;  // Check for wrap

   int xend2=poly->vlist[endvert2].x+poly->x0;      // get x & y coordinates
   int yend2=poly->vlist[endvert2].y+poly->y0;      // of ending vertices

   // 32-bit color components
   UCHAR blue  = (poly->color >> 0) & 0xFF;
   UCHAR green = (poly->color >> 8) & 0xFF;
   UCHAR red   = (poly->color >> 16) & 0xFF;
   UCHAR alpha = (poly->color >> 24) & 0xFF;
   // draw the polygon:

   while (edgecount>0) {    
      // continue drawing until all edges drawn
      offset1=mempitch*ystart1+xstart1*4;  // offset of edge 1
      offset2=mempitch*ystart2+xstart2*4;  // offset of edge 2
      
      // initialize error terms
      // for edges 1 & 2
      errorterm1=0;        
      errorterm2=0;           

      // get absolute value of
      if ((ydiff1=yend1-ystart1) < 0) 
         ydiff1=-ydiff1;

         // x & y lengths of edges
      if ((ydiff2=yend2-ystart2) < 0) 
            ydiff2=-ydiff2; 

      if ((xdiff1=xend1-xstart1) < 0) {               
            // get value of length
         xunit1=-4;                    // calculate X increment
         xdiff1=-xdiff1;
      } else {
         xunit1=4;
      }

      if ((xdiff2=xend2-xstart2) < 0) {
         // Get value of length
         xunit2=-4;                   // calculate X increment
         xdiff2=-xdiff2;
      } else {
         xunit2=4;
      }

      // choose which of four routines to use
      if (xdiff1 > ydiff1)  {    
            // if x length of edge 1 is greater than y length
         if (xdiff2 > ydiff2) {  
            // if X length of edge 2 is greater than y length

            // increment edge 1 on X and edge 2 on X:
            count1=xdiff1;    // count for x increment on edge 1
            count2=xdiff2;    // count for x increment on edge 2

            while (count1 && count2) {  
                     // continue drawing until one edge is done
               // calculate edge 1:
               while ((errorterm1 < xdiff1) && (count1 > 0)) { 
                     // finished w/edge 1?
                     if (count1--) {     
                        // count down on edge 1
                        offset1+=xunit1;  // increment pixel offset
                        xstart1 += (xunit1 / 4);  // update x coordiation
                     }

                     errorterm1+=ydiff1; // increment error term

                     if (errorterm1 < xdiff1) {  
                        // if not more than XDIFF
                     vbuffer[offset1] = blue;       // blue
                     vbuffer[offset1 + 1] = green;  // green
                     vbuffer[offset1 + 2] = red;    // red
                     vbuffer[offset1 + 3] = alpha;  // alpha
                     }
                  }
                  
               errorterm1-=xdiff1; // if time to increment X, restore error term

               // calculate edge 2:

               while ((errorterm2 < xdiff2) && (count2 > 0)) {  
                     // finished w/edge 2?
                     if (count2--) {     
                        // count down on edge 2
                        offset2+=xunit2;  // increment pixel offset
                        xstart2 += (xunit2 / 4);  // update x coordiation
                  }

                     errorterm2+=ydiff2; // increment error term

                     if (errorterm2 < xdiff2) {  
                        // if not more than XDIFF
	                     vbuffer[offset2] = blue;       // blue
	                     vbuffer[offset2 + 1] = green;  // green
	                     vbuffer[offset2 + 2] = red;    // red
	                     vbuffer[offset2 + 3] = alpha;  // alpha
                     }

                  }

                  errorterm2-=xdiff2; // if time to increment X, restore error term

                  // draw line from edge 1 to edge 2:

                  length=(offset2 - offset1) / 4; // determine length of horizontal line

                  if (length < 0) { 
                     // if negative...
                     length=-length;       // make it positive
                     start=offset2;        // and set START to edge 2
                  } else 
                     start=offset1;     // else set START to edge 1
            
               	// draw the line
                 for (int i = 0; i <= length; i++) {  
                  vbuffer[start + i*4] = blue;       // blue
                  vbuffer[start + i*4 + 1] = green;  // green
                  vbuffer[start + i*4 + 2] = red;    // red
                  vbuffer[start + i*4 + 3] = alpha;  // alpha
                  }

                  offset1+=mempitch;           // advance edge 1 offset to next line
                  ystart1++;
                  offset2+=mempitch;           // advance edge 2 offset to next line
                  ystart2++;
               }

            } else {
               // increment edge 1 on X and edge 2 on Y:
               count1=xdiff1;    // count for X increment on edge 1
               count2=ydiff2;    // count for Y increment on edge 2
            
               while (count1 && count2) {  
                  // continue drawing until one edge is done
                  // calculate edge 1:
                  while ((errorterm1 < xdiff1) && (count1 > 0)) { 
                     // finished w/edge 1?
                     if (count1--) {
                        // count down on edge 1
                        offset1+=xunit1;  // increment pixel offset
                        xstart1+=(xunit1 / 4); //// update x coordiation
                     }

                     errorterm1+=ydiff1; // increment error term

                     if (errorterm1 < xdiff1) {  
                        // If not more than XDIFF
                     vbuffer[offset1] = blue;       // blue
                     vbuffer[offset1 + 1] = green;  // green
                     vbuffer[offset1 + 2] = red;    // red
                     vbuffer[offset1 + 3] = alpha;  // alpha
                     }
                  }

                  errorterm1-=xdiff1; // If time to increment X, restore error term

                  // calculate edge 2:
                  errorterm2+=xdiff2; // increment error term
                  
                  if (errorterm2 >= ydiff2)  { 
                     // if time to increment Y...
                     errorterm2-=ydiff2;        // ...restore error term
                     offset2+=xunit2;           // ...and advance offset to next pixel
                     xstart2+=(xunit2 / 4);     // update x coordiation
                  }

                  count2--;

                  // draw line from edge 1 to edge 2:

                  length=(offset2 - offset1) / 4; // determine length of horizontal line

                  if (length < 0)  { 
                     // if negative...
                     length=-length;       // ...make it positive
                     start=offset2;        // and set START to edge 2
                  } else 
                     start=offset1;        // else set START to edge 1

                     // from edge to edge
                     // ...draw the line
               for (int i = 0; i <= length; i++) {
                  vbuffer[start + i*4] = blue;       // blue
                  vbuffer[start + i*4 + 1] = green;  // green
                  vbuffer[start + i*4 + 2] = red;    // red
                  vbuffer[start + i*4 + 3] = alpha;  // alpha
                  }  
                  
                  offset1+=mempitch;           // advance edge 1 offset to next line
                  ystart1++;
                  offset2+=mempitch;           // advance edge 2 offset to next line
                  ystart2++;

               }
            }
         } else {
            if (xdiff2 > ydiff2) {
               // increment edge 1 on Y and edge 2 on X:

               count1=ydiff1;  // count for Y increment on edge 1
               count2=xdiff2;  // count for X increment on edge 2

               while(count1 && count2) {
                  // continue drawing until one edge is done
                  // calculate edge 1:

                  errorterm1+=xdiff1; // Increment error term

                  if (errorterm1 >= ydiff1) { 
                     // if time to increment Y...
                     errorterm1-=ydiff1;         // ...restore error term
                     offset1+=xunit1;            // ...and advance offset to next pixel
                     xstart1+=(xunit1 / 4);// update x coordiation
                  }

                  count1--;

                  // Calculate edge 2:

                  while ((errorterm2 < xdiff2) && (count2 > 0)) {
                        // finished w/edge 1?
                     if (count2--) {
                        // count down on edge 2
                        offset2+=xunit2;  // increment pixel offset
                        xstart2+=(xunit2 / 4);// update x coordiation
                     }

                     errorterm2+=ydiff2; // increment error term

                     if (errorterm2 < xdiff2) {
                        // if not more than XDIFF
                     vbuffer[offset2] = blue;       // blue
                     vbuffer[offset2 + 1] = green;  // green
                     vbuffer[offset2 + 2] = red;    // red
                     vbuffer[offset2 + 3] = alpha;  // alpha                        
                     }
                  }

                  errorterm2-=xdiff2;  // if time to increment X, restore error term

                  // draw line from edge 1 to edge 2:

                  length=(offset2 - offset1) / 4; // determine length of horizontal line

                  if (length < 0) {
                     // if negative...
                     length=-length;  // ...make it positive
                     start=offset2;   // and set START to edge 2
                  } else 
                     start=offset1;  // else set START to edge 1

                     // from edge to edge...
                     // ...draw the line
                 for (int i = 0; i <= length; i++) {
                  vbuffer[start + i*4] = blue;       // blue
                  vbuffer[start + i*4 + 1] = green;  // green
                  vbuffer[start + i*4 + 2] = red;    // red
                  vbuffer[start + i*4 + 3] = alpha;  // alpha
                  }

                  offset1+=mempitch;         // advance edge 1 offset to next line
                  ystart1++;
                  offset2+=mempitch;         // advance edge 2 offset to next line
                  ystart2++;

               }
            } else {
               // increment edge 1 on Y and edge 2 on Y:
               count1=ydiff1;  // count for Y increment on edge 1
               count2=ydiff2;  // count for Y increment on edge 2

               while(count1 && count2) {  
                  // continue drawing until one edge is done
                  // calculate edge 1:
                  errorterm1+=xdiff1;  // increment error term

                  if (errorterm1 >= ydiff1) {
                     // if time to increment Y
                     errorterm1-=ydiff1;         // ...restore error term
                     offset1+=xunit1;            // ...and advance offset to next pixel
                     xstart1+=(xunit1 / 4);
                  }
            
                  count1--;

                  // calculate edge 2:
                  errorterm2+=xdiff2;            // increment error term

                  if (errorterm2 >= ydiff2) {  
                     // if time to increment Y
                     errorterm2-=ydiff2;         // ...restore error term
                     offset2+=xunit2;            // ...and advance offset to next pixel
                     xstart2+=(xunit2 / 4);
                  }

                  --count2;

                  // draw line from edge 1 to edge 2:

                  length=(offset2 - offset1) / 4;  // determine length of horizontal line

                  if (length < 0) {          
                        // if negative...
                     length=-length;        // ...make it positive
                     start=offset2;         // and set START to edge 2
                  } else 
                     start=offset1;         // else set START to edge 1

                        // from edge to edge
                        // ...draw the line
	               for (int i = 0; i <= length; i++) {  
	                  vbuffer[start + i*4] = blue;       // blue
	                  vbuffer[start + i*4 + 1] = green;  // green
	                  vbuffer[start + i*4 + 2] = red;    // red
	                  vbuffer[start + i*4 + 3] = alpha;  // alpha
                  }

                  offset1+=mempitch;            // advance edge 1 offset to next line
                  ystart1++;
                  offset2+=mempitch;            // advance edge 2 offset to next line
                  ystart2++;

               }

            }

         }

         // another edge (at least) is complete. Start next edge, if any.
         if (!count1) {
                                    // if edge 1 is complete...
            --edgecount;           // decrement the edge count
            startvert1=endvert1;   // make ending vertex into start vertex
            --endvert1;            // and get new ending vertex
         
            if (endvert1 < 0) 
               endvert1=poly->num_verts-1; // check for wrap

            xend1=poly->vlist[endvert1].x+poly->x0;  // get x & y of new end vertex
            yend1=poly->vlist[endvert1].y+poly->y0;
         }

         if (!count2) {
                                 // if edge 2 is complete...
            --edgecount;          // decrement the edge count
            startvert2=endvert2;  // make ending vertex into start vertex
            endvert2++;           // and get new ending vertex
         
            if (endvert2==(poly->num_verts)) 
               endvert2=0;                // check for wrap

            xend2=poly->vlist[endvert2].x+poly->x0;  // get x & y of new end vertex
            yend2=poly->vlist[endvert2].y+poly->y0;
         }
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
   
   static int green_inc = 4; // used to animate the green glow
   static DWORD color = _RGB32BIT(0,0,255,0);
   UCHAR green; // used to animate the green glow

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

   // do the graphics
   Draw_Filled_Polygon2D(&object, (UCHAR *)ddsd.lpSurface, ddsd.lPitch);

   // rotate the polygon by 5 degrees
   Rotate_Polygon2D(&object, 5);

   // unlock primary buffer
   if (FAILED(lpddsback->Unlock(NULL)))
      return(0);

   // animate green
   green = (object.color >> 8) & 0xFF;
   green += green_inc;
   
   // check if ready to change animation in other direction
   if (green > 255 || green < 16) {
      // invert increment
      green_inc = -green_inc;
      green += green_inc;
   }

   object.color = _RGB32BIT(0, 0, green, 0);
   
   // draw the text
   DWORD text_color = _RGB32BIT(0,255,255,255);
   Draw_Text_GDI((char*)"Press <ESC> to exit.", 8,8,text_color, lpddsback);

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
   object.color       = _RGB32BIT(0,0,1,0); // animated green
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
                              L"Demo8_9",  // title
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