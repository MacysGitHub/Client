#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <winuser.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#pragma pack(push, 1)

WSAData wData;
WORD ver = MAKEWORD(2, 2);


int main() {
	int wsOk = WSAStartup(ver, &wData);
	if (wsOk != 0) {
		std::cerr << "Error Initializing WinSock! Exiting" << std::endl;
		return -1;
	}
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	//Bind IP address to port
	std::string ipAddress = "127.0.0.1";
	sockaddr_in sockin;
	sockin.sin_family = AF_INET;
	sockin.sin_port = htons(8081);
	inet_pton(AF_INET, ipAddress.c_str(), &sockin.sin_addr);

	std::cout << "Attempting to connect to master controller..." << std::endl;

	int x1, y1, x2, y2, w, h;

	// get screen dimensions
	x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
	x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	w = x2 - x1;
	h = y2 - y1;

	/*int* Buf = new int[64];
	ZeroMemory(Buf, 64);
	Buf[0] = w;
	Buf[1] = h;*/

	//std::cout << Buf[0] << " " << Buf[1] << std::endl;

	// copy screen to bitmap
	HDC     hScreen = GetDC(NULL);
	HDC     hdcCompat = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
	//Use the previously created device context with the bitmap
	HGDIOBJ old_obj = SelectObject(hdcCompat, hBitmap);
	StretchBlt(hdcCompat, 0, 0, w, h, hScreen, x1, y1, w, h, SRCCOPY);   //change SRCCOPY to NOTSRCCOPY for wacky colors !
	//BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x1, y1, SRCCOPY);
	DWORD dwBmpSize = 0;

	BITMAP bmpScreen;
	GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;
	char* lpbitmap = NULL;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = w;
	bi.biHeight = h;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HGLOBAL hDIB = GlobalAlloc(GHND, dwBmpSize);
	lpbitmap = (char*)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap, and copies them into a buffer 
	// that's pointed to by lpbitmap.
	GetDIBits(hScreen, hBitmap, 0,
		(UINT)h,
		lpbitmap,
		(BITMAPINFO*)&bi, DIB_RGB_COLORS);

	//HBITMAP hbitTest = CreateBitmap(4096, 2160, 1, 32, lpbitmap);
	HBITMAP hbitTest = CreateCompatibleBitmap(hScreen, w, h);

	int diSet = SetDIBits(hScreen, hbitTest, 0,
		(UINT)h,
		lpbitmap,
		(BITMAPINFO*)&bi, DIB_RGB_COLORS);

	std::cout << "diBitSet: " << diSet << std::endl;

	//Message protocol


	//Connect to server
	bool connected = true;
	while (connected) {
		if (connect(sock, (sockaddr*)&sockin, sizeof(sockin)) != SOCKET_ERROR) {
			//1. Send hDC
			//2. Send Bitmap Info Header
			//3. 
			char* buffer = new char[sizeof(BITMAPINFOHEADER)];
			memcpy(buffer, &bi, sizeof(BITMAPINFOHEADER));
			std::cout << "sent Device Context" << std::endl;
			if (send(sock, (char*)buffer, sizeof(bi), NULL)) {
				std::cout << "sent bitmapinfo" << std::endl;

			}
		}

		send(sock, (char*)lpbitmap, ((((w * 32 + 31) / 32) * 4) * h) + 1, NULL);

	}
}