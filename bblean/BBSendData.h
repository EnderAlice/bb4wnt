//-------------------------------------------------------------------------
// BBSendData.h

#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <string.h>

struct bb_senddata
{
    WPARAM wParam;
    BYTE lParam_data[256];
};

// Send a message with data to bbLean from another process
// returns: nonzero on success, zero on error

// Usage example: Set a style
// BBSendData(FindWindow("BlackboxClass", "Blackbox"), BB_SETSTYLE, stylefile, 1+strlen(stylefile));

BOOL BBSendData(
    HWND BBhwnd,        // blackbox core window
    UINT msg,           // the message to send
    WPARAM wParam,      // wParam
    LPCVOID lParam,     // lParam
    size_t lParam_size     // size of the data referenced by lParam
    )
{
    if (NULL == BBhwnd) return FALSE;
    struct bb_senddata BBSD, *pBBSD;
    size_t s;

    if (-1 == lParam_size) lParam_size = (1+_tcsnlen((LPCTSTR)lParam, 0x3FFF)) * sizeof(TCHAR);
    s = sizeof(BBSD)- sizeof(BBSD.lParam_data) + lParam_size;

    // for speed, the local buffer is used when sufficient
    if (s <= sizeof(BBSD))
        pBBSD = &BBSD;
    else
        pBBSD = (struct bb_senddata*)malloc(s);

    pBBSD->wParam = wParam;
    memcpy(pBBSD->lParam_data, lParam, lParam_size);

    COPYDATASTRUCT cds;
    cds.dwData = msg;
    cds.cbData = (DWORD)s;
    cds.lpData = (void*)pBBSD;
    LRESULT result = SendMessage (BBhwnd, WM_COPYDATA, 0, (LPARAM)&cds);
    if (pBBSD != &BBSD) free(pBBSD);
    return result != 0;
}

// Receive a message from bbLean

// Usage example: Get the path to the currently loaded stylefile:
// 1. Send the request:
//      SendMessage(FindWindow("BlackboxClass", "Blackbox"), BB_GETSTYLE, 0, 0);
// 2. Handle the answer:
//      case WM_COPYDATA: return BBReceiveData(hwnd, lParam);
//      case BB_SETSTYLE: { LPCSTR stylepath = (LPCSTR)lParam; /* do whatever with it */ }

BOOL BBReceiveData(HWND hwnd, LPARAM lParam)
{
    UINT msg = (UINT)((COPYDATASTRUCT*)lParam)->dwData;
    if (msg < BB_MSGFIRST || msg >= BB_MSGLAST) return TRUE;
    struct bb_senddata *pBBSD = (struct bb_senddata*)((COPYDATASTRUCT*)lParam)->lpData;
    if (BB_SENDDATA == msg)
        memcpy((void*)pBBSD->wParam, pBBSD->lParam_data, ((PCOPYDATASTRUCT)lParam)->cbData);
    else
        SendMessage(hwnd, msg, pBBSD->wParam, (LPARAM)pBBSD->lParam_data);
    return TRUE;
}

