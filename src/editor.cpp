
#include "litengine.h"
#include "editor.h"

long mouse_press_time = 0;

SceneEditor::SceneEditor( SCN::Scene* scene, SCN::Renderer* renderer )
{
	visible = true;
	this->scene = scene;
	this->renderer = renderer;
	camera = nullptr;
	sidebar_width = 300;
	show_textures = false;
}

void SceneEditor::renderDebug(Camera* camera)
{
	vec2 size = CORE::getWindowSize();
	vec2 mouse = Input::mouse_position;
	for (auto ent : scene->entities)
	{
		bool hover = SCN::BaseEntity::s_selected == ent;
		GFX::drawText3D(ent->root.model.getTranslation(), ent->name.c_str(), hover ? vec4(1, 1, 1, .5) : vec4(.75, .75, .75, .5), 1);
	}

	//in case you want to draw something for debug
	//...
}

void SceneEditor::render(Camera* camera)
{
	if (!scene)
		return;

	//render scene gizmos
	renderDebug(camera);

#ifndef SKIP_IMGUI //to block this code from compiling if we want

	Vector2f window_size = CORE::getWindowSize();
	this->camera = camera;

	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N"))
			{
				scene->clear();
			}
			if (ImGui::MenuItem("Load", "Ctrl+L"))
			{
				std::string result = CORE::openFileDialog();
				if (result.size())
				{
					if( scene->base_folder == cleanPath(result.substr(0, scene->base_folder.size())) )
						scene->load(result.c_str());
					else
					{
					}
				}
			}
			if (ImGui::MenuItem("Reload", "F6"))
			{
				scene->load(scene->filename.c_str());
			}
			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				scene->save(scene->filename.c_str());
			}
			//ImGui::MenuItem("Save as" );
			//ImGui::Separator();
			//ImGui::MenuItem("Options");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit", false))
		{
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add"))
		{
			for (auto it = SCN::BaseEntity::s_factory.begin(); it != SCN::BaseEntity::s_factory.end(); ++it)
			{
				if (ImGui::MenuItem(it->first.c_str()))
				{
					SCN::BaseEntity* ent = it->second->clone();
					ent->name = "entity";
					scene->addEntity(ent);
					SCN::BaseEntity::s_selected = ent;
				}
			}
			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Textures", "F4", &show_textures);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();

	}

	ImGui::PopStyleVar();

	//top bar
	//example of matrix we want to edit, change this to the matrix of your entity
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 800.f,600.f });
	ImGui::SetNextWindowPos(ImVec2(sidebar_width, 18));
	ImGui::SetNextWindowSize(ImVec2(window_size.x - sidebar_width, 32));

	if (ImGui::Begin("Tools", nullptr, flags))// Create a window
	{
		if (SCN::BaseEntity::s_selected)
		{
			static bool was_used = false;
			bool used = UI::manipulateMatrix(SCN::BaseEntity::s_selected->root.model, camera);
			if (!was_used && used)
				saveUndo();
			was_used = used;
		}
	}
	ImGui::End();
	//return;

	ImGui::SetNextWindowPos(ImVec2(sidebar_width, 48));
	ImGui::SetNextWindowSize(ImVec2(window_size.x - sidebar_width, 30));
	if (ImGui::Begin("Stats", nullptr, flags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs))// Create a window
		ImGui::Text(GFX::getGPUStats().c_str());					   // Display some text (you can use a format strings too)
	ImGui::End();

	int current_y = 18;
	int remaining_y = window_size.y - current_y;

	//sidebar ********************
	ImGui::SetNextWindowPos(ImVec2(0, current_y));
	ImGui::SetNextWindowSize(ImVec2(sidebar_width, remaining_y / 2));
	ImGui::Begin("Sidebar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

	remaining_y -= remaining_y / 2;

	ImGui::BeginTabBar("#Additional Parameters");
	if (ImGui::BeginTabItem("Entities"))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

		ImGui::BeginChild("entities");

		//root
		if (!SCN::BaseEntity::s_selected)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.0f));
		UI::DrawIcon(9, 0);
		ImGui::SameLine();
		ImGui::Text("Scene");
		if (!SCN::BaseEntity::s_selected)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked(0))
		{
			SCN::BaseEntity::s_selected = nullptr;
			SCN::Node::s_selected = nullptr;
		}

		//list of nodes
		for (size_t i = 0; i < scene->entities.size(); ++i)
		{
			SCN::BaseEntity* entity = scene->entities[i];
			switch (entity->getType())
			{
				case SCN::eEntityType::PREFAB: renderInList((SCN::PrefabEntity*)entity); break;
				default: renderInList(entity); break;
			}
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();

		ImGui::EndChild();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Rendering"))
	{
		renderer->showUI();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Project"))
	{
		char filename_buff[255];
		strcpy(filename_buff, scene->filename.c_str());
		if (ImGui::InputText("Filename", filename_buff, 255))
			scene->filename = filename_buff;

		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Stats"))
	{
	}


	ImGui::EndTabBar();
	ImGui::End();

	//inspector
	ImGui::SetNextWindowPos(ImVec2((float)0, (float)(window_size.y - remaining_y)));
	ImGui::SetNextWindowSize(ImVec2((float)sidebar_width, (float)remaining_y));

	ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));


	if (SCN::Node::s_selected)
	{
		inspectObject(SCN::Node::s_selected);
	}
	else if (SCN::BaseEntity::s_selected)
	{
		SCN::BaseEntity* ent = SCN::BaseEntity::s_selected;
		switch (ent->getType())
		{
		case SCN::eEntityType::PREFAB: inspectEntity((SCN::PrefabEntity*)ent); break;
		case SCN::eEntityType::LIGHT: inspectEntity((SCN::LightEntity*)ent); break;
		case SCN::eEntityType::NONE: inspectEntity((SCN::UnknownEntity*)ent); break;
		default: inspectEntity(ent); break;
		}
	}
	else
	{
		ImGui::ColorEdit3("BG color", scene->background_color.v);
		ImGui::ColorEdit3("Ambient Light", scene->ambient_light.v);

		if (UI::Filename("Skybox", scene->skybox_filename, scene->base_folder))
			renderer->setupScene(camera);

		//add info to the debug panel about the camera
		if (ImGui::TreeNode(camera, "Camera")) {
			inspectObject(camera);
			ImGui::TreePop();
		}
	}

	ImGui::PopStyleColor();
	ImGui::End();

	// Always center this window when appearing
	/*
	ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!\n\n");
		ImGui::Separator();
		//static int unused_i = 0;
		//ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");
		static bool dont_ask_me_next_time = false;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
		ImGui::PopStyleVar();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	*/
#endif
	if(show_textures)
		renderTexturesPanel();
}

