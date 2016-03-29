#ifndef CPU_RenderSystem_Hpp
#define CPU_RenderSystem_Hpp

#include "RenderSystem.h"
#include "MemoryCapture.h"
#include "CPUTexture.h"

namespace h3d {
	class H3D_API CPUFactory :public RenderFactory {
	public:
		CPUFactory();

		GDITexture* CreateGDITexture(SDst Width, SDst Height);
		D3D11Texture* CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format = BGRX8);

	private:
		friend class CPUEngine;
	};
}

#endif
