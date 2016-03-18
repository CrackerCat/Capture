#ifndef VIDEOENGINEGDICAPTUREINJECT_H
#define VIDEOENGINEGDICAPTUREINJECT_H

#include <string>


namespace h3d {
	//提升权限
	bool AdjustToken();

	//注入
	bool  InjectDLL(void * hProcess,const std::wstring& dllpath);
}

#endif
