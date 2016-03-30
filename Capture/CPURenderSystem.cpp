#include "CPURenderSystem.h"

using namespace h3d;

h3d::CPUFactory::CPUFactory()
{
}

GDITexture * h3d::CPUFactory::CreateGDITexture(HWND hwnd,SDst Width, SDst Height)
{
	return new GDITexture(hwnd,Width,Height);
}

MemoryTexture * h3d::CPUFactory::CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format)
{
	return new MemoryTexture(Width,Height);
}

bool h3d::CPUEngine::Construct(HWND hwnd)
{
	return true;
}

void h3d::CPUEngine::Destroy()
{
	curr_rt = NULL;
}

CPUFactory & h3d::CPUEngine::GetFactory()
{
	return factory;
}

void h3d::CPUEngine::BeginDraw(CaptureTexture * rt, BLEND_TYPE bt)
{
	curr_rt = rt;
	curr_bt = bt;
}

void h3d::CPUEngine::Draw(SDst x, SDst y, SDst width, SDst height, CaptureTexture * src, bool flip)
{
	//todo support flip support alpha blend
	RECT rect = { x,y,width,height };
	point_tff(src, curr_rt, rect);
}

void h3d::CPUEngine::EndDraw()
{
}

void h3d::CPUEngine::bilinear_tff(CaptureTexture * src, CaptureTexture * dst, RECT dst_rect)
{
	//todo impl
	point_tff(src, dst, dst_rect);
}

void h3d::CPUEngine::point_tff(CaptureTexture * src, CaptureTexture * dst, RECT dst_rect)
{
	SDst src_w = src->GetWidth();
	SDst src_h = src->GetHeight();

	SDst dst_w = dst_rect.right;
	SDst dst_h = dst_rect.bottom;

	SDst y_offset = dst_rect.top;
	SDst x_offset = dst_rect.bottom;

	double scale_w = src_w * 1.0 / dst_w;
	double scale_h = src_h * 1.0 / dst_h;
	h3d::CaptureTexture::MappedData srcmapped = src->Map();
	h3d::CaptureTexture::MappedData dstmapped = dst->Map();

	for (SDst y = 0; y != dst_h; ++y) {
		SDst src_y = min(src_h, static_cast<SDst>(scale_h*y + 0.5));//需要优化，强制忽略边界判断
		for (SDst x = 0; x != dst_w; ++x) {
			int src_x = min(src_w, static_cast<SDst>(scale_w*x + 0.5));
			memcpy(
				dstmapped.pData + (y_offset + y)*dstmapped.RowPitch + (x_offset + x) * 4, 
				srcmapped.pData + src_y*srcmapped.RowPitch + src_x * 4, 
				4);
		}
	}
}

H3D_API CPUEngine & h3d::GetCPUEngine()
{
	static CPUEngine mInstance;
	return mInstance;
}
