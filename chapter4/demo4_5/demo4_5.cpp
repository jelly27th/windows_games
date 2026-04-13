/* Polygon drawing demo */

#define WIN32_LEAN_AND_MEAN  // just say no to MFC

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>

#define WINDOW_CLASS_NAME L"WINCLASS1"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 300

/* macro */
#define KEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEYUP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

/* global variables */
HWND main_window_handle = NULL;
HINSTANCE hinstance_app = NULL;
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
                              L"Demo4_5",  // title
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              0,0,          // initial x, y
                              WINDOW_WIDTH, WINDOW_HEIGHT,   // initial width, height
                              NULL,       // handle to parent
                              NULL,       // handle to menu
                              hinstance,  // histance of this application
                              NULL)))     // extra creation parms
  {
    return 0;
  }

  // save the main window handle
  main_window_handle = hwnd;

  // get the graphics device context, we can use this to draw on the window
  hdc = GetDC(hwnd);

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

    // select random color for polygon
    HPEN pen_color = CreatePen(PS_SOLID, 1, RGB(rand() % 256, rand() % 256, rand() % 256));
    HBRUSH brush_color = CreateSolidBrush(RGB(rand() % 256, rand() % 256, rand() % 256));

    // select the pen and brush into dc
    SelectObject(hdc, pen_color);
    SelectObject(hdc, brush_color);

    // create list of random points of polygon
    int num_points = 3 + rand() % 8; // between 3 and 10 points

    // create an array of points for the polygon
    POINT points[10];
    for (int i = 0; i < num_points; i++) {
      points[i].x = rand() % WINDOW_WIDTH;
      points[i].y = rand() % WINDOW_HEIGHT;
    }

    // draw the polygon
    Polygon(hdc, points, num_points);

    // let user see it
    Sleep(500);

    /* main game processing goes here */
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(hwnd, WM_CLOSE, 0, 0);
    }

  }

  ReleaseDC(hwnd, hdc);

  return msg.wParam;
}