#include "GDICapture.h"

extern "C"
{
#include <libswscale/swscale.h>
}


using namespace h3d;

#pragma warning(disable:4244)
GDITexture::GDITexture(HWND hwnd,SDst width, SDst height)
	:cx(width), cy(height)
{
	HDC src_hdc = ::GetDC(hwnd);
	compatible_dc = CreateCompatibleDC(src_hdc);
	//must create compatible bitmap from the src_hdc
	target_bitmap = CreateCompatibleBitmap(src_hdc, width, height);
	SelectObject(compatible_dc, target_bitmap);
	SetStretchBltMode(compatible_dc, HALFTONE);
	ReleaseDC(hwnd, src_hdc);


	BitMapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitMapInfoHeader.biWidth = width;
	BitMapInfoHeader.biHeight =height;
	BitMapInfoHeader.biPlanes = GetDeviceCaps(compatible_dc, PLANES);
	BitMapInfoHeader.biBitCount = GetDeviceCaps(compatible_dc, BITSPIXEL) * BitMapInfoHeader.biPlanes;
	BitMapInfoHeader.biCompression = BI_RGB;
	BitMapInfoHeader.biXPelsPerMeter = 0;
	BitMapInfoHeader.biYPelsPerMeter = 0;
	BitMapInfoHeader.biClrUsed = 0;
	BitMapInfoHeader.biClrImportant = 0;

	DWORD dwBmpSize = ((width * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * height;

	native = new byte[dwBmpSize];
}

GDITexture::~GDITexture()
{
	DeleteObject(target_bitmap);
	DeleteObject(compatible_dc);
	delete[] native;
}

#ifdef _DEBUG
void GDITexture::DumpBMP(const wchar_t* bmpfile)
{
	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFileW(bmpfile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwBmpSize = ((cx * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * cy;
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
	WriteFile(hFile, native, dwBmpSize, &dwBytesWritten, NULL);

	CloseHandle(hFile);
}
#endif

CaptureTexture::MappedData GDITexture::Map() {
	GetDIBits(compatible_dc, target_bitmap, 0, cy, native, (LPBITMAPINFO)&BitMapInfoHeader, DIB_RGB_COLORS);

	unsigned long pitch = ((cx * BitMapInfoHeader.biBitCount + 31) / 32) * 4;

	return{ native,pitch};
}

void GDITexture::ReSize(SDst width, SDst height) {

	target_bitmap = CreateCompatibleBitmap(compatible_dc, width, height);
	DeleteObject(SelectObject(compatible_dc, target_bitmap));
	delete[] native;

	BitMapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitMapInfoHeader.biWidth = width;
	BitMapInfoHeader.biHeight = height;
	BitMapInfoHeader.biPlanes = GetDeviceCaps(compatible_dc, PLANES);
	BitMapInfoHeader.biBitCount = GetDeviceCaps(compatible_dc, BITSPIXEL) * BitMapInfoHeader.biPlanes;
	BitMapInfoHeader.biCompression = BI_RGB;
	BitMapInfoHeader.biXPelsPerMeter = 0;
	BitMapInfoHeader.biYPelsPerMeter = 0;
	BitMapInfoHeader.biClrUsed = 0;
	BitMapInfoHeader.biClrImportant = 0;
	DWORD dwBmpSize = ((width * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * height;
	native = new byte[dwBmpSize];

	cx = width;
	cy = height;

}

GDICapture::GDICapture(CaptureInfo & info, CaptureCallBack callback)
	:capture_info(info),opt_callbak(callback)
{
	if (capture_info.sNative == 0) {
		capture_info.sNative =reinterpret_cast<__int64>(GetDesktopWindow());
	}

	HWND hwnd = (HWND)capture_info.sNative;

	SDst cx = info.oWidth;
	SDst cy = info.oHeight;

	RECT rect = { 0 , 0 , 0, 0 };
	GetWindowRect(hwnd, &rect);

	if(cx == -1)
		cx = rect.right - rect.left;
	if(cy == -1)
		cy = rect.bottom - rect.top;

	capture_tex = new GDITexture(hwnd, cx, cy);
}

GDICapture::~GDICapture() {
	
	delete capture_tex;
}

GDITexture* GDICapture::Capture() {
	HWND hwnd = (HWND)capture_info.sNative;

	RECT rect = { 0 , 0 , 0, 0 };
	GetWindowRect(hwnd, &rect);
	SDst cx = rect.right - rect.left;
	SDst cy = rect.bottom - rect.top;
	auto src_hdc = GetDC(hwnd);

	if (capture_info.oWidth == -1 || capture_info.oHeight == -1) {
		if (capture_tex->GetWidth() != cx || capture_tex->GetHeight() != cy)
			capture_tex->ReSize(cx, cy);
	}
	if (!StretchBlt(capture_tex->GetDC(), 0, capture_tex->GetHeight(), capture_tex->GetWidth(), -(int)capture_tex->GetHeight(), src_hdc, 0, 0, cx, cy, SRCCOPY)) {
		return NULL;
	}

	return capture_tex;
}