/* Time interval locking demo */

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

  HPEN pen = NULL; // used to draw saver
  int color_change_counter = 0; // used to track when to change color

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
                              L"Demo4_7",  // title
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

  /* get the graphics context once this time and keep it */
  /* this is possible due to the CS_OWNDC flag */
  hdc = GetDC(hwnd);

  // seed the random number generator
  srand(GetTickCount());

  // endpoints of line
  int x1 = rand() % WINDOW_WIDTH;
  int y1 = rand() % WINDOW_HEIGHT;
  int x2 = rand() % WINDOW_WIDTH;
  int y2 = rand() % WINDOW_HEIGHT;

  // initial velocity of each end
  int x1v = -4 + rand() % 8;
  int y1v = -4 + rand() % 8;
  int x2v = -4 + rand() % 8;
  int y2v = -4 + rand() % 8;


  // enter main event loop, but this time we use PeekMessage()
  // instead of GetMessage() to retrieve messages
  while (TRUE) {

    // get time reference
    DWORD start_time = GetTickCount();

    // test if there is a message in queue, if so get it
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // test if this is a quit
      if (WM_QUIT == msg.message) break;
      // translate any accelerator keys
      TranslateMessage(&msg);

      // send the message to the window proc
      DispatchMessage(&msg);
    }

    // is it time to change color?
    if (++color_change_counter > 100) {
        // reset counter
        color_change_counter = 0;
    
        /* delete old pen and create a random color pen */
        if (pen) DeleteObject(pen);
        pen = CreatePen(PS_SOLID, 1, RGB(rand() % 256, rand() % 256, rand() % 256));
        // select the pen into the device context
        SelectObject(hdc, pen);
    }

    // move the endpoints of the line
    x1 += x1v;
    y1 += y1v;
    x2 += x2v;
    y2 += y2v;

    // test if either end hit window edge
    if (x1 < 0 || x1 > WINDOW_WIDTH) {
        // invert velocity
        x1v = -x1v;
        // bum endpoint back
        x1 += x1v;
    }
    if (y1 < 0 || y1 > WINDOW_HEIGHT) {
        y1v = -y1v;
        y1 += y1v;
    }
    // now test second endpoint
    if (x2 < 0 || x2 > WINDOW_WIDTH) {
        x2v = -x2v;
        x2 += x2v;
    }
    if (y2 < 0 || y2 > WINDOW_HEIGHT) {
        y2v = -y2v;
        y2 += y2v;
    }

    // move to end one of line
    MoveToEx(hdc, x1, y1, NULL);
    // draw the line to other end
    LineTo(hdc, x2, y2);

    // lock time to 30 fps which is approx. 33 milliseconds per frame
    while (GetTickCount() - start_time < 33) {
        // do nothing, just wait
    }

    /* main game processing goes here */
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(hwnd, WM_CLOSE, 0, 0);
    }
    
  }

  ReleaseDC(hwnd, hdc);

  return msg.wParam;
}