void SceneEditor::renderTexturesPanel()
{
	#ifndef SKIP_IMGUI
	vec2 window_size = CORE::getWindowSize();
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	ImGui::SetNextWindowPos(ImVec2(sidebar_width, 40));
	ImGui::SetNextWindowSize(ImVec2(window_size.x - sidebar_width, window_size.y - 40));
	static bool show_big = false;
	static int selected_texture = -1;

	if (ImGui::Begin("Textures", nullptr, flags))// Create a window
	{
		ImGui::Checkbox("Big", &show_big);
		for (auto it : GFX::Texture::sTextures)
		{
			GFX::Texture* tex = it.second;
			if (!tex || !tex->width)
				continue;
			int s = show_big ? 512 : 256;
			if (selected_texture == tex->index)
				s = window_size.x - sidebar_width - 10;

			ImGui::Image((ImTextureID)tex->texture_id, ImVec2(s,s));
			if (ImGui::IsItemClicked(0))
				selected_texture = selected_texture == tex->index ? -1 : tex->index;
			ImGui::Text("%dx%d %s", (int)tex->width, (int)tex->height, tex->filename.c_str());
		}
	}
	ImGui::End();
#endif
}

void SceneEditor::inspectEntity(SCN::BaseEntity* entity)
{
#ifndef SKIP_IMGUI
	char buff[1024];
	strcpy(buff, entity->name.c_str());
	//ImGui::Text("Name: %s", name.c_str()); 
	if (ImGui::InputText("Name", buff, 1024))
		entity->name = buff;
	ImGui::Text("Type: %s", entity->getTypeAsStr());
	ImGui::Checkbox("Visible", &entity->visible);
	UI::Layers("Layers", &entity->layers);

	UI::inspectObject(entity->root.model);//Model edit
#endif
}

