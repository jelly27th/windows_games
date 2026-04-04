#define WIN32_LEAN_AND_MEAN // just say no to MFC

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <math.h>

#define _UNICODE

#define WINDOW_CLASS_NAME "WINCLASS1"
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  PAINTSTRUCT ps;
  HDC hdc;
  switch(msg)
  {
    case WM_CREATE:
    {
      // do initialization stuff here
      return 0;
    } break;

    case WM_PAINT:
    {
      // simple validate the window
      hdc = BeginPaint(hwnd, &ps);
      // you would do your painting here
      EndPaint(hwnd, &ps);
      return 0;
    } break;

    case WM_DESTROY:
    {
      // kill the application, this sends a WM_QUIT message
      PostQuitMessage(0);
      return 0;
    } break;

    default: break;
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
  winclass.hIcon = LoadIcon(NULL, IDC_APPSTARTING);
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  winclass.lpszMenuName = NULL;
  winclass.lpszClassName = WINDOW_CLASS_NAME;
  winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  // register the window class
  if (!RegisterClassEx(&winclass)) 
  {
    return 0;
  }

  // create the window
  if (!(hwnd = CreateWindowEx(NULL, // extended style
                              WINDOW_CLASS_NAME, // class
                              "Your Basic Window", //title
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              0,0, // initial x, y
                              400,400, // inital width, height
                              NULL, // handle to parent
                              NULL, // handle to menu
                              hinstance, //histance of this application
                              NULL))) // extra creation parms
  {
    return 0;
  }

  // enter main event loop
  while (GetMessage(&msg, NULL, 0, 0))
  {
    // translate any accelerator keys
    TranslateMessage(&msg);

    // send the message to the window proc
    DispatchMessage(&msg);
  }

  return msg.wParam;
}