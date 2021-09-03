#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
#include <SDL_ttf.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
//#include <SDL_gpu.h>
//#include <SFML/Network.hpp>
//#include <SFML/Graphics.hpp>
#include <algorithm>
#include <atomic>
#include <codecvt>
#include <functional>
#include <locale>
#include <mutex>
#include <thread>
#ifdef __ANDROID__
#include "vendor/PUGIXML/src/pugixml.hpp"
#include <android/log.h> //__android_log_print(ANDROID_LOG_VERBOSE, "Mincraft", "Example number log: %d", number);
#include <jni.h>
#else
#include <filesystem>
#include <pugixml.hpp>
#ifdef __EMSCRIPTEN__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace std::chrono_literals;
#endif

// NOTE: Remember to uncomment it on every release
//#define RELEASE

#if defined _MSC_VER && defined RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

//240 x 240 (smart watch)
//240 x 320 (QVGA)
//360 x 640 (Galaxy S5)
//640 x 480 (480i - Smallest PC monitor)

int windowWidth = 240;
int windowHeight = 320;
SDL_Point mousePos;
SDL_Point realMousePos;
bool keys[SDL_NUM_SCANCODES];
bool buttons[SDL_BUTTON_X2 + 1];
SDL_Renderer* renderer;

#define PLAYER_SPEED 1

void logOutputCallback(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
    std::cout << message << std::endl;
}

int random(int min, int max)
{
    return min + rand() % ((max + 1) - min);
}

int SDL_QueryTextureF(SDL_Texture* texture, Uint32* format, int* access, float* w, float* h)
{
    int wi, hi;
    int result = SDL_QueryTexture(texture, format, access, &wi, &hi);
    if (w) {
        *w = wi;
    }
    if (h) {
        *h = hi;
    }
    return result;
}

SDL_bool SDL_PointInFRect(const SDL_Point* p, const SDL_FRect* r)
{
    return ((p->x >= r->x) && (p->x < (r->x + r->w)) && (p->y >= r->y) && (p->y < (r->y + r->h))) ? SDL_TRUE : SDL_FALSE;
}

std::ostream& operator<<(std::ostream& os, SDL_FRect r)
{
    os << r.x << " " << r.y << " " << r.w << " " << r.h;
    return os;
}

std::ostream& operator<<(std::ostream& os, SDL_Rect r)
{
    SDL_FRect fR;
    fR.w = r.w;
    fR.h = r.h;
    fR.x = r.x;
    fR.y = r.y;
    os << fR;
    return os;
}

SDL_Texture* renderText(SDL_Texture* previousTexture, TTF_Font* font, SDL_Renderer* renderer, const std::string& text, SDL_Color color)
{
    if (previousTexture) {
        SDL_DestroyTexture(previousTexture);
    }
    SDL_Surface* surface;
    if (text.empty()) {
        surface = TTF_RenderUTF8_Blended(font, " ", color);
    }
    else {
        surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    }
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }
    else {
        return 0;
    }
}

struct Text {
    std::string text;
    SDL_Surface* surface = 0;
    SDL_Texture* t = 0;
    SDL_FRect dstR{};
    bool autoAdjustW = false;
    bool autoAdjustH = false;
    float wMultiplier = 1;
    float hMultiplier = 1;

    ~Text()
    {
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
    }

    Text()
    {
    }

    Text(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
    }