void SceneEditor::inspectEntity(SCN::PrefabEntity* entity)
{
#ifndef SKIP_IMGUI
	this->inspectEntity((SCN::BaseEntity*)entity);

	ImGui::Separator();

	if (UI::Filename("filename", entity->filename, scene->base_folder))
	{
		entity->loadPrefab(entity->filename.c_str());
	}

#endif
}

void SceneEditor::inspectEntity(SCN::LightEntity* entity)
{
#ifndef SKIP_IMGUI
	this->inspectEntity((SCN::BaseEntity*)entity);

	int light_type = (int)entity->light_type;
	ImGui::Combo("light_type", &light_type, "UNKNOWN\0POINT\0SPOT\0DIRECTIONAL", 4);
	entity->light_type = (SCN::eLightType)(light_type);

	ImGui::ColorEdit3("color", entity->color.v);
	ImGui::SliderFloat("intensity", &entity->intensity, 0, 10);
	ImGui::DragFloat("near_distance", &entity->near_distance, 0.1, -10000.0f, 10000.0f);
	ImGui::DragFloat("max_distance", &entity->max_distance, 1.0f, 0.0f,10000.0f);

	if (light_type == SCN::eLightType::SPOT)
	{
		ImGui::SliderFloat("cone_start", &entity->cone_info.x, 0, 180);
		ImGui::SliderFloat("cone_end", &entity->cone_info.y, 0, 180);
	}
	if (light_type == SCN::eLightType::DIRECTIONAL)
		ImGui::DragFloat("area", &entity->area);

	ImGui::Checkbox("cast_shadows", &entity->cast_shadows);
	if (entity->cast_shadows)
	{
		ImGui::DragFloat("shadow_bias", &entity->shadow_bias, 0.001, 0.0f, 0.1f);
	}
#endif
}


void SceneEditor::inspectEntity( SCN::UnknownEntity* entity )
{
#ifndef SKIP_IMGUI
	this->inspectEntity((SCN::BaseEntity*)entity);
	if (!entity->data)
		return;
	ImGui::Separator();

	cJSON* item_json;
	cJSON_ArrayForEach(item_json, (cJSON*)entity->data)
	{
		ImGui::Text(" - %s", item_json->string);
	}
#endif
}

void SceneEditor::renderInList(SCN::BaseEntity* entity)
{
#ifndef SKIP_IMGUI
	ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);
	if(!entity->visible)
		color = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
	bool selected = SCN::BaseEntity::s_selected == entity;
	if (selected)
		color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::Text(" -");
	ImGui::SameLine();
	vec2 icon = entity->getTypeIcon();
	UI::DrawIcon(icon.x, icon.y, 0, selected ? 1.0 : 0.5f);
	ImGui::SameLine();
	ImGui::Text("%s", entity->name.c_str());

	if (ImGui::IsItemClicked(0)) //ImGui::IsMouseDoubleClicked(0)
	{
		SCN::BaseEntity::s_selected = entity;
		SCN::Node::s_selected = nullptr;
	}

	ImGui::PopStyleColor();
#endif
}

