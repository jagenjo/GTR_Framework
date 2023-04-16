#pragma once
#include "core.h"
#include "math.h"

class Camera;
namespace SCN {
	class Node;
	class Material;
}

namespace UI {

	//#undef ERROR
	enum eNotificationIconType {
		ICON_NONE = 0,
		ICON_INFO=1,
		ICON_ERROR=2,
		ICON_WARNING=3,
		ICON_QUESTION=4,
	};
	struct sNotification {
		size_t index;
		eNotificationIconType icon;
		std::string content;
		int64 time;
	};

	void init();

	void DrawIcon(int iconx, int icony, float size = 0,float alpha = 1.0f);
	bool ButtonIcon(int iconx, int icony, float size = 0, float alpha = 1.0f);

	void inspectObject(Matrix44& matrix);

	void Layers(const char* text, uint8* layers);
	bool Filename(const char* text, std::string& filename, std::string base_folder);


#ifndef SKIP_IMGUI
	extern ImGuizmo::OPERATION manipulate_operation;
#endif
	bool manipulateMatrix(Matrix44& matrix, Camera* camera);

	//notifications system
	void addNotification(std::string text, UI::eNotificationIconType icon = UI::eNotificationIconType::ICON_INFO, float duration_in_ms = 3000);
	void drawNotifications();
};