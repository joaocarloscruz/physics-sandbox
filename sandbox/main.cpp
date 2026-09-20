#include "raylib.h"
#include "physics/core/world.h"
#include "physics/core/forces/gravity.h"
#include <algorithm>
#include <memory>

int main() {
    namespace pe = PhysicsEngine;
    InitWindow(960, 540, "Physics Sandbox");
    SetTargetFPS(60);
    // Shapes outlive the bodies and the world that refer to them.
    auto box = pe::Polygon::MakeBox(1, 1);
    auto ground = pe::Polygon::MakeBox(18, .5f);
    pe::Material material{1, .35f, .6f, .4f};
    auto falling = std::make_shared<pe::RigidBody>(&box, material, pe::Vector2{4, 2});
    auto floor = std::make_shared<pe::RigidBody>(&ground, material, pe::Vector2{8, 7}, true);
    pe::World world;
    world.addBody(falling);
    world.addBody(floor);
    world.addUniversalForce(std::make_unique<pe::Gravity>(pe::Vector2{0, 9.81f}));
    constexpr float step = 1.0f / 120.0f;
    float accumulator = 0;
    while (!WindowShouldClose()) {
        accumulator += std::min(GetFrameTime(), .1f);
        while (accumulator >= step) {
            world.step(step);
            accumulator -= step;
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawRectanglePro({falling->position.x * 60, falling->position.y * 60, 60, 60},
                         {30, 30}, falling->orientation * RAD2DEG, BLUE);
        DrawRectanglePro({floor->position.x * 60, floor->position.y * 60, 1080, 30},
                         {540, 15}, 0, DARKGRAY);
        DrawText("Physics engine demo", 20, 20, 22, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