void SceneEditor::renderInList(SCN::PrefabEntity* entity)
{
#ifndef SKIP_IMGUI
	bool selected = SCN::BaseEntity::s_selected == entity;
	if(selected)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.0f));

	bool open = ImGui::TreeNodeEx(entity, ImGuiTreeNodeFlags_OpenOnArrow | (selected ? ImGuiTreeNodeFlags_Selected : 0), "");
	ImGui::SameLine();
	UI::DrawIcon(11, 0, 0, selected ? 1.0 : 0.5f);
	ImGui::SameLine();
	ImGui::Text("%s", entity->name.c_str());
	if (selected)
		ImGui::PopStyleColor();
	if (ImGui::IsItemClicked(0))
	{
		SCN::BaseEntity::s_selected = entity;
		SCN::Node::s_selected = nullptr;
	}

	if (open)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.65f, 0.8f, 1.0f));
		renderNodesInList(&entity->root);
		ImGui::PopStyleColor();
		ImGui::TreePop();
	}
#endif
}

void SceneEditor::renderNodesInList(SCN::Node* node)
{
#ifndef SKIP_IMGUI
	const char* nodename = node->name.size() > 0 ? node->name.c_str() : "unnamed";
	if (!node->children.size())
	{
		if (SCN::Node::s_selected == node)
			ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.f, 1.f), "%s", nodename);
		else
			ImGui::Text("%s", nodename);
		if (ImGui::IsItemClicked(0))
		{
			SCN::Node::s_selected = node;
		}
		return;
	}

	if (SCN::Node::s_selected == node)
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 1.f, 1.f));
	bool open = ImGui::TreeNodeEx(node, ImGuiTreeNodeFlags_OpenOnArrow, nodename);
	if (SCN::Node::s_selected == node)
		ImGui::PopStyleColor();

	if (ImGui::IsItemClicked(0))
		SCN::Node::s_selected = node;

	if (open)
	{
		if (node->children.size() > 0)
			for (auto& child : node->children)
				renderNodesInList(child);
		ImGui::TreePop();
	}
#endif
}

void SceneEditor::inspectObject(SCN::Node* node)
{
#ifndef SKIP_IMGUI
	ImGui::Text("Node Name: %s", node->name.size() > 0 ? node->name.c_str() : "unnamed");
	if (node->mesh)
		ImGui::Text("Mesh: %s", node->mesh->name.c_str());

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

	//Model edit
	UI::inspectObject(node->model);

	//Material
	if (node->material && ImGui::TreeNode(node->material, "Material"))
	{
		inspectObject(node->material);
		ImGui::TreePop();
	}

	ImGui::PopStyleColor();

	if (node->parent)
	{
		//ImGui::TextColored
		ImGui::Text("Parent: %s", node->parent->name.c_str());
		if (ImGui::IsItemClicked(0))
		{
			SCN::BaseEntity::s_selected = nullptr;
			SCN::Node::s_selected = node->parent;
		}
	}

	if (node->children.size() > 0)
	{
		if (ImGui::TreeNode(&node->children[0],"Children"))
		{
			for (int i = 0; i < node->children.size(); ++i)
			{
				ImGui::Text("- %s", node->children[i]->name.c_str());
				if (ImGui::IsItemClicked(0))
				{
					SCN::BaseEntity::s_selected = nullptr;
					SCN::Node::s_selected = node->children[i];
				}
			}
			ImGui::TreePop();
		}
	}
#endif
}

void SceneEditor::inspectObject(Camera* camera)
{
#ifndef SKIP_IMGUI
	bool changed = false;
	changed |= ImGui::Combo("Type", (int*)&camera->type, "PERSPECTIVE\0ORTHOGRAPHIC", 2);
	if (changed && camera->type == Camera::ORTHOGRAPHIC)
		camera->setOrthographic(-200, 200, -200 / camera->aspect, 200 / camera->aspect, 0.1, 10000);
	changed |= ImGui::SliderFloat3("Eye", &camera->eye.x, -100, 100);
	changed |= ImGui::SliderFloat3("Center", &camera->center.x, -100, 100);
	changed |= ImGui::SliderFloat3("Up", &camera->up.x, -100, 100);
	changed |= ImGui::SliderFloat("FOV", &camera->fov, 15, 180);
	changed |= ImGui::SliderFloat("Near", &camera->near_plane, 0.01, camera->far_plane);
	changed |= ImGui::SliderFloat("Far", &camera->far_plane, camera->near_plane, 10000);
	if (changed)
		camera->lookAt(camera->eye, camera->center, camera->up);
#endif
}

