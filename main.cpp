#include "engine/engine.h"

int main() {
    auto engine = new Engine();
    engine->init();
    engine->setupDemoData();
	engine->render();
    delete engine;
    return 0;
}


