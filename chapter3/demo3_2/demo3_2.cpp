/* loading .wav resources */

#define WIN32_LEAN_AND_MEAN  // just say no to MFC

#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mmsystem.h>

#define _UNICODE

HICON LoadIconFromFileEx(LPCTSTR filename) {
  HICON hIcon = (HICON)LoadImage(NULL, filename, IMAGE_ICON, 0, 0,
                                 LR_LOADFROMFILE | LR_DEFAULTSIZE);

  if (!hIcon) {
    hIcon = LoadIcon(NULL, IDI_APPLICATION);
  }

  return hIcon;
}

HCURSOR LoadCursorFromFileEx(LPCTSTR filename) {
  HCURSOR hCursor = (HCURSOR)LoadImage(NULL, filename, IMAGE_CURSOR, 0, 0,
                                       LR_LOADFROMFILE | LR_DEFAULTSIZE);

  if (!hCursor) {
    hCursor = LoadCursor(NULL, IDC_ARROW);
  }

  return hCursor;
}

#define WINDOW_CLASS_NAME "WINCLASS1"

/* global variables */
HWND main_window_handle = NULL;
HINSTANCE hinstance_app = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  PAINTSTRUCT ps;
  HDC hdc;
  switch (msg) {
    case WM_CREATE: {
      // do initialization stuff here

      // play the create sound once
      PlaySound("create.wav", hinstance_app, SND_FILENAME | SND_ASYNC);
      // play the music in loop mode
      PlaySound("techno.wav", hinstance_app, SND_FILENAME | SND_ASYNC | SND_LOOP);

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

  // first fill in the window class structure
  winclass.cbSize = sizeof(WNDCLASSEX);
  winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  winclass.lpfnWndProc = WindowProc;
  winclass.cbClsExtra = 0;
  winclass.cbWndExtra = 0;
  winclass.hInstance = hinstance;
  winclass.hIcon = LoadIconFromFileEx("t3dx.ico");
  winclass.hCursor = LoadCursorFromFileEx("crosshair.cur");
  winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  winclass.lpszMenuName = NULL;
  winclass.lpszClassName = WINDOW_CLASS_NAME;
  winclass.hIconSm = LoadIconFromFileEx("t3dx.ico");

  // register the window class
  if (!RegisterClassEx(&winclass)) {
    return 0;
  }

  // create the window
  if (!(hwnd = CreateWindowEx(NULL,                 // extended style
                              WINDOW_CLASS_NAME,    // class
                              "demo of custom curson and icon",  // title
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              0,0,          // initial x, y
                              400, 400,   // inital width, height
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
  }

  return msg.wParam;
}