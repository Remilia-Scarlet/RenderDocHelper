#pragma once

class RenderDocHelper
{
public:
	static RenderDocHelper* instance();
protected:
	RenderDocHelper() = default;
	~RenderDocHelper();
	bool initRenderDoc();

	void* RenderDocDLL = nullptr;
};