// parris_island_trials.cpp
#include "common.h"
#include "rendering.h"
#include <iostream>
#include <string>
#include <random>  // For random escape chance

class ParrisIslandTrials {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Renderer* gameRenderer;
    TTF_Font* font;
    Mix_Music* bgMusic = nullptr;
    Mix_Chunk* diYell = nullptr, *footsteps = nullptr;
    SDL_Texture* recruitTextures[4];
    SDL_Texture* diTextures[2];
    SDL_Texture* gearTexture;
    SDL_Texture* barrackTexture;      // Texture for barracks
    SDL_Texture* roadTexture;        // Texture for roads
    SDL_Texture* obstacleTexture;    // Texture for obstacles
    SDL_Texture* sandPitTexture;     // Texture for sand pits
    SDL_Texture* rifleRangeTexture;  // Texture for rifle ranges
    SDL_Texture* paradeDeckTexture;  // Texture for parade decks
    SDL_Texture* chowHallTexture;    // Texture for chow halls
    SDL_Texture* waterTexture;       // Texture for water areas
    Vector2 recruitPos, diPos, gearPos;
    std::vector<SDL_Rect> barracks, obstacleCourses, sandPits, roads, rifleRanges, paradeDecks, chowHalls;
    float recruitSpeed = 1.5f, sprintSpeed = 3.0f, diSpeed = 1.5f;  // Match recruitSpeed and diSpeed, increase sprintSpeed slightly
    float stamina = 100.0f, maxStamina = 100.0f, staminaDrain = 0.1f, staminaRegen = 0.2f;
    int recruitFrame = 0, diFrame = 0, catchCount = 0, maxCatches = 15, catchCooldown = 120, lastCatchCheck = 0;
    bool gearCollected = false, running = true, diLatched = false;
    int frameCount = 0, weatherTimer = 0, latchTimer = 0;
    const int LATCH_DURATION = 120;  // Frames to stay latched before automatic escape (2 seconds at 60 FPS)

