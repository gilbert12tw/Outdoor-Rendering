#pragma once

#include <imgui\imgui.h>
#include <imgui\imgui_impl_glfw.h>
#include <imgui\imgui_impl_opengl3.h>

#include <string>

class MyImGuiPanel
{
public:
	MyImGuiPanel();
	virtual ~MyImGuiPanel();

public:
	void update();
	void setAvgFPS(const double avgFPS);
	void setAvgFrameTime(const double avgFrameTime);
	int getTeleportIdx();
    bool getNormalMapping() { return normalMapping; }
    int getGBufferIdx() { return gBufferIdx; }

private:
	double m_avgFPS;
	double m_avgFrameTime;
	int teleportIdx = -1;
    int gBufferIdx = 5;
    bool normalMapping = true;
};