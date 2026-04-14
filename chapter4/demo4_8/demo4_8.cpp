/* Child window Button Demo */

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

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

/* button macro */
#define BUTTON_BASE_ID 100
#define NUM_BUTTONS 8

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
    
    case WM_COMMAND: {
      /* all buttons come through here */
      
      hdc = GetDC(hwnd);

      // set background mode
      SetBkMode(hdc, OPAQUE);

      // select a random text and background color
      SetTextColor(hdc, RGB(0, 255, 0));
      SetBkColor(hdc, RGB(128, 128, 128));

      // print out the wparam and lparam
      sprintf(buffer, "LOWORD(wparam) = %d, HIWORD(wparam) = %d", LOWORD(wparam), HIWORD(wparam));
      
      // print text at a fixed location
      TextOutA(hdc, 220, 100, buffer, strlen(buffer));

      // print out the wparam and lparam
      sprintf(buffer, "LOWORD(lparam) = 0X%X, HIWORD(lparam) = 0X%X", LOWORD(lparam), HIWORD(lparam));
      
      // print text at a fixed location
      TextOutA(hdc, 220, 140, buffer, strlen(buffer));

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
                              L"Demo4_8",  // title
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

  /* create the buttons*/
  const wchar_t* button_names[NUM_BUTTONS] = 
        {      
            L"PUSHBUTTON", 
            L"RADIOBUTTON", 
            L"CHECKBOX", 
            L"3STATE", 
            L"AUTO3STATE", 
            L"AUTOCHECKBOX", 
            L"AUTORADIOBUTTON", 
            L"OWNED"};

  long button_styles[NUM_BUTTONS] = 
        {      
            BS_PUSHBUTTON, 
            BS_RADIOBUTTON, 
            BS_CHECKBOX, 
            BS_3STATE, 
            BS_AUTO3STATE, 
            BS_AUTOCHECKBOX, 
            BS_AUTORADIOBUTTON, 
            BS_OWNERDRAW};

    for (int button = 0; button < NUM_BUTTONS; button++) {
        CreateWindowEx((DWORD)NULL,           // extended style
                        L"button",    // class
                        button_names[button],  // title
                        WS_CHILD | WS_VISIBLE | button_styles[button], // style
                        10,10+(button*36),          // initial x, y
                        wcslen(button_names[button])*16, 24,   // initial width, height
                        main_window_handle,       // handle to parent
                        (HMENU)(UINT_PTR)(BUTTON_BASE_ID + button),       // handle to menu
                        hinstance,  // histance of this application
                        NULL);    // extra creation parms
    }

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
    if (KEYDOWN(VK_ESCAPE)) {
      SendMessage(hwnd, WM_CLOSE, 0, 0);
    }
    
  }

  return msg.wParam;
}