#include <stdio.h>
#include "common.h"

void print_error(const char *context){
    DWORD code = GetLastError();
    wchar_t wmsg[512];
    char msg[1024] = "unknown error";
    DWORD wlen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, code, 0, wmsg, 512, NULL);
    if (wlen > 0){
        UINT cp = GetConsoleOutputCP();
        if (cp == 0){
            cp = CP_ACP;
        }
        int n = WideCharToMultiByte(cp, 0, wmsg, (int)wlen, msg, (int)sizeof(msg) - 1, NULL, NULL);
        if (n > 0){
            msg[n] = '\0';
            while(n > 0 && (msg[n - 1] == '\r' || msg[n - 1] == '\n' || msg[n - 1] == ' ')){
                msg[--n] = '\0';
            }
        }
    }
    fprintf(stderr, "ERROR: %s: %s (code %lu)\n", context, msg, (unsigned long)code);
}

double now_ms(void){
    static LARGE_INTEGER freq;
    LARGE_INTEGER t;
    if (freq.QuadPart == 0){
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart * 1000.0 / (double)freq.QuadPart;
}
