#include "CaptureHook.h"
#include "opengl.h"

namespace {
	bool has_textured = false;
	bool bFBOAvailable = false;

	using h3d::APIHook;
	APIHook                hdc_swapbuffers;
	APIHook                wgl_hdc_swaplayerbuffers;;
	APIHook                wgl_hdc_swapbuffers;
	APIHook                wgl_delcontext;

	h3d::CaptureInfo             glcaptureInfo;
}

WGLSWAPLAYERBUFFERSPROC  jimglSwapLayerBuffers = NULL;
WGLSWAPBUFFERSPROC       jimglSwapBuffers = NULL;
WGLDELETECONTEXTPROC     jimglDeleteContext = NULL;
WGLMAKECURRENTPROC       jimglMakeCurrent = NULL;
WGLGETCURRENTDCPROC      jimglGetCurrentDC = NULL;
WGLGETCURRENTCONTEXTPROC jimglGetCurrentContext = NULL;
WGLCREATECONTEXTPROC     jimglCreateContext = NULL;
WGLSETRESOURCESHAREHANDLENVPROC wglDXSetResourceShareHandleNV = NULL;
WGLDXOPENDEVICENVPROC           wglDXOpenDeviceNV = NULL;
WGLDXCLOSEDEVICENVPROC          wglDXCloseDeviceNV = NULL;
WGLDXREGISTEROBJECTNVPROC       wglDXRegisterObjectNV = NULL;
WGLDXUNREGISTEROBJECTNVPROC     wglDXUnregisterObjectNV = NULL;
WGLDXOBJECTACCESSNVPROC         wglDXObjectAccessNV = NULL;
WGLDXLOCKOBJECTSNVPROC          wglDXLockObjectsNV = NULL;
WGLDXUNLOCKOBJECTSNVPROC        wglDXUnlockObjectsNV = NULL;


GLGENFRAMEBUFFERSPROC      glGenFramebuffers = NULL;
GLDELETEFRAMEBUFFERSPROC   glDeleteFramebuffers = NULL;
GLBINDFRAMEBUFFERPROC      glBindFramebuffer = NULL;
GLBLITFRAMEBUFFERPROC      glBlitFramebuffer = NULL;
GLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;

GLREADBUFFERPROC         glReadBuffer = NULL;
GLDRAWBUFFERPROC         glDrawBuffer = NULL;
GLREADPIXELSPROC         glReadPixels = NULL;
GLGETINTEGERVPROC        glGetIntegerv = NULL;
GLGETERRORPROC           glGetError = NULL;


static void RegisterNVCapture();
static void RegisterFBO();

WGLGETPROCADDRESSPROC    jimglGetProcAddress = NULL;

GLBUFFERDATAARBPROC     glBufferData = NULL;
GLDELETEBUFFERSARBPROC  glDeleteBuffers = NULL;
GLDELETETEXTURESPROC    glDeleteTextures = NULL;
GLGENBUFFERSARBPROC     glGenBuffers = NULL;
GLGENTEXTURESPROC       glGenTextures = NULL;
GLMAPBUFFERPROC         glMapBuffer = NULL;
GLUNMAPBUFFERPROC       glUnmapBuffer = NULL;
GLBINDBUFFERPROC        glBindBuffer = NULL;
GLBINDTEXTUREPROC       glBindTexture = NULL;
static inline bool GetCaptureProcAddress()
{
	static bool get_address_succ = false;

	if (!get_address_succ)
	{
		HMODULE hGL = GetModuleHandle(TEXT("opengl32.dll"));

		glBufferData = (GLBUFFERDATAARBPROC)jimglGetProcAddress("glBufferData");
		glDeleteBuffers = (GLDELETEBUFFERSARBPROC)jimglGetProcAddress("glDeleteBuffers");
		glDeleteTextures = (GLDELETETEXTURESPROC)GetProcAddress(hGL, "glDeleteTextures");
		glGenBuffers = (GLGENBUFFERSARBPROC)jimglGetProcAddress("glGenBuffers");
		glGenTextures = (GLGENTEXTURESPROC)GetProcAddress(hGL, "glGenTextures");
		glMapBuffer = (GLMAPBUFFERPROC)jimglGetProcAddress("glMapBuffer");
		glUnmapBuffer = (GLUNMAPBUFFERPROC)jimglGetProcAddress("glUnmapBuffer");
		glBindBuffer = (GLBINDBUFFERPROC)jimglGetProcAddress("glBindBuffer");
		glGetIntegerv = (GLGETINTEGERVPROC)GetProcAddress(hGL, "glGetIntegerv");
		glBindTexture = (GLBINDTEXTUREPROC)GetProcAddress(hGL, "glBindTexture");

		if (!glReadBuffer || !glReadPixels || !glGetError || !jimglSwapLayerBuffers || !jimglSwapBuffers ||
			!jimglDeleteContext || !jimglGetProcAddress || !jimglMakeCurrent || !jimglGetCurrentDC ||
			!jimglGetCurrentContext || !jimglCreateContext)
		{
			return false;
		}

		RegisterNVCapture();
		RegisterFBO();
		get_address_succ = true;
	}

	return true;
}

