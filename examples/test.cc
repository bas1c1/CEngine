#include "main.hpp"

int gx, gy = 0;

void keyboard(unsigned char key, int x, int y) {
    if (key == 'w') {
        gy--;
    } else if (key == 's') {
        gy++;
    } else if (key == 'a') {
        gx--;
    } else if (key == 'd') {
        gx++;
    } else if (key == '[') {
        Engine::zoom += 0.005f;
    } else if (key == ']') {
        Engine::zoom -= 0.005f;
    } else if (key == 'x') {
        AudioModule::playAudio("a.mp3");
    }
}

Entity entity = Entity();
TransformComponent *transform = entity.GetComponent<TransformComponent>();
SpriteRenderComponent *render = entity.GetComponent<SpriteRenderComponent>();
AnimationComponent *animation = entity.AddComponent<AnimationComponent>();

void update() {
    DrawingModule::initDrawing();

    transform->setPos({gx, gy});
    animation->playCurrentFrame();
}

void start(int argc, char** argv) {
    SpriteBase spr1, spr2, spr3;
    spr1.fromImage("a.png");
    spr2.fromImage("b.png");
    spr3.fromImage("c.png");
    animation->push_frame(spr1);
    animation->push_frame(spr2);
    animation->push_frame(spr3);

    Engine::initWindow(argc, argv, "Lol", 800, 600);
    InputModule::setKeyboardEvent(&keyboard);
    DrawingModule::setDrawingEvent(&update);
    Engine::runEngine();
}