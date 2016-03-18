#ifndef VIDEOENGINECAPTUREHOOK_H
#define VIDEOENGINECAPTUREHOOK_H
#include "../Capture/CaptureHUB.h"

namespace h3d {
	typedef int(__stdcall * WINAPIPROC)();
	typedef unsigned char byte;

	WINAPIPROC GetVirtual(void* header_addrss, unsigned offset);

	class APIHook {
		byte data[14];
		/*WINAPIPROC origin_proc;*/
		WINAPIPROC target_proc;
		WINAPIPROC hook_proc;
		
		bool is_hooked;
		bool is_64bitjump;
		bool attempted_bounce;
	public:
		inline APIHook()
			:is_hooked(false),target_proc(0),hook_proc(0),is_64bitjump(false), attempted_bounce(false)
			/*,origin_proc(0)*/
		{
		}

		~APIHook();

		bool Do(WINAPIPROC target, WINAPIPROC hook);

		void ReDo(bool force = false);

		void UnDo();

		/*WINAPIPROC GetOrigin() const {
			return origin_proc;
		}*/

		WINAPIPROC GetTarget() const{
			return target_proc;
		}
	private:
		bool Probe();
	};

	void InitCaptureHook();
	void ExitCaptureHook();

	bool BeginD3D8CaptureHook();
	void EndD3D8CaptureHook();

	bool BeginD3D9CaptureHook();
	void EndD3D9CaptureHook();

	bool BeginDXGICaptureHook();
	void EndDXGICaptureHook();

	bool BeginOpenGLCapture();
	bool EndOpenGLCapture();


#ifdef _WIN64
	typedef unsigned __int64 PTR;
#else
	typedef unsigned long PTR;
#endif

	unsigned int CtorSharedMemCPUCapture(unsigned int texsize, unsigned int& totalsize, MemoryInfo *&share, byte** texaddress);

	unsigned int CtorSharedMemGPUCapture(unsigned __int64** handle);

	void DestroySharedMem();
}




#endif