void ClearGLData();

#include "D3D9_OPENGL_GPU.inl"

namespace {
	GLuint gl_fbo = 0;
	GLuint gl_sharedtex = 0;

	HANDLE gl_handle = NULL;
	HANDLE gl_d3ddevice = NULL;

	//nvidia specific(but also works on AMD surprisingly)
	//https://www.opengl.org/registry/specs/NV/DX_interop.txt
	bool nvidia_extend_support = false;

	IDXGISwapChain* dxgi_swapchain = NULL;
}

extern bool bDXGICapture;
IDXGISwapChain* CreateDummySwapChainByDevice(ID3D11Device * device) {
	HMODULE hDXGIDll = LoadLibraryW(L"dxgi.dll");
	if (!hDXGIDll)
		return NULL;

	h3d::DXGICREATEPROC CreateDXGI = (h3d::DXGICREATEPROC)GetProcAddress(hDXGIDll, "CreateDXGIFactory1");

	IDXGIFactory1* pFactory = NULL;
	if (FAILED(CreateDXGI(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory))))
		return NULL;

	//DXGI Captrure use some window create dummy swap_chian
	bDXGICapture = true;

	IDXGISwapChain* pSwapChian = NULL;
	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(swapDesc));
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.Width = 2;
	swapDesc.BufferDesc.Height = 2;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = TRUE;
	swapDesc.OutputWindow = hSender;
	pFactory->CreateSwapChain(device, &swapDesc, &pSwapChian);

	pFactory->Release();
	return pSwapChian;
}

