//#include "PPMHeader.h"
#include <iostream>
//
#include <memory>
#include <Windows.h>
#include <string>
//EnumDisplayDevices
//CreateDC

#include "GDICapture.h"


//the line stride need align to 32
int GDICapture(HWND hwnd,LPCWSTR savefilename) {
	SetFocus(hwnd);

	auto hTargetDC = GetDC(hwnd);

	auto hTempDC = CreateCompatibleDC(hTargetDC);

	//auto cx = GetDeviceCaps(hTargetDC, HORZRES);
	//auto cy = GetDeviceCaps(hTargetDC, VERTRES);

	RECT targetRect = { 0,0,0,0 };
	GetWindowRect(hwnd, &targetRect);
	auto cx = targetRect.right - targetRect.left;
	auto cy = targetRect.bottom - targetRect.top;

	auto hTempBitMap = CreateCompatibleBitmap(hTargetDC,cx,cy);

	SelectObject(hTempDC, hTempBitMap);

	BitBlt(hTempDC, 0, 0, cx, cy, hTargetDC, 0, 0, SRCCOPY);

	BITMAPINFOHEADER BitMapInfoHeader;
	BitMapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitMapInfoHeader.biWidth = cx;
	BitMapInfoHeader.biHeight = cy;
	BitMapInfoHeader.biPlanes = GetDeviceCaps(hTargetDC, PLANES);
	BitMapInfoHeader.biBitCount = GetDeviceCaps(hTargetDC, BITSPIXEL) * BitMapInfoHeader.biPlanes;
	BitMapInfoHeader.biCompression = BI_RGB;
	BitMapInfoHeader.biXPelsPerMeter = 0;
	BitMapInfoHeader.biYPelsPerMeter = 0;
	BitMapInfoHeader.biClrUsed = 0;
	BitMapInfoHeader.biClrImportant = 0;

	DWORD dwBmpSize = ((cx * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * cy;
	auto pixels = std::make_unique<char[]>(dwBmpSize);
	auto getBitsCount = GetDIBits(hTargetDC, hTempBitMap, 0, cy, pixels.get(), (BITMAPINFO *)&BitMapInfoHeader, DIB_RGB_COLORS);

	DeleteObject(hTempBitMap);
	DeleteObject(hTempDC);
	ReleaseDC(hwnd, hTargetDC);

	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFileW(savefilename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPFILEHEADER   bmfHeader;
	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&BitMapInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, pixels.get(), dwBmpSize, &dwBytesWritten, NULL);

	CloseHandle(hFile);

	return getBitsCount;
}

BOOL CALLBACK CaptureWindowProc(HWND hwnd, LPARAM lParam) {
	auto nChar = GetWindowTextLengthW(hwnd);

	std::wstring text = L"overlay ";
	if(nChar > 0)
		text.resize(nChar);

	GetWindowTextW(hwnd,&text[0], nChar);

	text.insert(text.size()-1, L".bmp");

	GDICapture(hwnd,text.c_str());

	return TRUE;
}


int main() {
	h3d::CaptureInfo info(NULL, 1280, 720);
	h3d::GDICapture hub(info);
	auto tex = hub.Capture();

	auto map = tex->Map();

	//

	tex->UnMap();

	return 0;
}