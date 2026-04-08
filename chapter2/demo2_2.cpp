/* a simple message box */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#define _UNICODE

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow)
{
  MessageBox(NULL, "THERE CAN BE ONLY ONE!!!", "MY FIRST WINDOWS PROGRAM", MB_OK|MB_ICONEXCLAMATION);
  return 0;
}