#include "rendering.h"
#include <iostream>
#include <cmath>
#include <random>

Renderer::Renderer(SDL_Window* win, SDL_Renderer* rend) : window(win), renderer(rend) {
    cameraPos = Vector2(0, 0);
    barrackTexture = nullptr;
    roadTexture = nullptr;
    obstacleTexture = nullptr;
    sandPitTexture = nullptr;
    rifleRangeTexture = nullptr;
    paradeDeckTexture = nullptr;
    chowHallTexture = nullptr;
    waterTexture = nullptr;
}

Renderer::~Renderer() {
    if (barrackTexture) SDL_DestroyTexture(barrackTexture);
    if (roadTexture) SDL_DestroyTexture(roadTexture);
    if (obstacleTexture) SDL_DestroyTexture(obstacleTexture);
    if (sandPitTexture) SDL_DestroyTexture(sandPitTexture);
    if (rifleRangeTexture) SDL_DestroyTexture(rifleRangeTexture);
    if (paradeDeckTexture) SDL_DestroyTexture(paradeDeckTexture);
    if (chowHallTexture) SDL_DestroyTexture(chowHallTexture);
    if (waterTexture) SDL_DestroyTexture(waterTexture);
    if (recruitTexture) SDL_DestroyTexture(recruitTexture);
    if (diTexture) SDL_DestroyTexture(diTexture);
    if (gearTexture) SDL_DestroyTexture(gearTexture);
}

void Renderer::initializeMap(const std::vector<SDL_Rect>& tiles, const std::vector<SDL_Rect>& buildings,
                            const std::vector<SDL_Rect>& obstacles, const std::vector<SDL_Rect>& pits,
                            const std::vector<SDL_Rect>& roadTiles, const std::vector<SDL_Rect>& ranges,
                            const std::vector<SDL_Rect>& decks, const std::vector<SDL_Rect>& chows,
                            const std::vector<SDL_Rect>& waters) {
    mapTiles = tiles;
    barracks = buildings;
    obstacleCourses = obstacles;
    sandPits = pits;
    roads = roadTiles;
    rifleRanges = ranges;
    paradeDecks = decks;
    chowHalls = chows;
    waterAreas = waters;
}

void Renderer::setTextures(SDL_Texture* recruit, SDL_Texture* di, SDL_Texture* gear,
                          SDL_Texture* barrack, SDL_Texture* road, SDL_Texture* obstacle,
                          SDL_Texture* sandPit, SDL_Texture* rifleRange, SDL_Texture* paradeDeck,
                          SDL_Texture* chowHall, SDL_Texture* water) {
    recruitTexture = recruit;
    diTexture = di;
    gearTexture = gear;
    barrackTexture = barrack ? barrack : nullptr;  // Optional textures
    roadTexture = road ? road : nullptr;
    obstacleTexture = obstacle ? obstacle : nullptr;
    sandPitTexture = sandPit ? sandPit : nullptr;
    rifleRangeTexture = rifleRange ? rifleRange : nullptr;
    paradeDeckTexture = paradeDeck ? paradeDeck : nullptr;
    chowHallTexture = chowHall ? chowHall : nullptr;
    waterTexture = water ? water : nullptr;
}

void Renderer::addParticle(Vector2 pos) {
    dustParticles.push_back(pos);
}

void Renderer::setCamera(Vector2 pos) {
    cameraPos = pos;
}

std::vector<Vector2>& Renderer::getDustParticles() {
    return dustParticles;
}

float Renderer::getDayNightFactor() {
    // Get current time in EST (UTC-5)
    time_t now = time(nullptr);
    struct tm* local = localtime(&now);
    int utc_offset = -5; // EST is UTC-5
    local->tm_hour = (local->tm_hour + utc_offset + 24) % 24; // Adjust to EST

    float hour = local->tm_hour + local->tm_min / 60.0f; // Hour as a float (e.g., 14:30 = 14.5)

    // Define day/night periods (6 AM to 6 PM day, 6 PM to 6 AM night)
    float sunrise = 6.0f;
    float sunset = 18.0f;
    float t;

    if (hour >= sunrise && hour < sunset) {
        // Daytime: interpolate from night (t=1) to day (t=0)
        t = (hour - sunrise) / (sunset - sunrise);
        t = 1.0f - t; // Reverse so 6 AM is full day (t=0), 6 PM is full night (t=1)
    } else {
        // Nighttime: interpolate across night period
        float night_hours = (hour < sunrise) ? (hour + 24 - sunset) : (hour - sunset);
        t = night_hours / (24.0f - (sunset - sunrise));
        t = (t < 0.5f) ? 1.0f : 1.0f - ((t - 0.5f) * 2); // Smooth transition: 1 at 6 PM, 0 at midnight, 1 at 6 AM
    }

    return std::max(0.0f, std::min(1.0f, t)); // Clamp between 0 and 1
}

