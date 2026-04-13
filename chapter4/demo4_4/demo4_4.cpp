/* Ellipse Animation demo */

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
                              L"Demo4_3",  // title
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

  // create the pens and brushes
  HPEN white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
  HPEN black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HBRUSH green_brush = CreateSolidBrush(RGB(0, 255, 0));
  HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));

  // starting position of ball
  int ball_x = WINDOW_WIDTH / 2;
  int ball_y = WINDOW_HEIGHT / 2;

  // initial velocity of ball
  int xv = -4 + rand() % 8;
  int yv = -4 + rand() % 8;

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

    /* ERASE the last position of the ball */

    // first select the black brush and pen into the context
    SelectObject(hdc, black_pen);
    SelectObject(hdc, black_brush);

    // draw the ball
    Ellipse(hdc, ball_x, ball_y, ball_x + 32, ball_y + 32);

    // move the ball
    ball_x += xv;
    ball_y += yv;

    // test for collision with walls

    // first x-axis
    if (ball_x < 0 || ball_x > WINDOW_WIDTH - 32) {

      // invert x-velocity of ball
      xv = -xv;
      //push ball back
      ball_x += xv;

    } 
    // second y-axis
    else if (ball_y < 0 || ball_y > WINDOW_HEIGHT - 32) {
      
      // invert y-velocity of ball
      yv = -yv;
      //push ball back
      ball_y += yv;

    }

    // now slect the green and white colors for brush and pen
    SelectObject(hdc, white_pen);
    SelectObject(hdc, green_brush);

    // draw the ball
    Ellipse(hdc, ball_x, ball_y, ball_x + 32, ball_y + 32);

    /* main game processing goes here */
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(hwnd, WM_CLOSE, 0, 0);
    }

    // slow system down a little
    Sleep(10);
    
  }

  // delete all the objects we created
  DeleteObject(white_pen);
  DeleteObject(black_pen);
  DeleteObject(green_brush);
  DeleteObject(black_brush);

  ReleaseDC(hwnd, hdc);

  return msg.wParam;
}