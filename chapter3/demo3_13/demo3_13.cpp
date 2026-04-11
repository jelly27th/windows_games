/* WM_MOUSEMOVE demo */

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

/* global variables */
HWND main_window_handle = NULL;
HINSTANCE hinstance_app = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  
  PAINTSTRUCT ps;
  HDC hdc;
  char buffer[80]; // used to print strings

  switch (msg) {
    case WM_CREATE: {
      // do initialization stuff here

      return 0;
    } break;

    case WM_MOUSEMOVE: {
      // extract the mouse position from lparam
      int mouse_x = (int)LOWORD(lparam);
      int mouse_y = (int)HIWORD(lparam);

      // get the botton state from wparam
      int buttons = (int)wparam;

      // get a graphics context
      hdc = GetDC(hwnd);
            
      // set the text color and background mode
      SetTextColor(hdc, RGB(0,255,0));
      SetBkColor(hdc, RGB(0,0,0));
      SetBkMode(hdc, OPAQUE);

      // draw the size of the window
      sprintf(buffer, "Mouse (X,Y) = (%d, %d)  ", mouse_x, mouse_y);
      TextOutA(hdc, 0,0, buffer, strlen(buffer));

      sprintf(buffer, "Right Buttons = %d  ", ((buttons & MK_RBUTTON) ? 1 : 0));
      TextOutA(hdc, 0, 16, buffer, strlen(buffer));

      sprintf(buffer, "Left Buttons = %d  ", ((buttons & MK_LBUTTON) ? 1 : 0));
      TextOutA(hdc, 0, 32, buffer, strlen(buffer));

      ReleaseDC(hwnd, hdc);

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
                              L"Demo3_13",  // title
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              0,0,          // initial x, y
                              400, 300,   // inital width, height
                              NULL,       // handle to parent
                              NULL,       // handle to menu
                              hinstance,  // histance of this application
                              NULL)))     // extra creation parms
  {
    return 0;
  }

  // save the main window handle
  main_window_handle = hwnd;

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

  }

  return msg.wParam;
}