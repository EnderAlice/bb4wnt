#pragma once

/* msvcrtについてはコンパイラ側で勝手に追加するのでここでは書かない */

#ifdef UNICODE
/* Unicodeビルドの時のみcomctl32 6.0のマニフェストを追加する */

#ifdef _M_IX86
#pragma comment(linker,"/MANIFESTDEPENDENCY:\"name='Microsoft.Windows.Common-Controls' type='win32' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' processorArchitecture='x86'\"")
#endif

#ifdef _M_X64
#pragma comment(linker,"/MANIFESTDEPENDENCY:\"name='Microsoft.Windows.Common-Controls' type='win32' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' processorArchitecture='amd64'\"")
#endif

#ifdef _M_IA64
#pragma comment(linker,"/MANIFESTDEPENDENCY:\"name='Microsoft.Windows.Common-Controls' type='win32' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' processorArchitecture='ia64'\"")
#endif

#endif
