#include <iostream>
#include <glm/glm.hpp>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <fstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <mpg123.h>
#include <ao/ao.h>
#include <memory>

#include <thread>
#include <chrono>

#include <unordered_map>
#include <typeinfo>
#include <typeindex>

void start(int argc, char** argv);

typedef struct
{
    int x, y;
} POSITION2D;

typedef struct
{
    float x, y;
} SCALE2D;

class COLOR3
{
public:
    float x, y, z;

    COLOR3(){}

    COLOR3(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    bool operator==(COLOR3 b) {
        return this->x == b.x && this->y == b.y && this->z == b.z;
    }
};

class COLOR4
{
public:
    int x, y, z, w;

    COLOR4(){}

    COLOR4(int x, int y, int z, int w) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    bool operator==(COLOR4 b) {
        return this->x == b.x && this->y == b.y && this->z == b.z && this->w == b.w;
    }
};

const std::string ascii_chars = "@3|+=-. ";

#define ASCII_CHAR(val) ascii_chars[(val * 7) / 255]

typedef struct
{
    char ch;
    COLOR3 col;
} PIXEL;

typedef struct
{
    PIXEL **map;
} MAP2D;

long long int globalFrameCounter = 0;

void timer(int) {
    globalFrameCounter += 1;
    glutSwapBuffers();
    glutPostRedisplay();
    glutTimerFunc(1000/60, timer, 0);
}

namespace Engine {
    float _WIDTH = 800;
    float _HEIGHT = 600;
    float zoom = 1;

    static void handleReshape(int w, int h) {
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        _WIDTH = w;
        _HEIGHT = h;
    }

    static void initWindow(int argc, char** argv, const char *name, int WIDTH, int HEIGHT) {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE  | GLUT_RGB);
        glutInitWindowSize(WIDTH, HEIGHT);
        _WIDTH = WIDTH;
        _HEIGHT = HEIGHT;
        glutCreateWindow(name);
        glutTimerFunc(0, timer, 1);
        glutReshapeFunc(handleReshape);
    }

    static void runEngine() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glutMainLoop();
    }
};

namespace AudioModule {
    static void AudioPlayerThread(const char *mp3) {
        std::size_t buffer_size, done;
        int driver, err, channels, encoding;
        long rate;
        std::string track;

        mpg123_handle *mh;
        std::shared_ptr<char> buffer;

        ao_sample_format format;
        ao_device *dev;

        ao_initialize();
        driver = ao_default_driver_id();
        mpg123_init();
        mh = mpg123_new(NULL, &err);
        buffer_size = mpg123_outblock(mh);

        buffer = std::shared_ptr<char>(
            new char[buffer_size], 
            std::default_delete<char[]>()
        );

        track = mp3;
        mpg123_open(mh, mp3);
        mpg123_getformat(mh, &rate, &channels, &encoding);

        format.bits = mpg123_encsize(encoding) * 8;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        while(mpg123_read(mh, buffer.get(), buffer_size, &done) == MPG123_OK){
            ao_play(dev, buffer.get(), done);
        }
    }

    static void playAudio(const char *mp3) {
        std::thread audioThread(AudioPlayerThread, mp3);
        audioThread.detach();
    }
};

namespace ImagesModule {
    std::vector<std::vector<PIXEL>> read_image_col(const std::string& file_path, int& width, int& height) {
        int num_channels;

        unsigned char* image_data = stbi_load(file_path.c_str(), &width, &height, &num_channels, 0);

        std::vector<std::vector<PIXEL>> grayscale_image(height, std::vector<PIXEL>(width));

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int pixel_index = (i * width + j) * num_channels;

                int r = image_data[pixel_index];
                int g = image_data[pixel_index + 1];
                int b = image_data[pixel_index + 2];

                int grayscale_value = (r + g + b) / 3;
                grayscale_image[i][j] = {ASCII_CHAR(grayscale_value), COLOR3(r, g, b)};
                
            }
        }

        stbi_image_free(image_data);

        return grayscale_image;
    }

    std::vector<std::vector<int>> read_image(const std::string& file_path, int& width, int& height) {
        int num_channels;

        unsigned char* image_data = stbi_load(file_path.c_str(), &width, &height, &num_channels, 0);

        std::vector<std::vector<int>> grayscale_image(height, std::vector<int>(width, 0));

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int pixel_index = (i * width + j) * num_channels;

                int r = image_data[pixel_index];
                int g = image_data[pixel_index + 1];
                int b = image_data[pixel_index + 2];

                int grayscale_value = (r + g + b) / 3;
                grayscale_image[i][j] = grayscale_value;
            }
        }

        stbi_image_free(image_data);

        return grayscale_image;
    }
};