    Text& operator=(const Text& rightText)
    {
        text = rightText.text;
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (rightText.surface) {
            surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
        }
        if (rightText.t) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
        dstR = rightText.dstR;
        autoAdjustW = rightText.autoAdjustW;
        autoAdjustH = rightText.autoAdjustH;
        wMultiplier = rightText.wMultiplier;
        hMultiplier = rightText.hMultiplier;
        return *this;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, std::string text, SDL_Color c = { 255, 255, 255 })
    {
        this->text = text;
#if 1 // NOTE: renderText
        if (surface) {
            SDL_FreeSurface(surface);
        }
        if (t) {
            SDL_DestroyTexture(t);
        }
        if (text.empty()) {
            surface = TTF_RenderUTF8_Blended(font, " ", c);
        }
        else {
            surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
        }
        if (surface) {
            t = SDL_CreateTextureFromSurface(renderer, surface);
        }
#endif
        if (autoAdjustW) {
            SDL_QueryTextureF(t, 0, 0, &dstR.w, 0);
        }
        if (autoAdjustH) {
            SDL_QueryTextureF(t, 0, 0, 0, &dstR.h);
        }
        dstR.w *= wMultiplier;
        dstR.h *= hMultiplier;
    }

    void setText(SDL_Renderer* renderer, TTF_Font* font, int value, SDL_Color c = { 255, 255, 255 })
    {
        setText(renderer, font, std::to_string(value), c);
    }

    void draw(SDL_Renderer* renderer)
    {
        if (t) {
            SDL_RenderCopyF(renderer, t, 0, &dstR);
        }
    }
};

