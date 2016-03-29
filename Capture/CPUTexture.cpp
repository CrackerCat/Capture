#include "CPUTexture.h"

using namespace h3d;

#pragma warning(disable:4244)
GDITexture::GDITexture(HWND hwnd, SDst width, SDst height)
	:CaptureTexture(Memory_Texture), cx(width), cy(height)
{
	//使用GetDC 只抓取客户区
	HDC src_hdc = ::GetWindowDC(hwnd);
	compatible_dc = CreateCompatibleDC(src_hdc);
	//must create compatible bitmap from the src_hdc
	target_bitmap = CreateCompatibleBitmap(src_hdc, width, height);
	SelectObject(compatible_dc, target_bitmap);
	SetStretchBltMode(compatible_dc, HALFTONE);
	::ReleaseDC(hwnd, src_hdc);


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
}

GDITexture::~GDITexture()
{
	DeleteObject(target_bitmap);
	DeleteObject(compatible_dc);
	delete[] native;
}

#ifdef _DEBUG
//void GDITexture::DumpBMP(const wchar_t* bmpfile)
//{
//	// A file is created, this is where we will save the screen capture.
//	HANDLE hFile = CreateFileW(bmpfile,
//		GENERIC_WRITE,
//		0,
//		NULL,
//		CREATE_ALWAYS,
//		FILE_ATTRIBUTE_NORMAL, NULL);
//
//	DWORD dwBmpSize = ((cx * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * cy;
//	// Add the size of the headers to the size of the bitmap to get the total file size
//	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
//
//	BITMAPFILEHEADER   bmfHeader;
//	//Offset to where the actual bitmap bits start.
//	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
//
//	//Size of the file
//	bmfHeader.bfSize = dwSizeofDIB;
//
//	//bfType must always be BM for Bitmaps
//	bmfHeader.bfType = 0x4D42; //BM   
//
//	DWORD dwBytesWritten = 0;
//	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
//	WriteFile(hFile, (LPSTR)&BitMapInfoHeader, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
//	WriteFile(hFile, native, dwBmpSize, &dwBytesWritten, NULL);
//
//	CloseHandle(hFile);
//}
#endif

CaptureTexture::MappedData GDITexture::Map() {
	GetDIBits(compatible_dc, target_bitmap, 0, cy, native, (LPBITMAPINFO)&BitMapInfoHeader, DIB_RGB_COLORS);

	unsigned long pitch = ((cx * BitMapInfoHeader.biBitCount + 31) / 32) * 4;

	return{ native,pitch };
}

//void GDITexture::ReSize(SDst width, SDst height) {
//
//	target_bitmap = CreateCompatibleBitmap(compatible_dc, width, height);
//	DeleteObject(SelectObject(compatible_dc, target_bitmap));
//	delete[] native;
//
//	BitMapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
//	BitMapInfoHeader.biWidth = width;
//	BitMapInfoHeader.biHeight = height;
//	BitMapInfoHeader.biPlanes = GetDeviceCaps(compatible_dc, PLANES);
//	BitMapInfoHeader.biBitCount = GetDeviceCaps(compatible_dc, BITSPIXEL) * BitMapInfoHeader.biPlanes;
//	BitMapInfoHeader.biCompression = BI_RGB;
//	BitMapInfoHeader.biXPelsPerMeter = 0;
//	BitMapInfoHeader.biYPelsPerMeter = 0;
//	BitMapInfoHeader.biClrUsed = 0;
//	BitMapInfoHeader.biClrImportant = 0;
//	DWORD dwBmpSize = ((width * BitMapInfoHeader.biBitCount + 31) / 32) * 4 * height;
//	native = new byte[dwBmpSize];
//
//	cx = width;
//	cy = height;
//
//}


#pragma warning(disable:4244)
MemoryTexture::MemoryTexture(SDst width, SDst height)
	:CaptureTexture(Memory_Texture), cx(width), cy(height)
{
	native = new byte[width*height * 4];
}

MemoryTexture::~MemoryTexture()
{
	delete[] native;
}

MemoryTexture::MappedData h3d::MemoryTexture::Map()
{
	return{ native,static_cast<unsigned long>(cx * 4) };
}

//void MemoryTexture::ReSize(SDst width, SDst height)
//{
//	cx = width;
//	cy = height;
//	delete[] native;
//	native = new byte[width*height * 4];
//}