static bool CreateOpenGLGPUCapture(RECT &rc)
{
	glcaptureInfo.oWidth = rc.right;
	glcaptureInfo.oHeight = rc.bottom;

	HANDLE shared_handle = h3d::CreateSharedTexture(d3d11_texture, d3d11_device, d3d11_context, DXGI_FORMAT_B8G8R8X8_UNORM, rc.right, rc.bottom);

	if (!shared_handle || !d3d11_device || !d3d11_texture) {
		logstream << "CreateOpenGLGPUCapture CreateSharedTexture Failed" << std::endl;
		ClearGLData();
		return false;
	}

	dxgi_swapchain = CreateDummySwapChainByDevice(d3d11_device);

	if (nvidia_extend_support) {
		gl_d3ddevice = wglDXOpenDeviceNV(d3d11_device);
		if (gl_d3ddevice == NULL) {
			logstream << "CreateOpenGLGPUCapture: wglDXOpenDeviceNV failed" << std::endl;
			ClearGLData();
			return false;
		}

		glGenTextures(1, &gl_sharedtex);
		gl_handle = wglDXRegisterObjectNV(gl_d3ddevice, d3d11_texture, gl_sharedtex, GL_TEXTURE_2D, WGL_ACCESS_WRITE_DISCARD_NV);

		if (gl_handle == NULL) {
			logstream << "CreateOpenGLGPUCapture: wglDXRegisterObjectNV failed" << std::endl;
			ClearGLData();
			return false;
		}
	}

	logstream << "D3DDevice <-> GL_D3DDevice : " << d3d11_device << " <->" << gl_d3ddevice << std::endl;
	logstream << "D3DTexture2D <-> GL_HANDLE : " << d3d11_texture << " <-> " << gl_handle << std::endl;

	glGenFramebuffers(1, &gl_fbo);

	glcaptureInfo.Reserved2 = h3d::CtorSharedMemGPUCapture(&pTexHandle);
	if (!glcaptureInfo.Reserved2)
	{
		logstream << "CreateOpenGLGPUCapture: failed to initialize shared memory" << std::endl;
		ClearGLData();
		return false;
	}

	glcaptureInfo.Reserved3 = -1;
	has_textured = true;
	*pTexHandle = reinterpret_cast<unsigned  __int64>(shared_handle);
	memcpy(ptr_capture_info, &glcaptureInfo, sizeof(h3d::CaptureInfo));
	SetEvent(hReadyEvent);
	logstream << "Allthing has ready,OpenGL GPU HOOK Success[NVIDIA SPEC][Sent Event to CaptureApp]" << std::endl;
	return true;
}

#include "D3D8_D3D9_OPENGL_CPU.inl"

#ifdef NUM_GLBACKBUFF
//2 buffers turns out to be more optimal than 3 -- seems that cache has something to do with this in GL
#undef NUM_GLBACKBUFF
#endif
#define                 NUM_GLBACKBUFF 2
GLuint                  gltextures[NUM_GLBACKBUFF] = { 0,0 };

namespace {
	DWORD curr_capture = 0;
}

void CreateOpenGLCPUCapture(RECT &rc)
{
	nvidia_extend_support = false;
	glcaptureInfo.oWidth = rc.right;
	glcaptureInfo.oHeight = rc.bottom;

	glGenBuffers(NUM_GLBACKBUFF, gltextures);

	DWORD dwSize = rc.right*rc.bottom;

	GLint lastPPB;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &lastPPB);

	for (UINT i = 0; i < NUM_GLBACKBUFF; i++)
	{
		UINT test = 0;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, gltextures[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, dwSize, 0, GL_STREAM_READ);


		InitializeCriticalSection(&data_mutexs[i]);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, lastPPB);

	if (CopySignal(glcaptureInfo, glcaptureInfo.oWidth * 4))
		logstream << "Allthing has ready,OpenGL CPU HOOK Success[Sent Event to CaptureApp]" << std::endl;
	else
		ClearGLData();
}

void CreateOpenGLCapture(RECT &rc)
{
	if (bFBOAvailable && nvidia_extend_support && CreateOpenGLGPUCapture(rc))
		return;

	CreateOpenGLCPUCapture(rc);
}

bool                    glLockedTextures[NUM_GLBACKBUFF];

LONG lastCX = 0, lastCY = 0;


HDC    hdc_acquried = NULL;
HWND   hwnd_acquired = NULL;

#include "CaptureHookCommon.h"

