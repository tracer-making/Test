#include "core/App.h"
#include "states/MainMenuState.h"
#include <memory>

int main(int, char**) {
	App app;
	if (!app.init("溯洄遗梦 - 主菜单", 1280, 720)) return 1;
	app.setState(std::unique_ptr<State>(static_cast<State*>(new MainMenuState())));
	app.run();
	return 0;
}


