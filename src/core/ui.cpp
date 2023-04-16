#include "ui.h"

#include "../pipeline/camera.h"
#include "../pipeline/material.h"
#include "../gfx/mesh.h"
#include "../gfx/texture.h"
#include "../pipeline/scene.h"
#include "../utils/utils.h"

GFX::Texture* icons = nullptr;

void UI::init()
{
#ifndef SKIP_IMGUI
	ImGuiIO& io = ImGui::GetIO();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	auto& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;// <- Set this on init or use ImGui::PushStyleVar()
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 2.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(.25f, .25f, .25f, 1);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(.2f, .2f, .2f, 1);
	style.Colors[ImGuiCol_Tab] = ImVec4(.15f, .15f, .15f, 1);
	style.Colors[ImGuiCol_TabActive] = ImVec4(.3f, .3f, .3f, 1);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(.15f, .15f, .15f, 1);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(.3f, .3f, .3f, 1);

	icons = GFX::Texture::Get("data/textures/icons.png");
#endif
}

void UI::DrawIcon(int iconx, int icony, float size, float alpha)
{
#ifndef SKIP_IMGUI
	if (!icons)
		return;
	if (size == 0)
		size = 16;
	float aspect = icons->width / icons->height;
	ImVec4 color(1.0f, 1.0f, 1.0f, alpha);
	ImVec2 uv0( iconx / 16.0f, icony / 16.0f);
	ImVec2 uv1(uv0.x + 1.0f / 16.0f, uv0.y + 1.0f / 16.0f);
	icons->bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	ImGui::Image((void*)(intptr_t)icons->texture_id, ImVec2(size / aspect, size), uv0, uv1, color);
#endif
}

bool UI::ButtonIcon(int iconx, int icony, float size, float alpha)
{
	if (!icons)
		return false;
#ifndef SKIP_IMGUI
	if (size == 0)
		size = 16;
	ImVec4 color(1.0f, 1.0f, 1.0f, alpha);
	ImVec2 uv0( iconx / 16.0f, icony / 16.0f);
	ImVec2 uv1(uv0.x + 1.0f / 16.0f, uv0.y + 1.0f / 16.0f);
	float aspect = icons->width / icons->height;
	return ImGui::ImageButton((void*)(intptr_t)icons->texture_id, ImVec2(size / aspect, size), uv0, uv1,-1,ImVec4(),color);
#endif
}

void UI::inspectObject(Matrix44& matrix)
{
#ifndef SKIP_IMGUI
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix.m, matrixTranslation, matrixRotation, matrixScale);
	ImGui::DragFloat3("Position", matrixTranslation, 0.1f);
	ImGui::DragFloat3("Rotation", matrixRotation, 0.1f);
	ImGui::DragFloat3("Scale", matrixScale, 0.1f);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m);
#endif
}

void UI::Layers(const char* text, uint8* layers)
{
#ifndef SKIP_IMGUI
	ImGui::Text(text, *layers);
	for (int i = 0; i < 8; ++i)
	{
		ImGui::SameLine();
		char text[] = "0";
		int bit_index = (7 - i);
		text[0] = '0' + bit_index;
		bool bit = (*layers) & (1 << bit_index);
		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_Button, bit ? ImVec4(0.4, 0.5, 0.4, 1) : ImVec4(0.1, 0.1, 0.1, 1));
		if (ImGui::Button(text))
		{
			if (bit)
				(*layers) &= (~(1 << bit_index));
			else
				(*layers) |= (1 << bit_index);
		}
		ImGui::PopStyleColor(1);
		ImGui::PopID();
	}
#endif
}