void OpenGLHDCCapture(HDC hDC)
{
	if (!GetCaptureProcAddress())
		return;

	if (hdc_acquried == NULL)
	{
		logstream << "setting up gl data" << std::endl;
		//PIXELFORMATDESCRIPTOR pfd;

		hwnd_acquired = ::WindowFromDC(hDC);

		//int pixFormat = ::GetPixelFormat(hDC);
		//DescribePixelFormat(hDC, pixFormat, sizeof(pfd), &pfd);

		if (hwnd_acquired)
		{
			target_acquired = true;
			hdc_acquried = hDC;
			glcaptureInfo.Reserved1 = h3d::BGRX8;
		}
		else
			return;
	}


	if (hDC == hdc_acquried)
	{
		h3d::EventProcess();

		if (!capture_run)
			ClearGLData();

		RECT rc;
		GetClientRect(hwnd_acquired, &rc);

		if (capture_run && !has_textured)
			CreateOpenGLCapture(rc);


		KeepAliveProcess(ClearGLData())

			if (has_textured && capture_run)
			{
				if (nvidia_extend_support) {
					wglDXLockObjectsNV(gl_d3ddevice, 1, &gl_handle);

					GLint lastFBO, lastTex2D;
					glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastFBO);
					glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTex2D);

					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_fbo);

					glBindTexture(GL_TEXTURE_2D, gl_sharedtex);
					glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_sharedtex, 0);

					glReadBuffer(GL_BACK); //source
					glDrawBuffer(GL_COLOR_ATTACHMENT0); //dest

					glBlitFramebuffer(0, 0, glcaptureInfo.oWidth, glcaptureInfo.oHeight, 0, 0, glcaptureInfo.oWidth, glcaptureInfo.oHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

					glBindTexture(GL_TEXTURE_2D, lastTex2D);
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastFBO);

					wglDXUnlockObjectsNV(gl_d3ddevice, 1, &gl_handle);

					dxgi_swapchain->Present(0, 0);
				}
				else {
					GLuint texture = gltextures[curr_capture];
					DWORD next_capture = (curr_capture == NUM_GLBACKBUFF - 1) ? 0 : (curr_capture + 1);

					glReadBuffer(GL_BACK);
					glBindBuffer(GL_PIXEL_PACK_BUFFER, texture);

					if (glLockedTextures[curr_capture])
					{
						EnterCriticalSection(&data_mutexs[curr_capture]);
						glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
						glLockedTextures[curr_capture] = false;

						LeaveCriticalSection(&data_mutexs[curr_capture]);
					}
					glReadPixels(0, 0, glcaptureInfo.oWidth, glcaptureInfo.oHeight, GL_BGRA, GL_UNSIGNED_BYTE, 0);


					glBindBuffer(GL_PIXEL_PACK_BUFFER, gltextures[next_capture]);
					copy_data = (void*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
					if (copy_data)
					{
						curr_capture = next_capture;
						glLockedTextures[next_capture] = true;


						SetEvent(copy_event);
					}

					glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

					curr_capture = next_capture;
				}
			}
			else if (has_textured)
				ClearGLData();
	}
	else {
		logstream << "DC change new = " << (UINT)hDC << " old = " << (UINT)hdc_acquried << std::endl;
		ClearGLData();
	}
}

static void OpenGLHDCDestroy()
{
	ClearGLData();
	hdc_acquried = NULL;
	target_acquired = false;
}

static BOOL WINAPI SwapBuffersHook(HDC hDC)
{
	OpenGLHDCCapture(hDC);

	hdc_swapbuffers.UnDo();
	BOOL bResult = SwapBuffers(hDC);
	hdc_swapbuffers.ReDo();

	return bResult;
}

static BOOL WINAPI wglSwapLayerBuffersHook(HDC hDC, UINT fuPlanes)
{
	OpenGLHDCCapture(hDC);

	wgl_hdc_swaplayerbuffers.UnDo();
	BOOL bResult = jimglSwapLayerBuffers(hDC, fuPlanes);
	wgl_hdc_swaplayerbuffers.ReDo();

	return bResult;
}

static BOOL WINAPI wglSwapBuffersHook(HDC hDC)
{
	OpenGLHDCCapture(hDC);

	wgl_hdc_swapbuffers.UnDo();
	BOOL bResult = jimglSwapBuffers(hDC);
	wgl_hdc_swapbuffers.ReDo();

	return bResult;
}

