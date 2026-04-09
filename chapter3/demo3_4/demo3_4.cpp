/* loading a menu resources and processing selections */

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
#include "demo3_4res.H"

#define WINDOW_CLASS_NAME L"WINCLASS1"

/* global variables */
HWND main_window_handle = NULL;
HINSTANCE hinstance_app = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  PAINTSTRUCT ps;
  HDC hdc;
  switch (msg) {
    case WM_CREATE: {
      // do initialization stuff here

      return 0;
    } break;

    case WM_COMMAND: {
      switch (LOWORD(wparam))
      {
        case MENU_FILE_ID_EXIT:// handle the FILE menu
        {
          PostQuitMessage(0); // terminate window
        } break;
        case MENU_HELP_ABOUT: // handle the HELP menu
        {
          MessageBox(hwnd, L"Menu Sound Demo", L"About Sound Menu", MB_OK|MB_ICONEXCLAMATION);
        } break;
        // handle each of sounds
        case MENU_PLAY_ID_ENERGIZE:
        {
          PlaySound(MAKEINTRESOURCE(SOUND_ID_ENERGIZE), hinstance_app, SND_RESOURCE|SND_ASYNC);
        } break;
        case MENU_PLAY_ID_BEAM:
        {
          PlaySound(MAKEINTRESOURCE(SOUND_ID_BEAM), hinstance_app, SND_RESOURCE|SND_ASYNC);
        } break;
        case MENU_PLAY_ID_TELEPORT:
        {
          PlaySound(MAKEINTRESOURCE(SOUND_ID_TELEPORT), hinstance_app, SND_RESOURCE|SND_ASYNC);
        } break;
        case MENU_PLAY_ID_WARP:
        {
          PlaySound(MAKEINTRESOURCE(SOUND_ID_WARP), hinstance_app, SND_RESOURCE|SND_ASYNC);
        } break;

        default: break;
      }
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
  winclass.hIcon = LoadIcon(hinstance, MAKEINTRESOURCE(ICON_T3DX));
  winclass.hCursor = LoadCursor(hinstance, MAKEINTRESOURCE(CURSOR_CROSSHAIR));
  winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  winclass.lpszMenuName = L"SoundMenu";
  winclass.lpszClassName = WINDOW_CLASS_NAME;
  winclass.hIconSm = LoadIcon(hinstance, MAKEINTRESOURCE(ICON_T3DX));

  // register the window class
  if (!RegisterClassEx(&winclass)) {
    return 0;
  }

  // create the window
  if (!(hwnd = CreateWindowEx((DWORD)NULL,           // extended style
                              WINDOW_CLASS_NAME,    // class
                              L"Demo3_4",  // title
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