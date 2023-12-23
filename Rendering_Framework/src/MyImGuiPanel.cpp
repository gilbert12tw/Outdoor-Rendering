#include "MyImGuiPanel.h"



MyImGuiPanel::MyImGuiPanel()
{
	this->m_avgFPS = 0.0;
	this->m_avgFrameTime = 0.0;
}


MyImGuiPanel::~MyImGuiPanel()
{
}

void MyImGuiPanel::update() {
	// performance information
	const std::string FPS_STR = "FPS: " + std::to_string(this->m_avgFPS);
	ImGui::TextColored(ImVec4(0, 220, 0, 255), FPS_STR.c_str());
	const std::string FT_STR = "Frame: " + std::to_string(this->m_avgFrameTime);
	ImGui::TextColored(ImVec4(0, 220, 0, 255), FT_STR.c_str());

	if (ImGui::Button("Teleport 0")) {
		teleportIdx = 0;
	}
    ImGui::SameLine();
	if (ImGui::Button("Teleport 1")) {
		teleportIdx = 1;
	}
    ImGui::SameLine();
	if (ImGui::Button("Teleport 2")) {
		teleportIdx = 2;
	}

    // normal maping button
    if (ImGui::Button("Normal Mapping")) {
        normalMapping = !normalMapping;
    }
    ImGui::SameLine();

    // 5 type of G buffer
    // 1: world position, 2: world normal, 3: diffuse, 4: specular
    if (ImGui::Button("Original")) {
        gBufferIdx = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Gamma correction")) {
        gBufferIdx = 5;
    }

    if (ImGui::Button("World space vertex")) {
        gBufferIdx = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("World space normal")) {
        gBufferIdx = 2;
    }
    ImGui::SameLine();
    if (ImGui::Button("Diffuse")) {
        gBufferIdx = 3;
    }
    ImGui::SameLine();
    if (ImGui::Button("Specular")) {
        gBufferIdx = 4;
    }
    if (ImGui::Button("Depth 0")) {
        gBufferIdx = 6;
        depthLevel = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 1")) {
        gBufferIdx = 6;
        depthLevel = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 2")) {
        gBufferIdx = 6;
        depthLevel = 2;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 3")) {
        gBufferIdx = 6;
        depthLevel = 3;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 4")) {
        gBufferIdx = 6;
        depthLevel = 4;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 5")) {
        gBufferIdx = 6;
        depthLevel = 5;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 6")) {
        gBufferIdx = 6;
        depthLevel = 6;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 7")) {
        gBufferIdx = 6;
        depthLevel = 7;
    }
    ImGui::SameLine();
    if (ImGui::Button("Depth 8")) {
        gBufferIdx = 6;
        depthLevel = 8;
    }
}

int MyImGuiPanel::getTeleportIdx() {
	int ret = this->teleportIdx;
	this->teleportIdx = -1;
	return ret;
}

void MyImGuiPanel::setAvgFPS(const double avgFPS){
	this->m_avgFPS = avgFPS;
}
void MyImGuiPanel::setAvgFrameTime(const double avgFrameTime){
	this->m_avgFrameTime = avgFrameTime;
}