    SDL_Texture* renderText(const std::string& text, SDL_Color color) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!surface) {
            std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
            return nullptr;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    Vector2 slideAroundObstacle(Vector2 pos, Vector2 direction, float speed, const SDL_Rect& obstacle) {
        SDL_Rect newRect = {static_cast<int>(pos.x), static_cast<int>(pos.y), SPRITE_SIZE, SPRITE_SIZE};
        if (SDL_HasIntersection(&newRect, &obstacle)) {
            Vector2 slidePos(pos.x + direction.x * speed * 2, pos.y);
            newRect = {static_cast<int>(slidePos.x), static_cast<int>(slidePos.y), SPRITE_SIZE, SPRITE_SIZE};
            if (!SDL_HasIntersection(&newRect, &obstacle)) return slidePos;
            slidePos = Vector2(pos.x, pos.y + direction.y * speed * 2);
            newRect = {static_cast<int>(slidePos.x), static_cast<int>(slidePos.y), SPRITE_SIZE, SPRITE_SIZE};
            if (!SDL_HasIntersection(&newRect, &obstacle)) return slidePos;
            return pos;  // Stay in place if still stuck
        }
        return Vector2(pos.x + direction.x * speed, pos.y + direction.y * speed);
    }

    // Helper to generate random number (0 to 1) for escape chance
    float getRandomChance() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

public:
    ParrisIslandTrials() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || TTF_Init() < 0 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cerr << "Initialization failed: " << SDL_GetError() << " " << TTF_GetError() << " " << Mix_GetError() << std::endl;
            running = false;
        }
        window = SDL_CreateWindow("The Parris Island Trials", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!window || !renderer) {
            std::cerr << "Window/Renderer failed: " << SDL_GetError() << std::endl;
            running = false;
        }
        font = TTF_OpenFont("sprites/DejaVuSans.ttf", 36);
        if (!font) std::cerr << "Font failed: " << TTF_GetError() << std::endl;

        bgMusic = Mix_LoadMUS("sprites/march_music.mp3");
        diYell = Mix_LoadWAV("sprites/di_yell.wav");
        footsteps = Mix_LoadWAV("sprites/footsteps.wav");
        if (bgMusic) Mix_PlayMusic(bgMusic, -1);

        // Load and verify recruit sprites
        for (int i = 0; i < 4; ++i) {
            std::string path = "sprites/recruit_walk" + std::to_string(i + 1) + ".png";
            SDL_Surface* surface = IMG_Load(path.c_str());
            if (!surface) {
                std::cerr << "Recruit sprite failed: " << path << " " << IMG_GetError() << std::endl;
                running = false;
                break;
            }
            recruitTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            if (!recruitTextures[i]) {
                std::cerr << "Failed to create recruit texture from " << path << ": " << SDL_GetError() << std::endl;
                running = false;
                break;
            }
        }

        // Load and verify DI sprites
        for (int i = 0; i < 2; ++i) {
            std::string path = "sprites/di_yell" + std::to_string(i + 1) + ".png";
            SDL_Surface* surface = IMG_Load(path.c_str());
            if (!surface) {
                std::cerr << "DI sprite failed: " << path << " " << IMG_GetError() << std::endl;
                running = false;
                break;
            }
            diTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            if (!diTextures[i]) {
                std::cerr << "Failed to create DI texture from " << path << ": " << SDL_GetError() << std::endl;
                running = false;
                break;
            }
        }

        // Load and verify gear sprite
        SDL_Surface* gearSurface = IMG_Load("sprites/gear.png");
        if (!gearSurface) {
            std::cerr << "Gear sprite failed: " << IMG_GetError() << std::endl;
            running = false;
        } else {
            gearTexture = SDL_CreateTextureFromSurface(renderer, gearSurface);
            SDL_FreeSurface(gearSurface);
            if (!gearTexture) {
                std::cerr << "Failed to create gear texture: " << SDL_GetError() << std::endl;
                running = false;
            }
        }

        // Load and verify map textures with detailed error messages
        barrackTexture = IMG_LoadTexture(renderer, "sprites/barrack.png");
        if (!barrackTexture) {
            std::cerr << "Barrack texture failed: " << IMG_GetError() << " (Path: sprites/barrack.png)" << std::endl;
            running = false;
        }
        roadTexture = IMG_LoadTexture(renderer, "sprites/road.png");
        if (!roadTexture) {
            std::cerr << "Road texture failed: " << IMG_GetError() << " (Path: sprites/road.png)" << std::endl;
            running = false;
        }
        obstacleTexture = IMG_LoadTexture(renderer, "sprites/obstacle.png");
        if (!obstacleTexture) {
            std::cerr << "Obstacle texture failed: " << IMG_GetError() << " (Path: sprites/obstacle.png)" << std::endl;
            running = false;
        }
        sandPitTexture = IMG_LoadTexture(renderer, "sprites/sand_pit.png");
        if (!sandPitTexture) {
            std::cerr << "Sand pit texture failed: " << IMG_GetError() << " (Path: sprites/sand_pit.png)" << std::endl;
            running = false;
        }
        rifleRangeTexture = IMG_LoadTexture(renderer, "sprites/rifle_range.png");
        if (!rifleRangeTexture) {
            std::cerr << "Rifle range texture failed: " << IMG_GetError() << " (Path: sprites/rifle_range.png)" << std::endl;
            running = false;
        }
        paradeDeckTexture = IMG_LoadTexture(renderer, "sprites/parade_deck.png");
        if (!paradeDeckTexture) {
            std::cerr << "Parade deck texture failed: " << IMG_GetError() << " (Path: sprites/parade_deck.png)" << std::endl;
            running = false;
        }
        chowHallTexture = IMG_LoadTexture(renderer, "sprites/chow_hall.png");
        if (!chowHallTexture) {
            std::cerr << "Chow hall texture failed: " << IMG_GetError() << " (Path: sprites/chow_hall.png)" << std::endl;
            running = false;
        }
        waterTexture = IMG_LoadTexture(renderer, "sprites/water.png");
        if (!waterTexture) {
            std::cerr << "Water texture failed: " << IMG_GetError() << " (Path: sprites/water.png)" << std::endl;
            running = false;
        }

        if (!running) {
            std::cerr << "Critical asset loading failed, exiting initialization." << std::endl;
            return;
        }

        recruitPos = Vector2(400, 250);  // Start near west road, on ground
        diPos = Vector2(1200, 500);      // Start near east road, on ground
        gearPos = Vector2(random() % (MAP_WIDTH - 200) + 100, random() % (MAP_HEIGHT - 200) + 100);

        // Initialize map (roads, buildings, etc., passed to Renderer)
        const int TILE_SIZE = 32;
        std::vector<SDL_Rect> tiles, buildings, obstacles, pits, roadTiles, ranges, decks, chows;
        for (int y = 0; y < MAP_HEIGHT; y += TILE_SIZE) {
            for (int x = 0; x < MAP_WIDTH; x += TILE_SIZE) {
                tiles.push_back({x, y, TILE_SIZE, TILE_SIZE});
            }
        }
        roadTiles.push_back({800 - 16, 0, 32, MAP_HEIGHT});         // Main north-south road
        roadTiles.push_back({800 - 16, 300 - 16, 32, 32});          // Upper crossroad
        roadTiles.push_back({800 - 16, 900 - 16, 32, 32});          // Lower crossroad
        roadTiles.push_back({100, 300 - 16, 16, 32});               // Side path to obstacle
        roadTiles.push_back({1500, 300 - 16, 16, 32});              // Side path to rifle range
        roadTiles.push_back({100, 900 - 16, 16, 32});               // Side path to parade deck
        roadTiles.push_back({1500, 900 - 16, 16, 32});              // Side path to chow hall
        buildings.push_back({200, 100, 64, 64});    // Barracks 1
        buildings.push_back({500, 400, 64, 64});    // Barracks 2
        buildings.push_back({600, 500, 64, 64});    // Chow Hall
        obstacles.push_back({100, 300, 192, 48});   // Obstacle Course
        pits.push_back({300, 300, 100, 100});       // Sand Pit
        ranges.push_back({600, 100, 192, 48});      // Rifle Range
        decks.push_back({100, 500, 192, 48});       // Parade Deck
        chows.push_back({600, 500, 64, 64});        // Chow Hall (already in buildings, but included for consistency)

        barracks = buildings;
        obstacleCourses = obstacles;
        sandPits = pits;
        roads = roadTiles;
        rifleRanges = ranges;
        paradeDecks = decks;
        chowHalls = chows;

        gameRenderer = new Renderer(window, renderer);
        gameRenderer->initializeMap(tiles, buildings, obstacles, pits, roadTiles, ranges, decks, chows, std::vector<SDL_Rect>()); // No water areas for now
        gameRenderer->setTextures(recruitTextures[0], diTextures[0], gearTexture,
                                 barrackTexture, roadTexture, obstacleTexture,
                                 sandPitTexture, rifleRangeTexture, paradeDeckTexture,
                                 chowHallTexture, waterTexture);
    }

    ~ParrisIslandTrials() {
        if (gameRenderer) delete gameRenderer;
        if (font) TTF_CloseFont(font);
        if (bgMusic) Mix_FreeMusic(bgMusic);
        if (diYell) Mix_FreeChunk(diYell);
        if (footsteps) Mix_FreeChunk(footsteps);
        if (barrackTexture) SDL_DestroyTexture(barrackTexture);
        if (roadTexture) SDL_DestroyTexture(roadTexture);
        if (obstacleTexture) SDL_DestroyTexture(obstacleTexture);
        if (sandPitTexture) SDL_DestroyTexture(sandPitTexture);
        if (rifleRangeTexture) SDL_DestroyTexture(rifleRangeTexture);
        if (paradeDeckTexture) SDL_DestroyTexture(paradeDeckTexture);
        if (chowHallTexture) SDL_DestroyTexture(chowHallTexture);
        if (waterTexture) SDL_DestroyTexture(waterTexture);
        Mix_CloseAudio();
        TTF_Quit();
        for (int i = 0; i < 4; ++i) SDL_DestroyTexture(recruitTextures[i]);
        for (int i = 0; i < 2; ++i) SDL_DestroyTexture(diTextures[i]);
        SDL_DestroyTexture(gearTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool isRunning() const { return running; }

    void run() {
        SDL_Event event;
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                    else if (diLatched && event.key.keysym.sym == SDLK_SPACE) {  // Attempt to escape when latched
                        float escapeChance = std::min(0.9f, stamina / maxStamina);  // Cap at 90% chance, even at full stamina
                        if (getRandomChance() < escapeChance) {
                            diLatched = false;
                            latchTimer = 0;
                            diPos = Vector2(diPos.x + random() % 200 - 100, diPos.y + random() % 200 - 100);  // DI backs off further
                            std::cout << "Escaped the DI's grasp!\n";
                            // Visual feedback (on-screen message)
                            SDL_Texture* escapeText = renderText("Escaped the DI!", WHITE);
                            if (escapeText) {
                                int textW, textH;
                                SDL_QueryTexture(escapeText, NULL, NULL, &textW, &textH);
                                SDL_Rect textRect = {(WIDTH - textW) / 2, HEIGHT / 2, textW, textH};
                                SDL_RenderCopy(renderer, escapeText, NULL, &textRect);
                                SDL_DestroyTexture(escapeText);
                                SDL_RenderPresent(renderer);
                                SDL_Delay(1000);  // Show message for 1 second
                            }
                        } else {
                            stamina -= 15.0f;  // Increase stamina cost for failed escape
                            std::cout << "Failed to escape the DI! Stamina reduced.\n";
                            // Visual feedback for failed escape
                            SDL_Texture* failText = renderText("Failed to Escape!", WHITE);
                            if (failText) {
                                int textW, textH;
                                SDL_QueryTexture(failText, NULL, NULL, &textW, &textH);
                                SDL_Rect textRect = {(WIDTH - textW) / 2, HEIGHT / 2, textW, textH};
                                SDL_RenderCopy(renderer, failText, NULL, &textRect);
                                SDL_DestroyTexture(failText);
                                SDL_RenderPresent(renderer);
                                SDL_Delay(1000);  // Show message for 1 second
                            }
                        }
                    }
                }
            }

            const Uint8* keys = SDL_GetKeyboardState(NULL);
            Vector2 direction(0, 0);
            bool isSprinting = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
            if (keys[SDL_SCANCODE_W]) direction.y -= 1;
            if (keys[SDL_SCANCODE_S]) direction.y += 1;
            if (keys[SDL_SCANCODE_A]) direction.x -= 1;
            if (keys[SDL_SCANCODE_D]) direction.x += 1;

            if (direction.x != 0 || direction.y != 0) {
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length != 0) { direction.x /= length; direction.y /= length; }
            }

            float currentSpeed = (isSprinting && stamina > 0 && !diLatched) ? sprintSpeed : recruitSpeed;
            if (isSprinting && stamina > 0 && !diLatched) stamina -= staminaDrain;
            else if (stamina < maxStamina && !diLatched) stamina += staminaRegen;
            stamina = std::max(0.0f, std::min(stamina, maxStamina));

            // Move recruit, check collisions with all structures, unless latched
            Vector2 newPos = recruitPos;
            bool inSandPit = false;

            if (!diLatched) {
                // Slide around or slow down in sand pits for all structures except roads
                newPos = slideAroundObstacle(newPos, direction, currentSpeed, barracks[0]);
                for (const auto& barrack : barracks) {
                    newPos = slideAroundObstacle(newPos, direction, currentSpeed, barrack);
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &barrack)) newPos = recruitPos;  // Block movement into barracks
                }
                for (const auto& obstacle : obstacleCourses) {
                    newPos = slideAroundObstacle(newPos, direction, currentSpeed, obstacle);
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &obstacle)) newPos = recruitPos;  // Block movement into obstacles
                }
                for (const auto& pit : sandPits) {
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &pit)) {
                        inSandPit = true;  // Flag that we're in a sand pit
                        stamina -= staminaDrain;  // Drain stamina in sand pit
                        // Apply slower speed (e.g., half the current speed or a fixed slow speed)
                        float slowSpeed = currentSpeed / 2.0f;  // Halve the speed in sand pits
                        newPos = Vector2(recruitPos.x + direction.x * slowSpeed, recruitPos.y + direction.y * slowSpeed);
                    }
                }
                for (const auto& range : rifleRanges) {
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &range)) newPos = recruitPos;  // Block movement into rifle range
                }
                for (const auto& deck : paradeDecks) {
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &deck)) newPos = recruitPos;  // Block movement into parade deck
                }
                for (const auto& chow : chowHalls) {
                    newPos = slideAroundObstacle(newPos, direction, currentSpeed, chow);
                    SDL_Rect recruitRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                    if (SDL_HasIntersection(&recruitRect, &chow)) newPos = recruitPos;  // Block movement into chow hall
                }

                // Apply movement if not blocked
                recruitPos = newPos;
            }

            // Ensure recruit stays on ground (not above buildings), unless latched
            if (!diLatched) {
                for (const auto& barrack : barracks) {
                    if (recruitPos.x >= barrack.x && recruitPos.x <= barrack.x + barrack.w &&
                        recruitPos.y < barrack.y + barrack.h) {
                        recruitPos.y = barrack.y + barrack.h;  // Push recruit to ground level below building
                    }
                }
                for (const auto& chow : chowHalls) {
                    if (recruitPos.x >= chow.x && recruitPos.x <= chow.x + chow.w &&
                        recruitPos.y < chow.y + chow.h) {
                        recruitPos.y = chow.y + chow.h;  // Push recruit to ground level below chow hall
                    }
                }
            }

            // Update camera (center on recruit, scroll if near edges)
            Vector2 cameraPos = recruitPos;
            cameraPos.x -= WIDTH / 2;
            cameraPos.y -= HEIGHT / 2;
            cameraPos.x = std::max(0.0f, std::min(cameraPos.x, static_cast<float>(MAP_WIDTH - WIDTH)));
            cameraPos.y = std::max(0.0f, std::min(cameraPos.y, static_cast<float>(MAP_HEIGHT - HEIGHT)));

            // Keep recruit on map
            recruitPos.x = std::max(0.0f, std::min(recruitPos.x, static_cast<float>(MAP_WIDTH - SPRITE_SIZE)));
            recruitPos.y = std::max(0.0f, std::min(recruitPos.y, static_cast<float>(MAP_HEIGHT - SPRITE_SIZE)));

            // DI chasing logic (stops if gear collected, with obstacle avoidance and latching)
            if (!gearCollected) {
                if (diLatched) {
                    // DI is latched onto recruit, no movement, increment latch timer
                    latchTimer++;
                    diPos = recruitPos;  // DI stays on recruit while latched
                    if (latchTimer >= LATCH_DURATION || (stamina <= 0 && latchTimer >= LATCH_DURATION / 2)) {  // Automatic escape if timed out or low stamina
                        diLatched = false;
                        latchTimer = 0;
                        diPos = Vector2(diPos.x + random() % 200 - 100, diPos.y + random() % 200 - 100);  // DI backs off further
                        std::cout << "Escaped the DI's grasp automatically!\n";
                        // Visual feedback for automatic escape
                        SDL_Texture* autoEscapeText = renderText("Automatically Escaped the DI!", WHITE);
                        if (autoEscapeText) {
                            int textW, textH;
                            SDL_QueryTexture(autoEscapeText, NULL, NULL, &textW, &textH);
                            SDL_Rect textRect = {(WIDTH - textW) / 2, HEIGHT / 2, textW, textH};
                            SDL_RenderCopy(renderer, autoEscapeText, NULL, &textRect);
                            SDL_DestroyTexture(autoEscapeText);
                            SDL_RenderPresent(renderer);
                            SDL_Delay(1000);  // Show message for 1 second
                        }
                    }
                } else {
                    Vector2 diDirection(recruitPos.x - diPos.x, recruitPos.y - diPos.y);
                    float length = std::sqrt(diDirection.x * diDirection.x + diDirection.y * diDirection.y);
                    if (length != 0) { diDirection.x /= length; diDirection.y /= length; }
                    Vector2 newDiPos = diPos;
                    bool diInSandPit = false;

                    newDiPos = slideAroundObstacle(newDiPos, diDirection, diSpeed, barracks[0]);
                    for (const auto& barrack : barracks) {
                        newDiPos = slideAroundObstacle(newDiPos, diDirection, diSpeed, barrack);
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &barrack)) newDiPos = diPos;  // Block movement into barracks
                    }
                    for (const auto& obstacle : obstacleCourses) {
                        newDiPos = slideAroundObstacle(newDiPos, diDirection, diSpeed, obstacle);
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &obstacle)) newDiPos = diPos;  // Block movement into obstacles
                    }
                    for (const auto& pit : sandPits) {
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &pit)) {
                            diInSandPit = true;  // Flag that DI is in a sand pit
                            // Apply slower speed (e.g., half the DI speed or a fixed slow speed)
                            float diSlowSpeed = diSpeed / 2.0f;  // Halve the speed in sand pits
                            newDiPos = Vector2(diPos.x + diDirection.x * diSlowSpeed, diPos.y + diDirection.y * diSlowSpeed);
                        }
                    }
                    for (const auto& range : rifleRanges) {
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &range)) newDiPos = diPos;  // DI avoids rifle range
                    }
                    for (const auto& deck : paradeDecks) {
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &deck)) newDiPos = diPos;  // DI avoids parade deck
                    }
                    for (const auto& chow : chowHalls) {
                        newDiPos = slideAroundObstacle(newDiPos, diDirection, diSpeed, chow);
                        SDL_Rect diRect = {static_cast<int>(newDiPos.x), static_cast<int>(newPos.y), SPRITE_SIZE, SPRITE_SIZE};
                        if (SDL_HasIntersection(&diRect, &chow)) newDiPos = diPos;  // Block movement into chow hall
                    }

                    diPos = newDiPos;

                    // Ensure DI stays on ground
                    for (const auto& barrack : barracks) {
                        if (diPos.x >= barrack.x && diPos.x <= barrack.x + barrack.w &&
                            diPos.y < barrack.y + barrack.h) {
                            diPos.y = barrack.y + barrack.h;  // Push DI to ground level below building
                        }
                    }
                    for (const auto& chow : chowHalls) {
                        if (diPos.x >= chow.x && diPos.x <= chow.x + chow.w &&
                            diPos.y < chow.y + chow.h) {
                            diPos.y = chow.y + chow.h;  // Push DI to ground level below chow hall
                        }
                    }

                    // Keep DI on map
                    diPos.x = std::max(0.0f, std::min(diPos.x, static_cast<float>(MAP_WIDTH - SPRITE_SIZE)));
                    diPos.y = std::max(0.0f, std::min(diPos.y, static_cast<float>(MAP_HEIGHT - SPRITE_SIZE)));

                    // Check for catching the recruit (only if not already latched)
                    float distanceToDi = std::sqrt((recruitPos.x - diPos.x) * (recruitPos.x - diPos.x) +
                                                 (recruitPos.y - diPos.y) * (recruitPos.y - diPos.y));
                    if (distanceToDi < 20 && !diLatched && lastCatchCheck >= catchCooldown) {
                        diLatched = true;
                        latchTimer = 0;
                        catchCount++;  // Increment catch count only once per latch
                        lastCatchCheck = 0;
                        if (diYell) Mix_PlayChannel(-1, diYell, 0);
                        std::cout << "DI caught you! Times caught: " << catchCount << "/15\n";
                        if (catchCount >= maxCatches) {
                            running = false;
                            std::cout << "DI won! You strip your blouse and head to the sand pit. Game Over!\n";
                            continue;
                        }
                    }
                }
            }

            float gearDistance = std::sqrt((recruitPos.x - gearPos.x) * (recruitPos.x - gearPos.x) +
                                         (recruitPos.y - gearPos.y) * (recruitPos.y - gearPos.y));
            if (gearDistance < 20 && !gearCollected) {
                gearCollected = true;
                std::cout << "Gear collected! DI backs off... for now.\n";
                if (diLatched) {
                    diLatched = false;
                    latchTimer = 0;
                    diPos = Vector2(diPos.x + random() % 200 - 100, diPos.y + random() % 200 - 100);  // DI backs off further
                }
            }

            if (direction.x != 0 || direction.y != 0 && frameCount % 10 == 0 && !diLatched) {
                recruitFrame = (recruitFrame + 1) % 4;
                if (footsteps) Mix_PlayChannel(-1, footsteps, 0);
            }
            if (!gearCollected && frameCount % 15 == 0 && !diLatched) diFrame = (diFrame + 1) % 2;

            // Use EST-based day/night cycle instead of simple toggle
            float dayNightCycle = gameRenderer->getDayNightFactor();
            if (frameCount % 600 == 0) weatherTimer = 120;
            if (weatherTimer > 0) {
                weatherTimer--;
                if (random() % 5 == 0) gameRenderer->addParticle(Vector2(recruitPos.x + SPRITE_SIZE / 2, recruitPos.y + SPRITE_SIZE / 2));
            }

            for (auto it = gameRenderer->getDustParticles().begin(); it != gameRenderer->getDustParticles().end();) {
                it->y += 1;
                if (it->y > cameraPos.y + HEIGHT || it->x < cameraPos.x || it->x > cameraPos.x + WIDTH) {
                    it = gameRenderer->getDustParticles().erase(it);
                } else {
                    ++it;
                }
            }

            frameCount++;
            lastCatchCheck++;

            // Pass recruitFrame and diFrame to renderScene for animation
            gameRenderer->setCamera(cameraPos);
            gameRenderer->renderScene(recruitPos, diPos, gearPos, gearCollected, stamina, catchCount, frameCount, weatherTimer, dayNightCycle, recruitFrame, diFrame);

            // Text rendering (ensure font and renderer are correct)
            float distanceToDi = std::sqrt((recruitPos.x - diPos.x) * (recruitPos.x - diPos.x) +
                                         (recruitPos.y - diPos.y) * (recruitPos.y - diPos.y));
            if (!gearCollected && distanceToDi < 100 && !diLatched) {
                SDL_Texture* diText = renderText("You look like the monkey off Ace Ventura!", BLACK);
                SDL_Texture* diShadow = renderText("You look like the monkey off Ace Ventura!", WHITE);
                if (diText && diShadow) {
                    int textW, textH;
                    SDL_QueryTexture(diText, NULL, NULL, &textW, &textH);
                    SDL_Rect shadowRect = {12, HEIGHT - 100, textW, textH};
                    SDL_Rect textRect = {10, HEIGHT - 102, textW, textH};
                    SDL_RenderCopy(renderer, diShadow, NULL, &shadowRect);
                    SDL_RenderCopy(renderer, diText, NULL, &textRect);
                    SDL_DestroyTexture(diText);
                    SDL_DestroyTexture(diShadow);
                }
            }
            if (gearCollected) {
                SDL_Texture* missionText = renderText("Mission Complete: Gear Secured!", BLACK);
                SDL_Texture* missionShadow = renderText("Mission Complete: Gear Secured!", WHITE);
                if (missionText && missionShadow) {
                    int textW, textH;
                    SDL_QueryTexture(missionText, NULL, NULL, &textW, &textH);
                    SDL_Rect shadowRect = {12, HEIGHT - 60, textW, textH};
                    SDL_Rect textRect = {10, HEIGHT - 62, textW, textH};
                    SDL_RenderCopy(renderer, missionShadow, NULL, &shadowRect);
                    SDL_RenderCopy(renderer, missionText, NULL, &textRect);
                    SDL_DestroyTexture(missionText);
                    SDL_DestroyTexture(missionShadow);
                }
            }
            std::string catchStr = "Times Caught: " + std::to_string(catchCount) + "/15";
            SDL_Texture* catchText = renderText(catchStr, BLACK);
            SDL_Texture* catchShadow = renderText(catchStr, WHITE);
            if (catchText && catchShadow) {
                int textW, textH;
                SDL_QueryTexture(catchText, NULL, NULL, &textW, &textH);
                SDL_Rect shadowRect = {12, 42, textW, textH};
                SDL_Rect textRect = {10, 40, textW, textH};
                SDL_RenderCopy(renderer, catchShadow, NULL, &shadowRect);
                SDL_RenderCopy(renderer, catchText, NULL, &textRect);
                SDL_DestroyTexture(catchText);
                SDL_DestroyTexture(catchShadow);
            }

            // Display escape prompt while latched
            if (diLatched) {
                SDL_Texture* escapePrompt = renderText("Press Space to Escape!", WHITE);
                if (escapePrompt) {
                    int textW, textH;
                    SDL_QueryTexture(escapePrompt, NULL, NULL, &textW, &textH);
                    SDL_Rect promptRect = {(WIDTH - textW) / 2, HEIGHT - 150, textW, textH};
                    SDL_RenderCopy(renderer, escapePrompt, NULL, &promptRect);
                    SDL_DestroyTexture(escapePrompt);
                }
            }

            SDL_RenderPresent(renderer);  // Ensure all rendering (including text) is displayed

            SDL_Delay(16);  // Cap frame rate (60 FPS)
        }
    }
};

int main(int argc, char* argv[]) {
    ParrisIslandTrials game;
    if (game.isRunning()) game.run();
    return 0;
}