static BOOL WINAPI wglDeleteContextHook(HGLRC hRC)
{
	if (hdc_acquried && target_acquired)
	{
		HDC hLastHDC = jimglGetCurrentDC();
		HGLRC hLastHGLRC = jimglGetCurrentContext();

		jimglMakeCurrent(hdc_acquried, hRC);
		OpenGLHDCDestroy();
		jimglMakeCurrent(hLastHDC, hLastHGLRC);
	}

	wgl_delcontext.UnDo();
	BOOL bResult = jimglDeleteContext(hRC);
	wgl_delcontext.ReDo();

	return bResult;
}

static void RegisterNVCapture()
{
	wglDXSetResourceShareHandleNV = (WGLSETRESOURCESHAREHANDLENVPROC)jimglGetProcAddress("wglDXSetResourceShareHandleNV");
	wglDXOpenDeviceNV = (WGLDXOPENDEVICENVPROC)jimglGetProcAddress("wglDXOpenDeviceNV");
	wglDXCloseDeviceNV = (WGLDXCLOSEDEVICENVPROC)jimglGetProcAddress("wglDXCloseDeviceNV");
	wglDXRegisterObjectNV = (WGLDXREGISTEROBJECTNVPROC)jimglGetProcAddress("wglDXRegisterObjectNV");
	wglDXUnregisterObjectNV = (WGLDXUNREGISTEROBJECTNVPROC)jimglGetProcAddress("wglDXUnregisterObjectNV");
	wglDXObjectAccessNV = (WGLDXOBJECTACCESSNVPROC)jimglGetProcAddress("wglDXObjectAccessNV");
	wglDXLockObjectsNV = (WGLDXLOCKOBJECTSNVPROC)jimglGetProcAddress("wglDXLockObjectsNV");
	wglDXUnlockObjectsNV = (WGLDXUNLOCKOBJECTSNVPROC)jimglGetProcAddress("wglDXUnlockObjectsNV");

	nvidia_extend_support = wglDXSetResourceShareHandleNV && wglDXOpenDeviceNV && wglDXCloseDeviceNV &&
		wglDXRegisterObjectNV && wglDXUnregisterObjectNV && wglDXObjectAccessNV &&
		wglDXLockObjectsNV && wglDXUnlockObjectsNV;

	if (nvidia_extend_support)
		logstream << "NV Capture May Be available" <<std::endl;
}

static void RegisterFBO()
{
	glGenFramebuffers = (GLGENFRAMEBUFFERSPROC)jimglGetProcAddress("glGenFramebuffers");
	glDeleteFramebuffers = (GLDELETEFRAMEBUFFERSPROC)jimglGetProcAddress("glDeleteFramebuffers");
	glBindFramebuffer = (GLBINDFRAMEBUFFERPROC)jimglGetProcAddress("glBindFramebuffer");
	glBlitFramebuffer = (GLBLITFRAMEBUFFERPROC)jimglGetProcAddress("glBlitFramebuffer");
	glFramebufferTexture2D = (GLFRAMEBUFFERTEXTURE2DPROC)jimglGetProcAddress("glFramebufferTexture2D");

	bFBOAvailable = glGenFramebuffers && glDeleteFramebuffers && glBindFramebuffer &&
		glBlitFramebuffer && glFramebufferTexture2D;

	if (bFBOAvailable)
		logstream << "FBO Maybe available" <<std::endl;
}