void Renderer::renderScene(Vector2 recruitPos, Vector2 diPos, Vector2 gearPos, bool gearCollected, float stamina, int catchCount, int frameCount, int weatherTimer, float dayNightCycle, int recruitFrame, int diFrame) {
    float t = (dayNightCycle >= 0) ? dayNightCycle : getDayNightFactor();

    // Interpolate between SAND (day) and NIGHT (night)
    SDL_Color bgColor = {
        static_cast<Uint8>((1 - t) * SAND.r + t * NIGHT.r),
        static_cast<Uint8>((1 - t) * SAND.g + t * NIGHT.g),
        static_cast<Uint8>((1 - t) * SAND.b + t * NIGHT.b),
        255
    };
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderClear(renderer);

    // Draw Parris Island map (tiles, water, roads, buildings, obstacles, sand pits, etc.) with textures
    for (const auto& tile : mapTiles) {
        SDL_Rect screenTile = {static_cast<int>(tile.x - cameraPos.x), static_cast<int>(tile.y - cameraPos.y), tile.w, tile.h};
        if (screenTile.x + tile.w > 0 && screenTile.x < WIDTH && screenTile.y + tile.h > 0 && screenTile.y < HEIGHT) {
            SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);  // Use interpolated background
            SDL_RenderFillRect(renderer, &screenTile);
        }
    }
    for (const auto& water : waterAreas) {
        SDL_Rect screenWater = {static_cast<int>(water.x - cameraPos.x), static_cast<int>(water.y - cameraPos.y), water.w, water.h};
        if (screenWater.x + screenWater.w > 0 && screenWater.x < WIDTH && screenWater.y + screenWater.h > 0 && screenWater.y < HEIGHT) {
            if (waterTexture) {
                SDL_RenderCopy(renderer, waterTexture, NULL, &screenWater);
            } else {
                SDL_SetRenderDrawColor(renderer, WATER.r, WATER.g, WATER.b, WATER.a);  // Fallback to color
                SDL_RenderFillRect(renderer, &screenWater);
            }
        }
    }
    for (const auto& road : roads) {
        SDL_Rect screenRoad = {static_cast<int>(road.x - cameraPos.x), static_cast<int>(road.y - cameraPos.y), road.w, road.h};
        if (screenRoad.x + screenRoad.w > 0 && screenRoad.x < WIDTH && screenRoad.y + screenRoad.h > 0 && screenRoad.y < HEIGHT) {
            if (roadTexture) {
                SDL_RenderCopy(renderer, roadTexture, NULL, &screenRoad);
            } else {
                SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, GRAY.a);  // Fallback to gray
                SDL_RenderFillRect(renderer, &screenRoad);
            }
        }
    }
    for (const auto& barrack : barracks) {
        SDL_Rect screenBarrack = {static_cast<int>(barrack.x - cameraPos.x), static_cast<int>(barrack.y - cameraPos.y), barrack.w, barrack.h};
        if (screenBarrack.x + screenBarrack.w > 0 && screenBarrack.x < WIDTH && screenBarrack.y + screenBarrack.h > 0 && screenBarrack.y < HEIGHT) {
            if (barrackTexture) {
                SDL_RenderCopy(renderer, barrackTexture, NULL, &screenBarrack);
            } else {
                SDL_SetRenderDrawColor(renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);  // Fallback to brown
                SDL_RenderFillRect(renderer, &screenBarrack);
            }
        }
    }
    for (const auto& obstacle : obstacleCourses) {
        SDL_Rect screenObstacle = {static_cast<int>(obstacle.x - cameraPos.x), static_cast<int>(obstacle.y - cameraPos.y), obstacle.w, obstacle.h};
        if (screenObstacle.x + screenObstacle.w > 0 && screenObstacle.x < WIDTH && screenObstacle.y + screenObstacle.h > 0 && screenObstacle.y < HEIGHT) {
            if (obstacleTexture) {
                SDL_RenderCopy(renderer, obstacleTexture, NULL, &screenObstacle);
            } else {
                SDL_SetRenderDrawColor(renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);  // Fallback to brown
                SDL_RenderFillRect(renderer, &screenObstacle);
            }
        }
    }
    for (const auto& sandPit : sandPits) {
        SDL_Rect screenSandPit = {static_cast<int>(sandPit.x - cameraPos.x), static_cast<int>(sandPit.y - cameraPos.y), sandPit.w, sandPit.h};
        if (screenSandPit.x + screenSandPit.w > 0 && screenSandPit.x < WIDTH && screenSandPit.y + screenSandPit.h > 0 && screenSandPit.y < HEIGHT) {
            if (sandPitTexture) {
                SDL_RenderCopy(renderer, sandPitTexture, NULL, &screenSandPit);
            } else {
                SDL_SetRenderDrawColor(renderer, DARK_SAND.r, DARK_SAND.g, DARK_SAND.b, DARK_SAND.a);  // Fallback to dark sand
                SDL_RenderFillRect(renderer, &screenSandPit);
            }
        }
    }
    for (const auto& range : rifleRanges) {
        SDL_Rect screenRange = {static_cast<int>(range.x - cameraPos.x), static_cast<int>(range.y - cameraPos.y), range.w, range.h};
        if (screenRange.x + screenRange.w > 0 && screenRange.x < WIDTH && screenRange.y + screenRange.h > 0 && screenRange.y < HEIGHT) {
            if (rifleRangeTexture) {
                SDL_RenderCopy(renderer, rifleRangeTexture, NULL, &screenRange);
            } else {
                SDL_SetRenderDrawColor(renderer, GRASS.r, GRASS.g, GRASS.b, GRASS.a);  // Fallback to green grass
                SDL_RenderFillRect(renderer, &screenRange);
            }
        }
    }
    for (const auto& deck : paradeDecks) {
        SDL_Rect screenDeck = {static_cast<int>(deck.x - cameraPos.x), static_cast<int>(deck.y - cameraPos.y), deck.w, deck.h};
        if (screenDeck.x + screenDeck.w > 0 && screenDeck.x < WIDTH && screenDeck.y + screenDeck.h > 0 && screenDeck.y < HEIGHT) {
            if (paradeDeckTexture) {
                SDL_RenderCopy(renderer, paradeDeckTexture, NULL, &screenDeck);
            } else {
                SDL_SetRenderDrawColor(renderer, PAVEMENT.r, PAVEMENT.g, PAVEMENT.b, PAVEMENT.a);  // Fallback to white pavement
                SDL_RenderFillRect(renderer, &screenDeck);
            }
        }
    }
    for (const auto& chow : chowHalls) {
        SDL_Rect screenChow = {static_cast<int>(chow.x - cameraPos.x), static_cast<int>(chow.y - cameraPos.y), chow.w, chow.h};
        if (screenChow.x + screenChow.w > 0 && screenChow.x < WIDTH && screenChow.y + screenChow.h > 0 && screenChow.y < HEIGHT) {
            if (chowHallTexture) {
                SDL_RenderCopy(renderer, chowHallTexture, NULL, &screenChow);
            } else {
                SDL_SetRenderDrawColor(renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);  // Fallback to brown
                SDL_RenderFillRect(renderer, &screenChow);
            }
        }
    }

    // Draw recruit with animation
    SDL_Rect srcRect = {recruitFrame * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE};  // Assuming horizontal sprite sheet
    SDL_Rect recruitRect = {static_cast<int>(recruitPos.x - cameraPos.x), static_cast<int>(recruitPos.y - cameraPos.y), SPRITE_SIZE, SPRITE_SIZE};
    if (recruitRect.x + recruitRect.w > 0 && recruitRect.x < WIDTH && recruitRect.y + recruitRect.h > 0 && recruitRect.y < HEIGHT) {
        SDL_RenderCopy(renderer, recruitTexture, &srcRect, &recruitRect);
    }

    // Draw DI (if not gear collected) with animation
    if (!gearCollected) {
        SDL_Rect diSrcRect = {diFrame * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE};  // Assuming horizontal sprite sheet
        SDL_Rect diRect = {static_cast<int>(diPos.x - cameraPos.x), static_cast<int>(diPos.y - cameraPos.y), SPRITE_SIZE, SPRITE_SIZE};
        if (diRect.x + diRect.w > 0 && diRect.x < WIDTH && diRect.y + diRect.h > 0 && diRect.y < HEIGHT) {
            SDL_RenderCopy(renderer, diTexture, &diSrcRect, &diRect);
        }
    }

    // Draw gear (if not collected)
    if (!gearCollected) {
        SDL_Rect gearRect = {static_cast<int>(gearPos.x - cameraPos.x), static_cast<int>(gearPos.y - cameraPos.y), SPRITE_SIZE, SPRITE_SIZE};
        if (gearRect.x + gearRect.w > 0 && gearRect.x < WIDTH && gearRect.y + gearRect.h > 0 && gearRect.y < HEIGHT) {
            SDL_RenderCopy(renderer, gearTexture, NULL, &gearRect);
        }
    }

    // Draw stamina bar (UI)
    SDL_Rect staminaOutline = {10, 10, 200, 20};
    SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderFillRect(renderer, &staminaOutline);
    SDL_Rect staminaFill = {10, 10, static_cast<int>((stamina / 100.0f) * 200), 20};
    SDL_SetRenderDrawColor(renderer, GREEN.r, GREEN.g, GREEN.b, GREEN.a);
    SDL_RenderFillRect(renderer, &staminaFill);

    // Weather (dust storm if weatherTimer > 0)
    if (weatherTimer > 0) {
        weatherTimer--;
        if (random() % 5 == 0) {
            addParticle(Vector2(random() % WIDTH + cameraPos.x, random() % HEIGHT + cameraPos.y));
        }
    }

    // Update and draw dust particles
    for (auto it = dustParticles.begin(); it != dustParticles.end();) {
        it->y += 1;
        if (it->y > cameraPos.y + HEIGHT || it->x < cameraPos.x || it->x > cameraPos.x + WIDTH) {
            it = dustParticles.erase(it);
        } else {
            SDL_Rect particleRect = {static_cast<int>(it->x - cameraPos.x), static_cast<int>(it->y - cameraPos.y), 8, 8};
            SDL_SetRenderDrawColor(renderer, BROWN.r, BROWN.g, BROWN.b, BROWN.a);
            SDL_RenderFillRect(renderer, &particleRect);
            ++it;
        }
    }

    SDL_RenderPresent(renderer);
}
