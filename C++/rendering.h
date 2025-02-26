#ifndef RENDERING_H
#define RENDERING_H

#include "common.h"
#include <vector>

class Renderer {
private:
    SDL_Renderer* renderer;
    SDL_Window* window;
    std::vector<SDL_Rect> mapTiles;
    std::vector<SDL_Rect> barracks;
    std::vector<SDL_Rect> obstacleCourses;
    std::vector<SDL_Rect> sandPits;
    std::vector<SDL_Rect> roads;
    std::vector<SDL_Rect> rifleRanges;
    std::vector<SDL_Rect> paradeDecks;
    std::vector<SDL_Rect> chowHalls;
    std::vector<SDL_Rect> waterAreas;
    SDL_Texture* recruitTexture;
    SDL_Texture* diTexture;
    SDL_Texture* gearTexture;
    SDL_Texture* barrackTexture;      // Texture for barracks (brick with windows)
    SDL_Texture* roadTexture;        // Texture for roads (pavement)
    SDL_Texture* obstacleTexture;    // Texture for obstacles (wooden walls or training equipment)
    SDL_Texture* sandPitTexture;     // Texture for sand pits (detailed sand pattern)
    SDL_Texture* rifleRangeTexture;  // Texture for rifle ranges (tall grass or target range)
    SDL_Texture* paradeDeckTexture;  // Texture for parade decks (smooth pavement or concrete)
    SDL_Texture* chowHallTexture;    // Texture for chow halls (brick or building facade)
    SDL_Texture* waterTexture;       // Texture for water areas (ocean or river)
    Vector2 cameraPos;
    std::vector<Vector2> dustParticles;

public:
    Renderer(SDL_Window* win, SDL_Renderer* rend);
    ~Renderer();
    void initializeMap(const std::vector<SDL_Rect>& tiles, const std::vector<SDL_Rect>& buildings,
                       const std::vector<SDL_Rect>& obstacles, const std::vector<SDL_Rect>& pits,
                       const std::vector<SDL_Rect>& roadTiles, const std::vector<SDL_Rect>& ranges,
                       const std::vector<SDL_Rect>& decks, const std::vector<SDL_Rect>& chows,
                       const std::vector<SDL_Rect>& waters);
    void setTextures(SDL_Texture* recruit, SDL_Texture* di, SDL_Texture* gear,
                     SDL_Texture* barrack = nullptr, SDL_Texture* road = nullptr, SDL_Texture* obstacle = nullptr,
                     SDL_Texture* sandPit = nullptr, SDL_Texture* rifleRange = nullptr, SDL_Texture* paradeDeck = nullptr,
                     SDL_Texture* chowHall = nullptr, SDL_Texture* water = nullptr);
    void renderScene(Vector2 recruitPos, Vector2 diPos, Vector2 gearPos, bool gearCollected, float stamina, int catchCount, int frameCount, int weatherTimer, float dayNightCycle, int recruitFrame, int diFrame);
    void addParticle(Vector2 pos);
    void setCamera(Vector2 pos);
    std::vector<Vector2>& getDustParticles();
    float getDayNightFactor();
};

#endif // RENDERING_H