void SceneEditor::inspectObject(SCN::Material* material)
{
#ifndef SKIP_IMGUI
	ImGui::Text("Name: %s", material->name.c_str()); // Show String
	ImGui::Checkbox("Two sided", &material->two_sided);
	ImGui::Combo("AlphaMode", (int*)&material->alpha_mode, "NO_ALPHA\0MASK\0BLEND", 3);
	ImGui::SliderFloat("Alpha Cutoff", &material->alpha_cutoff, 0.0f, 1.0f);
	ImGui::ColorEdit4("Color", material->color.v); // Edit 4 floats representing a color + alpha
	ImGui::ColorEdit3("Emissive", material->emissive_factor.v);
	for (size_t i = 0; i < SCN::eTextureChannel::ALL; ++i)
	{
		if (material->textures[i].texture && ImGui::TreeNode( &material->textures[i], SCN::texture_channel_str[i] ))
		{
			int w = ImGui::GetColumnWidth();
			float aspect = material->textures[i].texture->width / (float)material->textures[i].texture->height;
			ImGui::Image((void*)(intptr_t)(material->textures[i].texture->texture_id), ImVec2(w, w * aspect));
			ImGui::TreePop();
		}
	}
#endif
}

std::vector<std::string> undo_history;
void SceneEditor::saveUndo()
{
	if (!scene)
		return;
	//return;
	std::cout << "saving undo" << std::endl;
	std::string data;
	scene->toString(data);
	if (undo_history.size() > 100)
		undo_history.pop_front();
	undo_history.push_back(data);
}

void SceneEditor::doUndo()
{
	if (!scene)
		return;
	if (undo_history.size() == 0)
		return;
	std::cout << "doing undo" << std::endl;
	std::string selected_name = "";
	if (SCN::BaseEntity::s_selected)
		selected_name = SCN::BaseEntity::s_selected->name;
	std::string step = undo_history.back();
	undo_history.pop_back();

	scene->fromString(step, scene->base_folder.c_str());
	SCN::BaseEntity::s_selected = scene->getEntity(selected_name);
	UI::addNotification("Undo done");
}

void SceneEditor::clearUndo()
{
	undo_history.clear();
}


void SceneEditor::deleteSelection()
{
	if (!SCN::BaseEntity::s_selected)
		return;
	saveUndo();
	scene->removeEntity(SCN::BaseEntity::s_selected);
	delete SCN::BaseEntity::s_selected;
	SCN::BaseEntity::s_selected = nullptr;
}

