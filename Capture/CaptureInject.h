#ifndef VIDEOENGINEGDICAPTUREINJECT_H
#define VIDEOENGINEGDICAPTUREINJECT_H

#include <string>
#include "CapturePlugin.h"

namespace h3d {
	//����Ȩ��
	bool H3D_API AdjustToken();

	//ע��
	bool H3D_API InjectDLL(void * hProcess,const std::wstring& dllpath);

	//bool InjectDllBySetWindowHook
}

#endif
