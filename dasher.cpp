#pragma GCC diagnostic ignored "-Wnarrowing"

//#define PLATFORM_WEB

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "raylib.h"
#include <cstdio>
const int screenWidth{800};
const int screenHeight{450};
const int gravity{1'000};
const int jumpVel{-600};

void UpdateDrawFrame(void); // Update and Draw one frame

struct AnimData
{
    Rectangle rec;
    Vector2 pos;
    int frame;
    float updateTime;
    float runningTime;
};

bool isOnGround(const AnimData &data) //Pass reference to data which will not be changable, and returns bool
{
    return data.pos.y >= screenHeight - data.rec.height;
}

void updateAnimDataWithNewFrame(AnimData &data, const float spriteWidth, const float deltaTime, const int SpriteRows, const int SpriteColumns, const bool SingleRowSprite = true, const int OffsetOnLastRow = 0)
{
    if (data.runningTime >= data.updateTime)
    {
        data.frame++;
        if (SingleRowSprite)
        {
            data.rec.x = data.frame * spriteWidth;
            if (data.frame > SpriteColumns - 1) //-1 to make it 0-based
            {
                data.frame = 0;
            }
        }
        else
        {
            data.rec.x = data.frame % SpriteRows * spriteWidth;
            data.rec.y = data.frame / SpriteRows * data.rec.height;
            if (data.frame >= (SpriteRows * SpriteColumns - OffsetOnLastRow))
            {
                data.frame = 0;
            }
        }

        if (data.frame > SpriteColumns - 1) //-1 to make it 0-based
        {
            data.frame = 0;
        }

        data.runningTime = 0;
    }

    data.runningTime += deltaTime;
}

float calculateBackgroundScale(const Texture2D &texture)
{
    return texture.width > screenWidth ? (float)texture.width / (float)screenWidth : (float)screenWidth / (float)texture.width;
}

float updateBackgroundAndReturnNewXOffset(const Texture2D &texture, float XOffset, const float scale, const float deltaTime, const float speed)
{
    XOffset -= speed * deltaTime;
    if (XOffset <= -texture.width * scale)
    {
        XOffset = 0;
    }

    // Draw background
    Vector2 bgPosPart1{XOffset, screenHeight - texture.height * scale};
    DrawTextureEx(texture, bgPosPart1, 0, scale, WHITE);
    Vector2 bgPosPart2{XOffset + texture.width * scale, screenHeight - texture.height * scale};
    DrawTextureEx(texture, bgPosPart2, 0, scale, WHITE);

    return XOffset;
}

int main(void)
{
    // Initialization

    InitWindow(screenWidth, screenHeight, "Dasher");

    // Scarfy velocity (pixels per second)
    int velocity{0};

    //nebula x velocity (pixels per second)
    int nebVal{-200};

    Texture2D background = LoadTexture("textures/far-buildings.png");
    float backgroundScale = calculateBackgroundScale(background);
    float bgX{};

    Texture2D midground = LoadTexture("textures/back-buildings.png");
    float midgroundScale = calculateBackgroundScale(midground);
    float bgXmid{};

    Texture2D foreground = LoadTexture("textures/foreground.png");
    float foregroundScale = calculateBackgroundScale(foreground);
    float bgXfore{};

    float finishLine{screenWidth * 5};

    //nebula variables
    Texture2D nebula = LoadTexture("textures/12_nebula_spritesheet.png");
    int nebulaWidth{nebula.width / 8};
    int nebulaHeight{nebula.height / 8};
    float nebulaPad{42};

    //AnimData for nebula;
    AnimData nebulaData{
        {0, 0, nebulaWidth, nebulaHeight},                        //Rectangle
        {screenWidth + nebulaWidth, screenHeight - nebulaHeight}, //Vector2
        0,                                                        // Frame
        0.0833f,                                                  // Update time
        0.0f                                                      // Running time
    };

    //nebula variables end

    //scarfy variables
    Texture2D scarfy = LoadTexture("textures/scarfy.png");
    int scarfyWidth{scarfy.width / 6};
    AnimData scarfyData{
        {0, 0, scarfyWidth, scarfy.height},                                        //Rectangle
        {screenWidth / 2 - scarfyWidth / 2, screenHeight / 2 - scarfy.height / 2}, //Vector2
        0,                                                                         //Frame
        0.0833f,                                                                   //Update Time
        0.0f                                                                       //Running Time
    };

    bool isJumping{false};
    bool lost{false};
    bool win{false};

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
#endif
    while (!WindowShouldClose())
    {
        //delta time
        float dt = GetFrameTime();

        // Update
        // NOTE: Screen clear is done before drawing
        BeginDrawing();
        ClearBackground(RAYWHITE);

        bgX = updateBackgroundAndReturnNewXOffset(background, bgX, backgroundScale, dt, 5);
        bgXmid = updateBackgroundAndReturnNewXOffset(midground, bgXmid, midgroundScale, dt, 10);
        bgXfore = updateBackgroundAndReturnNewXOffset(foreground, bgXfore, foregroundScale, dt, 20);

        //Draw
        if (isOnGround(scarfyData))
        {
            velocity = 0;
            isJumping = false;
        }
        else
        {
            velocity += gravity * dt;
            isJumping = true;
        }

        //Jump
        if (IsKeyPressed(KEY_SPACE) && !isJumping)
        {
            velocity += jumpVel;
        }

        //Move finish line to the left
        finishLine += nebVal * dt;

        if (!lost)
        {
            // update nebula position
            nebulaData.pos.x += nebVal * dt;

            // update scarfy position
            scarfyData.pos.y += velocity * dt;

            if (!isJumping)
            {
                updateAnimDataWithNewFrame(scarfyData, scarfyWidth, dt, 1, 6);
            }

            if (!win)
            {
                updateAnimDataWithNewFrame(nebulaData, nebulaWidth, dt, 8, 8, false, 4);

                if (nebulaData.pos.x < -nebulaWidth)
                {
                    nebulaData.pos.x = screenWidth + nebulaWidth;
                }

                //Draw nebula
                DrawTextureRec(nebula, nebulaData.rec, nebulaData.pos, WHITE);

                Rectangle nebCollisionRec{nebulaData.pos.x + nebulaPad, nebulaData.pos.y + nebulaPad, nebulaWidth - 2 * nebulaPad, nebulaHeight - 2 * nebulaPad};
                Rectangle scarfyCollisionRec{scarfyData.pos.x, scarfyData.pos.y, scarfyWidth, scarfy.height};
                if (CheckCollisionRecs(nebCollisionRec, scarfyCollisionRec) || lost)
                {
                    lost = true;
                }
            }

            //Draw scarfy
                DrawTextureRec(scarfy, scarfyData.rec, scarfyData.pos, WHITE);
        }

        if (lost)
        {
            DrawText("DEAD", screenWidth / 2 - MeasureText("DEAD", 20), screenHeight / 2, 20, WHITE);
        }
        else if (scarfyData.pos.x > finishLine)
        {
            DrawText("WIN", screenWidth / 2 - MeasureText("WIN", 20), screenHeight / 2, 20, WHITE);
            win = true;
        }

        EndDrawing();
    }
    UnloadTexture(scarfy);
    UnloadTexture(nebula);
    UnloadTexture(background);
    UnloadTexture(midground);
    UnloadTexture(foreground);
    CloseWindow();
}

void UpdateDrawFrame(void)
{
}