//Keyboard event handler (sync input)
bool SceneEditor::onKeyDown(SDL_KeyboardEvent event)
{
#ifndef SKIP_IMGUI
	if (!scene)
		return false;

	switch (event.keysym.sym)
	{
	case SDLK_f:
		if (SCN::BaseEntity::s_selected && camera)
			camera->lookAt(camera->eye, SCN::BaseEntity::s_selected->root.model.getTranslation(), Vector3f(0, 1, 0));
		break;
	case SDLK_l:
		if (event.keysym.mod & KMOD_CTRL)
		{
			scene->load(scene->filename.c_str());
			clearUndo();
		}
		break;
	case SDLK_s:
		if (event.keysym.mod & KMOD_CTRL)
			scene->save(scene->filename.c_str());
		break;
	case SDLK_d:
		if (event.keysym.mod & KMOD_CTRL && SCN::BaseEntity::s_selected)
		{
			SCN::BaseEntity::s_selected = SCN::BaseEntity::s_selected->clone();
			scene->addEntity(SCN::BaseEntity::s_selected);
		}
		break;
	case SDLK_c:
		if (event.keysym.mod & KMOD_CTRL)
		{
			if (clipboard)
				delete clipboard;
			clipboard = SCN::BaseEntity::s_selected->clone();
		}
		break;
	case SDLK_v:
		if (event.keysym.mod & KMOD_CTRL)
		{
			if (clipboard)
			{
				saveUndo();
				scene->addEntity(clipboard);
				SCN::BaseEntity::s_selected = clipboard;
				clipboard = nullptr;
			}
		}
		break;
	case SDLK_z:
		if (event.keysym.mod & KMOD_CTRL)
			doUndo();
		break;
	case SDLK_1: UI::manipulate_operation = ImGuizmo::TRANSLATE; break;
	case SDLK_2: UI::manipulate_operation = ImGuizmo::ROTATE; break;
	case SDLK_3: UI::manipulate_operation = ImGuizmo::SCALE; break;
	case SDLK_DELETE: deleteSelection(); break; //ESC key, kill the app
	case SDLK_F4: show_textures = !show_textures;break;
	case SDLK_F6: //refresh
		scene->clear();
		scene->load(scene->filename.c_str());
		camera->lookAt(scene->main_camera.eye, scene->main_camera.center, Vector3f(0, 1, 0));
		camera->fov = scene->main_camera.fov;
		break;
	default:
		return false;
	}
#endif
	return false;
}

void SceneEditor::onMouseButtonDown(SDL_MouseButtonEvent event)
{
	mouse_press_time = event.timestamp;
}

void SceneEditor::onMouseButtonUp(SDL_MouseButtonEvent event)
{
	//return;
	uint32 click_time = event.timestamp - mouse_press_time;
	Camera* camera = Camera::current;
	Vector2f window_size = CORE::getWindowSize();

	if (click_time < 200)
	{
		//first check names
		for (auto ent : scene->entities)
		{
			Vector3f pos = camera->project(ent->root.model.getTranslation(), window_size.x, window_size.y, true);
			if (pos.z < 1 && event.x > pos.x - 5 && event.x < pos.x + 5 && event.y > pos.y - 5 && event.y < pos.y + 5)
			{
				SCN::BaseEntity::s_selected = ent;
				SCN::Node::s_selected = nullptr;
				return;
			}
		}

		//otherwise check meshes
		Ray ray;
		ray.origin = camera->eye;
		ray.direction = camera->getRayDirection(event.x, event.y, window_size.x, window_size.y);
		auto result = scene->testRay(ray, 0xFF);
		if (result.entity)
		{
			SCN::BaseEntity::s_selected = result.entity;
			SCN::Node::s_selected = nullptr;
			//std::cout << result.collision << std::endl;
		}
		else
		{
			SCN::BaseEntity::s_selected = nullptr;
			SCN::Node::s_selected = nullptr;
		}

	}
}

void SceneEditor::onFileDrop(std::string filename, std::string relative, SDL_Event event)
{
	if (filename == relative)
	{
		std::cout << "FILE must be in data folder" << std::endl;
		return;
	}
	if (relative.find("data/") == -1)
	{
		std::cout << "FILE must be in data folder" << std::endl;
		return;
	}

	if (relative.find(".glb") != std::string::npos || relative.find(".gltf") != std::string::npos)
		addPrefab(relative.substr(5).c_str());
	else if (relative.find(".json") != std::string::npos)
		scene->load(relative.c_str());
	else
		std::cout << "Unknown file" << std::endl;
}


void SceneEditor::addPrefab(const char* filename)
{
	std::cout << "Adding prefab: " << filename << std::endl;
	SCN::PrefabEntity* ent = new SCN::PrefabEntity();
	scene->addEntity(ent);
	ent->name = filename;
	ent->loadPrefab(filename);
	SCN::BaseEntity::s_selected = ent;
	SCN::Node::s_selected = nullptr;
}