bool h3d::BeginOpenGLCapture()
{
	HMODULE hGL = GetModuleHandleW(L"opengl32.dll");
	if (hGL)
	{
		glReadBuffer = (GLREADBUFFERPROC)GetProcAddress(hGL, "glReadBuffer");
		glDrawBuffer = (GLDRAWBUFFERPROC)GetProcAddress(hGL, "glDrawBuffer");
		glReadPixels = (GLREADPIXELSPROC)GetProcAddress(hGL, "glReadPixels");
		glGetError = (GLGETERRORPROC)GetProcAddress(hGL, "glGetError");
		jimglSwapLayerBuffers = (WGLSWAPLAYERBUFFERSPROC)GetProcAddress(hGL, "wglSwapLayerBuffers");
		jimglSwapBuffers = (WGLSWAPBUFFERSPROC)GetProcAddress(hGL, "wglSwapBuffers");
		jimglDeleteContext = (WGLDELETECONTEXTPROC)GetProcAddress(hGL, "wglDeleteContext");
		jimglGetProcAddress = (WGLGETPROCADDRESSPROC)GetProcAddress(hGL, "wglGetProcAddress");
		jimglMakeCurrent = (WGLMAKECURRENTPROC)GetProcAddress(hGL, "wglMakeCurrent");
		jimglGetCurrentDC = (WGLGETCURRENTDCPROC)GetProcAddress(hGL, "wglGetCurrentDC");
		jimglGetCurrentContext = (WGLGETCURRENTCONTEXTPROC)GetProcAddress(hGL, "wglGetCurrentContext");
		jimglCreateContext = (WGLCREATECONTEXTPROC)GetProcAddress(hGL, "wglCreateContext");

		if (jimglSwapLayerBuffers && jimglSwapBuffers && jimglDeleteContext)
		{
			hdc_swapbuffers.Do((FARPROC)SwapBuffers, (FARPROC)SwapBuffersHook);
			wgl_hdc_swaplayerbuffers.Do((FARPROC)jimglSwapLayerBuffers, (FARPROC)wglSwapLayerBuffersHook);
			wgl_hdc_swapbuffers.Do((FARPROC)jimglSwapBuffers, (FARPROC)wglSwapBuffersHook);
			wgl_delcontext.Do((FARPROC)jimglDeleteContext, (FARPROC)wglDeleteContextHook);
			return true;
		}
	}

	return false;
}

void h3d::EndOpenGLCapture()
{
	hdc_swapbuffers.UnDo();
	wgl_hdc_swaplayerbuffers.UnDo();
	wgl_hdc_swapbuffers.UnDo();
	wgl_delcontext.UnDo();

	ClearGLData();
}

void ClearGLData()
{
	if (copy_info)
		copy_info->Reserved3 = -1;

	if (copy_thread)
	{
		copy_thread_run = false;
		SetEvent(copy_event);
		if (WaitForSingleObject(copy_thread, 500) != WAIT_OBJECT_0)
			TerminateThread(copy_thread, -1);

		CloseHandle(copy_thread);
		CloseHandle(copy_event);

		copy_thread = NULL;
		copy_event = NULL;
	}

	for (int i = 0; i < NUM_GLBACKBUFF; ++i)
	{
		if (glLockedTextures[i])
		{
			EnterCriticalSection(&data_mutexs[i]);

			glBindBuffer(GL_PIXEL_PACK_BUFFER, gltextures[i]);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			glLockedTextures[i] = false;

			LeaveCriticalSection(&data_mutexs[i]);
		}
		DeleteCriticalSection(&data_mutexs[i]);
	}

	if (nvidia_extend_support) {
		if (gl_d3ddevice) {
			if (gl_handle) {
				wglDXUnregisterObjectNV(gl_d3ddevice, gl_handle);
				gl_handle = NULL;
			}

			wglDXCloseDeviceNV(gl_d3ddevice);
			gl_d3ddevice = NULL;
		}

		if (gl_sharedtex) {
			glDeleteTextures(1, &gl_sharedtex);
			gl_sharedtex = 0;
		}

		if (gl_fbo) {
			glDeleteFramebuffers(1, &gl_fbo);
			gl_fbo = 0;
		}

		SR(d3d11_context);
		SR(d3d11_device);
		SR(dxgi_swapchain);
		SR(d3d11_texture);
	}
	else if (has_textured) {
		glDeleteBuffers(NUM_GLBACKBUFF, gltextures);
		ZeroMemory(gltextures, sizeof(gltextures));
	}


	h3d::DestroySharedMem();
	copy_data = NULL;
	copy_info = NULL;
	copy_index = 0;
	curr_capture = 0;
	has_textured = false;
	hdc_acquried = NULL;
	pTexHandle = NULL;

	logstream << "Flush OpenGL Capture" <<std::endl;
}