int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
        status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
        status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
        status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius - 1;
    status = 0;

    while (offsety >= offsetx) {

        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
            x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
            x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
            x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
            x + offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2 * offsetx) {
            d -= 2 * offsetx + 1;
            offsetx += 1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

int eventWatch(void* userdata, SDL_Event* event)
{
    // WARNING: Be very careful of what you do in the function, as it may run in a different thread
    if (event->type == SDL_APP_TERMINATING || event->type == SDL_APP_WILLENTERBACKGROUND) {
    }
    return 0;
}

std::string mapStr =
#include "map.h"
    ;

struct Tile {
    SDL_FRect dstR{};
    SDL_Rect srcR{};
    std::string source;
};

SDL_Texture* getT(SDL_Renderer* renderer, std::string name)
{
    static std::unordered_map<std::string, SDL_Texture*> map;
    auto it = map.find(name);
    if (it == map.end()) {
        SDL_Texture* t = IMG_LoadTexture(renderer, ("res/" + name).c_str());
        map.insert({ name, t });
        return t;
    }
    else {
        return it->second;
    }
}

enum Movement {
    Up,
    Down,
    Left,
    Right,
};

bool running = true;
SDL_Rect playerR;
Movement movement = Movement::Right;
std::vector<Tile> tiles;
int mapWidth;
int mapHeight;
SDL_Texture* playerT;

void mainLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            running = false;
            // TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
        }
        if (event.type == SDL_KEYDOWN) {
            keys[event.key.keysym.scancode] = true;
            if (event.key.keysym.scancode == SDL_SCANCODE_A) {
                movement = Movement::Left;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_D) {
                movement = Movement::Right;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_W) {
                movement = Movement::Up;
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_S) {
                movement = Movement::Down;
            }
        }
        if (event.type == SDL_KEYUP) {
            keys[event.key.keysym.scancode] = false;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            buttons[event.button.button] = true;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            buttons[event.button.button] = false;
        }
        if (event.type == SDL_MOUSEMOTION) {
            float scaleX, scaleY;
            SDL_RenderGetScale(renderer, &scaleX, &scaleY);
            mousePos.x = event.motion.x / scaleX;
            mousePos.y = event.motion.y / scaleY;
            realMousePos.x = event.motion.x;
            realMousePos.y = event.motion.y;
        }
    }
    if (movement == Movement::Left) {
        for (Tile& t : tiles) {
            t.dstR.x += PLAYER_SPEED;
        }
    }
    else if (movement == Movement::Right) {
        for (Tile& t : tiles) {
            t.dstR.x -= PLAYER_SPEED;
        }
    }
    else if (movement == Movement::Up) {
        for (Tile& t : tiles) {
            t.dstR.y += PLAYER_SPEED;
        }
    }
    else if (movement == Movement::Down) {
        for (Tile& t : tiles) {
            t.dstR.y -= PLAYER_SPEED;
        }
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    for (Tile& t : tiles) {
        if (t.dstR.x + t.dstR.w >= mapWidth) {
            t.dstR.x += -mapWidth;
        }
        if (t.dstR.x + t.dstR.w < 0) {
            t.dstR.x += mapWidth;
        }
        if (t.dstR.y + t.dstR.h >= mapHeight) {
            t.dstR.y += -mapHeight;
        }
        if (t.dstR.y + t.dstR.h < 0) {
            t.dstR.y += mapHeight;
        }
        if (t.dstR.x + t.dstR.w >= 0 && t.dstR.x - t.dstR.w <= windowWidth && t.dstR.y + t.dstR.h >= 0 && t.dstR.y - t.dstR.h <= windowHeight) {
            SDL_RenderCopyF(renderer, getT(renderer, t.source), &t.srcR, &t.dstR);
        }
    }
    SDL_RenderCopy(renderer, playerT, 0, &playerR);
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[])
{
    std::srand(std::time(0));
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_LogSetOutputFunction(logOutputCallback, 0);
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    SDL_GetMouseState(&mousePos.x, &mousePos.y);
    SDL_Window* window = SDL_CreateWindow("Mincraft", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
    SDL_AddEventWatch(eventWatch, 0);
    playerT = IMG_LoadTexture(renderer, "res/player.png");
    playerR.w = 16;
    playerR.h = 32;
    playerR.x = windowWidth / 2 - playerR.w / 2;
    playerR.y = windowHeight / 2 - playerR.h / 2;

    pugi::xml_document doc;
    doc.load_string(mapStr.c_str());
    pugi::xml_node mapNode = doc.child("map");
    mapWidth = mapNode.attribute("width").as_int() * mapNode.attribute("tilewidth").as_int();
    mapHeight = mapNode.attribute("height").as_int() * mapNode.attribute("tileheight").as_int();
    auto layersIt = mapNode.children("layer");
    for (pugi::xml_node& layer : layersIt) {
        std::vector<int> data;
        {
            std::string csv = layer.child("data").text().as_string();
            std::stringstream ss(csv);
            std::string value;
            while (std::getline(ss, value, ',')) {
                data.push_back(std::stoi(value));
            }
        }
        for (int y = 0; y < mapNode.attribute("height").as_int(); ++y) {
            for (int x = 0; x < mapNode.attribute("width").as_int(); ++x) {
                Tile t;
                t.dstR.w = mapNode.attribute("tilewidth").as_int();
                t.dstR.h = mapNode.attribute("tileheight").as_int();
                t.dstR.x = x * t.dstR.w;
                t.dstR.y = y * t.dstR.h;
                int id = data[y * mapNode.attribute("height").as_int() + x];
                if (id != 0) {
                    pugi::xml_node tilesetNode;
                    auto tilesetsIt = mapNode.children("tileset");
                    for (pugi::xml_node& tileset : tilesetsIt) {
                        int firstgid = tileset.attribute("firstgid").as_int();
                        int tileCount = tileset.attribute("tilecount").as_int();
                        int lastGid = firstgid + tileCount - 1;
                        if (id >= firstgid && id <= lastGid) {
                            tilesetNode = tileset;
                            break;
                        }
                    }
                    t.srcR.w = tilesetNode.attribute("tilewidth").as_int();
                    t.srcR.h = tilesetNode.attribute("tileheight").as_int();
                    t.srcR.x = (id - tilesetNode.attribute("firstgid").as_int()) % tilesetNode.attribute("columns").as_int() * t.srcR.w;
                    t.srcR.y = (id - tilesetNode.attribute("firstgid").as_int()) / tilesetNode.attribute("columns").as_int() * t.srcR.h;
                    t.source = tilesetNode.child("image").attribute("source").as_string();
                    tiles.push_back(t);
                }
            }
        }
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    while (running) {
        mainLoop();
    }
#endif
    // TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
    return 0;
}