void (*GLOBAL_keyboardFunction)(unsigned char, int, int);
void (*GLOBAL_drawingFunction)();

void renderFunc() {
    glViewport (0, 0, Engine::_WIDTH, Engine::_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 0, 0, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef( Engine::zoom, Engine::zoom, Engine::zoom );

    GLOBAL_drawingFunction();
}

void keyboardFunc(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(0);
    }

    GLOBAL_keyboardFunction(key, x, y);
}

namespace InputModule {
    static void setKeyboardEvent(void (*keyboardFunction)(unsigned char, int, int)) {
        GLOBAL_keyboardFunction = keyboardFunction;
        glutKeyboardFunc(keyboardFunc);
    }
};

namespace DrawingModule {
    static void initDrawing() {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    static void flush() {
        glutSwapBuffers();
    }

    static void setDrawingEvent(void (*drawingFunction)()) {
        GLOBAL_drawingFunction = drawingFunction;
        glutDisplayFunc(renderFunc);
    }
};

class BaseComponent {
public:
    virtual ~BaseComponent() {}
};

class Entity {
private:
    std::unordered_map<std::type_index, BaseComponent*> components;

public:
    template<typename T>
    T* AddComponent() {
        T* component = new T();
        components[typeid(T)] = component;
        component->SetEntity(this);
        return component;
    }

    template<typename T>
    void RemoveComponent() {
        auto iter = components.find(typeid(T));
        if (iter != components.end()) {
            delete iter->second;
            components.erase(iter);
        }
    }

    template<typename T>
    T* GetComponent() {
        auto iter = components.find(typeid(T));
        if (iter != components.end()) {
            return dynamic_cast<T*>(iter->second);
        }
        return nullptr;
    }

    Entity();
};

class SpriteBase {
private:
    int width;
    int height;
    COLOR3 color;
    MAP2D map;

public:
    SpriteBase() {
        this->width = 0;
        this->height = 0;
        this->map.map = new PIXEL*[height];
        this->color = {1.f, 1.f, 1.f};
        for (int i = 0; i < height; i++) {
            this->map.map[i] = new PIXEL[width];
        }
    }

    SpriteBase(int width, int height, COLOR3 color) {
        this->width = width;
        this->height = height;
        this->map.map = new PIXEL*[height];
        this->color = color;
        for (int i = 0; i < height; i++) {
            this->map.map[i] = new PIXEL[width];
        }

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                this->map.map[i][j].ch = ' ';
            }
        }
    }

    void fromImage(std::string fname) {
        std::vector<std::vector<PIXEL>> grayscale_image = ImagesModule::read_image_col(fname.c_str(), this->width, this->height);

        this->map.map = new PIXEL*[this->height];
        this->color = color;
        for (int i = 0; i < this->height; i++) {
            this->map.map[i] = new PIXEL[this->width];
        }

        for (int i = 0; i < this->height; i++) {
            for (int j = 0; j < this->width; j++) {
                PIXEL pix = grayscale_image[i][j];
                //char ascii_char = ImagesModule::get_ascii_char(grayscale_value);

                this->map.map[i][j].col.x = pix.col.x;
                this->map.map[i][j].col.y = pix.col.y;
                this->map.map[i][j].col.z = pix.col.z;
                this->map.map[i][j].ch = pix.ch;
            }
        }
    }

    int getWidth() { return this->width; }
    int getHeight() { return this->height; }

    void setWidth(int width) { this->width = width; }
    void setHeight(int height) { this->height = height; }

    COLOR3 *getColor() { return &this->color; }
    MAP2D *getMap() { return &this->map; }
};

class SpriteRenderComponent : public BaseComponent {
private:
    SpriteBase sprite;
    Entity* entity;

public:
    SpriteRenderComponent() {}

    void SetEntity(Entity* entity) {
        this->entity = entity;
    }

    SpriteBase *getSprite() { return &this->sprite; }

    void instantDraw();
};

class AnimationComponent : public BaseComponent {
private:
    std::vector<SpriteBase> frames;
    Entity* entity;
    int counter = 0;
    int delay = 30;

public:
    AnimationComponent() {
        this->frames = std::vector<SpriteBase>();
    }