bool UI::Filename(const char* text, std::string& filename, std::string base_folder)
{
	bool changed = false;
#ifndef SKIP_IMGUI
	char buff[255];
	strcpy_s(buff, filename.c_str());
	if (ImGui::InputText(text, buff, 255, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		filename = buff;
		changed = true;
	}
	ImGui::SameLine();

	if (ImGui::Button("..."))
	{
		std::string result = CORE::openFileDialog();
		if (!result.empty())
		{
			std::string relpath = makePathRelative(result);
			if (base_folder == relpath.substr(0, base_folder.size()))
			{
				filename = relpath.substr(base_folder.size() + 1);
				changed = true;
			}
			else
				UI::addNotification("File be in the same folder as the scene", UI::eNotificationIconType::ICON_WARNING);
		}
	}
#endif
	return changed;

}

#ifndef SKIP_IMGUI
ImGuizmo::OPERATION UI::manipulate_operation = ImGuizmo::TRANSLATE;
#endif

//example of matrix we want to edit, change this to the matrix of your entity
bool UI::manipulateMatrix(Matrix44& matrix, Camera* camera)
{
#ifndef SKIP_IMGUI

	/*
	if (ImGui::ImageButton( (ImTextureID)Texture::Get("data/textures/brdfLUT.png")->texture_id, ImVec2(100, 100)) )
	{
		std::cout << "foo" << std::endl;
	}
	*/

	//static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
	/*
	if (ImGui::IsKeyPressed(90))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(69))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(82)) // r Key
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	*/
	if (ImGui::RadioButton("Translate", manipulate_operation == ImGuizmo::TRANSLATE))
		manipulate_operation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", manipulate_operation == ImGuizmo::ROTATE))
		manipulate_operation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", manipulate_operation == ImGuizmo::SCALE))
		manipulate_operation = ImGuizmo::SCALE;
	/*
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix.m, matrixTranslation, matrixRotation, matrixScale);
	ImGui::InputFloat3("Tr", matrixTranslation, 3);
	ImGui::InputFloat3("Rt", matrixRotation, 3);
	ImGui::InputFloat3("Sc", matrixScale, 3);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m);
	*/
	if (manipulate_operation != ImGuizmo::SCALE)
	{
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	static bool useSnap(true);
	//if (ImGui::IsKeyPressed(83))
	//	useSnap = !useSnap;
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &useSnap);
	ImGui::SameLine();
	static Vector3f translation_snap(1, 1, 1);
	static float angle_snap = 1;
	static float scale_snap = 0.1;
	float* snap = nullptr;
	ImGui::SameLine();
	switch (manipulate_operation)
	{
	case ImGuizmo::TRANSLATE:
		//snap = config.mSnapTranslation;
		//ImGui::InputFloat3("Snap", &translation_snap.x);
		snap = (float*)&translation_snap;
		break;
	case ImGuizmo::ROTATE:
		//snap = config.mSnapRotation;
		//ImGui::InputFloat("Angle Snap", &angle_snap);
		snap = (float*)&angle_snap;
		break;
	case ImGuizmo::SCALE:
		//snap = config.mSnapScale;
		//ImGui::InputFloat("Scale Snap", &scale_snap);
		snap = (float*)&scale_snap;
		break;
	}

	//draw gizmo in 3D
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	bool changed = ImGuizmo::Manipulate(camera->view_matrix.m, camera->projection_matrix.m, manipulate_operation, mCurrentGizmoMode, matrix.m, NULL, useSnap ? snap : NULL);
	return ImGuizmo::IsUsing();
	/*
	if (ImGuizmo::IsOver)
	{
		std::cout << "using" << std::endl;
	}
	*/

#endif
}


std::vector<UI::sNotification> notifications;
size_t last_notification_index = 0;

void UI::addNotification(std::string text, UI::eNotificationIconType icon, float duration_in_ms )
{
	UI::sNotification n;
	n.content = text;
	n.icon = icon;
	n.time = CORE::getTime() + duration_in_ms;
	n.index = last_notification_index++;
	notifications.push_back(n);
}

#ifndef SKIP_IMGUI

#define NOTIFY_MAX_MSG_LENGTH			4096		// Max message content length
#define NOTIFY_PADDING_X				20.f		// Bottom-left X padding
#define NOTIFY_PADDING_Y				20.f		// Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y		10.f		// Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME			150			// Fade in and out duration
#define NOTIFY_DEFAULT_DISMISS			3000		// Auto dismiss after X ms (default, applied only of no data provided in constructors)
#define NOTIFY_OPACITY					1.0f		// 0-1 Toast opacity
#define NOTIFY_TOAST_FLAGS				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing
#define NOTIFY_USE_SEPARATOR

#endif

void UI::drawNotifications()
{
#ifndef SKIP_IMGUI
	vec2 vp_size = CORE::getWindowSize();
	float opacity = 0.7f;
	float height = 0.0f;
	ImVec4 text_color(1, 0, 0, 1);
	int64 now = CORE::getTime();

	std::vector<UI::sNotification> alive;

	for (size_t i = 0; i < notifications.size(); ++i)
	{
		auto& n = notifications[i];
		if (n.time > now)
			alive.push_back(n);
		float alpha_factor = clamp((n.time - now) / 500.0f, 0.0f, 1.0f);
		if (alpha_factor <= 0.0f)
			continue;

		char window_name[50];
		sprintf_s(window_name, "##NOTIFY%d", n.index);
		ImGui::SetNextWindowBgAlpha(opacity);
		ImGui::SetNextWindowPos(ImVec2(vp_size.x - NOTIFY_PADDING_X + (1-alpha_factor) * 200, vp_size.y - NOTIFY_PADDING_Y - height), ImGuiCond_Always, ImVec2(1.0f, 1.0f));

		ImGui::Begin(window_name, NULL, NOTIFY_TOAST_FLAGS);

		ImGui::PushTextWrapPos(vp_size.x / 3.f); // We want to support multi-line text, this will wrap the text after 1/3 of the screen width

		switch (n.icon)
		{
			case UI::eNotificationIconType::ICON_ERROR: DrawIcon(12, 1); break;
			case UI::eNotificationIconType::ICON_WARNING: DrawIcon(13, 1); break;
			case UI::eNotificationIconType::ICON_QUESTION: DrawIcon(14, 0); break;
			case UI::eNotificationIconType::ICON_INFO:DrawIcon(10, 9); break;
			default: break;
		}
		
		ImGui::SameLine();
		//ImGui::TextColored(text_color);
		ImGui::Text(n.content.c_str());

		height += ImGui::GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;
		ImGui::End();
	}

	notifications = alive;
#endif
}



