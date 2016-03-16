#ifndef D3D9_MAGIC_SUPPORT
#define D3D9_MAGIC_SUPPORT




namespace h3d {
	bool D3D9PatchTypeSupportSharedTexture(void* module_address);

	//这个代码来源于OBS，待研究 调用这个函数前先调用D3D9PatchTypeSupportSharedTexture
	//D3D9PatchTypeSupportSharedTexture 将会在EndScene调用SharedTextureHandle调用
	void BeginD3D9Patch(void* module_address);
	void EndD3D9Patch(void* module_address);
}

#endif
