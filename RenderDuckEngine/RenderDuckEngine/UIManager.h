#pragma once

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

struct ImGuiParams
{
	ImGuiParams() :
		m_ShowDemoWindow(false)
	{ }

	bool m_ShowDemoWindow;
};

class UIManager
{
public:
	UIManager();
	~UIManager();

	void InitGUI();

	void DrawGUI();

private:

	ImGuiParams m_Params;
};

