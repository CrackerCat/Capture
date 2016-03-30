#ifndef CPU_RenderSystem_Hpp
#define CPU_RenderSystem_Hpp

#include "RenderSystem.h"
#include "CPUTexture.h"

namespace h3d {
	class H3D_API CPUFactory :public RenderFactory {
	public:
		CPUFactory();

		GDITexture* CreateGDITexture(HWND hwnd,SDst Width, SDst Height);
		MemoryTexture* CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format = BGRX8);

	private:
		friend class CPUEngine;
	};

	class H3D_API CPUEngine : public RenderEngine {
	public:
		bool Construct(HWND hwnd);
		void Destroy();
	public:
		CPUFactory& GetFactory();

		void BeginDraw(CaptureTexture* rt, BLEND_TYPE bt);
		void Draw(SDst x, SDst y, SDst width, SDst height, CaptureTexture* src, bool flip = false);
		void EndDraw();
	private:
		static void bilinear_tff(CaptureTexture* src,CaptureTexture* dst,RECT dst_rect);
		static void point_tff(CaptureTexture* src, CaptureTexture* dst, RECT dst_rect);
	private:
		CaptureTexture * curr_rt;
		BLEND_TYPE curr_bt;


		CPUFactory factory;
	};

	H3D_API CPUEngine&  GetCPUEngine();
}

#endif
