
#include <deque>


class Application;
class Camera;
namespace SCN {
	class Scene;
	class Renderer;

	class PrefabEntity;
	class LightEntity;
};

class SceneEditor
{
public:
	bool visible;
	SCN::Scene* scene;
	SCN::Renderer* renderer;
	Camera* camera;
	SCN::BaseEntity* clipboard = nullptr;
	int sidebar_width;
	bool show_textures;

	SceneEditor(SCN::Scene* scene, SCN::Renderer* renderer);

	void render( Camera* camera );
	void renderDebug(Camera* camera);
	void renderTexturesPanel();

	void inspectEntity(SCN::BaseEntity* entity);
	void inspectEntity(SCN::PrefabEntity* entity);
	void inspectEntity(SCN::LightEntity* entity);
	void inspectEntity(SCN::UnknownEntity* entity);

	void renderInList(SCN::BaseEntity* entity);
	void renderInList(SCN::PrefabEntity* entity);
	void renderNodesInList(SCN::Node* node);

	void inspectObject(SCN::Node* node);
	void inspectObject(Camera* camera);
	void inspectObject(SCN::Material* material);

	void addPrefab(const char* filename);
	void deleteSelection();

	//undo
	std::deque<std::string> undo_history;
	void saveUndo();
	void doUndo();
	void clearUndo();


	void onMouseButtonDown(SDL_MouseButtonEvent event);
	void onMouseButtonUp(SDL_MouseButtonEvent event);
	bool onKeyDown(SDL_KeyboardEvent event);
	void onFileDrop(std::string filename, std::string relative, SDL_Event event);
};