    int push_frame(SpriteBase frame) {
        this->frames.push_back(frame);
        return this->frames.size() - 1;
    }

    int pop_frame() {
        this->frames.pop_back();
        return this->frames.size() - 1;
    }

    SpriteBase back() {
        if (counter == 0) {
            counter -= 1;
            return this->frames[counter];
        } else {
            return this->frames[this->frames.size() - 1];
        }
    }

    SpriteBase next() {
        counter += 1;
        if ((counter) >= this->frames.size()){
            counter = 0;

            return this->frames[0];
        }
        return this->frames[counter-1];
    }

    int getDelay() { return this->delay; }
    void setDelay(int nd) { this->delay = nd; }

    void playCurrentFrame();

    void SetEntity(Entity* entity) {
        this->entity = entity;
    }
};

class TransformComponent : public BaseComponent {
private:
    POSITION2D position;
    SCALE2D scale;
    Entity* entity;

public:
    TransformComponent() {
        this->position = {0, 0};
        this->scale = {1.f, 1.f};
    }

    TransformComponent(POSITION2D position) {
        this->position = position;
        this->scale = {1.f, 1.f};
    }

    TransformComponent(POSITION2D position, SCALE2D scale) {
        this->position = position;
        this->scale = scale;
    }

    POSITION2D getPos() { return this->position; }
    SCALE2D getScale() { return this->scale; }

    void setPos(POSITION2D newPos) { this->position = newPos; }
    void setScale(SCALE2D newScale) { this->scale = newScale; }

    void SetEntity(Entity* entity) {
        this->entity = entity;
    }
};

Entity::Entity() {
    this->AddComponent<TransformComponent>();
    this->AddComponent<SpriteRenderComponent>();
}

void SpriteRenderComponent::instantDraw() {
    SpriteBase *Sprite = this->getSprite();
    TransformComponent *transform = this->entity->GetComponent<TransformComponent>();
    int height = Sprite->getHeight();
    int width = Sprite->getWidth();
    POSITION2D position = transform->getPos();
    SCALE2D scale = transform->getScale();
    
    float fixX = (Engine::_WIDTH / Engine::_HEIGHT * 1.5f) * scale.x;
    float fixHeight = glutBitmapHeight(GLUT_BITMAP_9_BY_15);
    float fixXY = (Engine::_WIDTH / Engine::_HEIGHT * scale.y) * glutBitmapWidth(GLUT_BITMAP_9_BY_15, Sprite->getMap()->map[0][0].ch);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            glColor3ub(Sprite->getMap()->map[i][j].col.x, Sprite->getMap()->map[i][j].col.y, Sprite->getMap()->map[i][j].col.z);
            glRasterPos2f(-1.f + (position.x + j / fixX) * 0.03f, 1.f - (position.y + (i+1) * fixXY / fixHeight) * 0.04f);
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, Sprite->getMap()->map[i][j].ch);
        }
    }
}

void AnimationComponent::playCurrentFrame() {
    SpriteBase curSprite = this->frames[counter];
    TransformComponent *transform = this->entity->GetComponent<TransformComponent>();
    if (globalFrameCounter % delay == 0) {
        curSprite = this->next();
    }

    int height = curSprite.getHeight();
    int width = curSprite.getWidth();
    POSITION2D position = transform->getPos();
    SCALE2D scale = transform->getScale();
    float fixX = (Engine::_WIDTH / Engine::_HEIGHT * 1.5f) * scale.x;
    float fixHeight = glutBitmapHeight(GLUT_BITMAP_9_BY_15);
    float fixXY = (Engine::_WIDTH / Engine::_HEIGHT * scale.y) * glutBitmapWidth(GLUT_BITMAP_9_BY_15, curSprite.getMap()->map[0][0].ch);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            glColor3ub(curSprite.getMap()->map[i][j].col.x, curSprite.getMap()->map[i][j].col.y, curSprite.getMap()->map[i][j].col.z);
            glRasterPos2f(-1.f + (position.x + j / fixX) * 0.03f, 1.f - (position.y + (i+1) * fixXY / fixHeight) * 0.04f);
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, curSprite.getMap()->map[i][j].ch);
        }
    }
}

int main(int argc, char** argv) {
    start(argc, argv);
    return 0;
}
