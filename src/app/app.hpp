#pragma once
#include <cstdint>
#include "helper.hpp"
#include "input.hpp"

struct AppPimpl;

class App
{
private:
	App();

public:
	~App();

public:
	void Init();
	void Shutdown();

	void OnWidowResize(uint32_t width, uint32_t height);

	void Update(float dt);
	void Render();

	double GetGputTime() const { return m_gpuTime; }

private:
	void GBufferPass();
	void RaytraceShadows();
	void LightingPass();
	void SSAOPass();
	void DrawSkybox();
	void FinalHDRPass();

private:
	AppPimpl* m_pApp;
	bool m_isRenderInit;
	double m_gpuTime;

public:
	AppInput Input;

public:
	static App& Get()
	{
		static App app;
		return app;
	}
};