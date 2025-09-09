#include "raylib.h"
#include "physics_engine.h"
#include "physics/core/rigidbody.h"
#include "physics/core/shape.h"
#include "physics/core/forces/gravity.h"
#include <memory>
#include <vector>

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Physics Sandbox");

    SetTargetFPS(60);

    // Physics Engine Setup
    PhysicsEngine::PhysicsEngine physicsEngine;
    std::vector<PhysicsEngine::RigidBody*> bodies;

    // Create two rectangles
    PhysicsEngine::Rectangle* rectShape1 = new PhysicsEngine::Rectangle(50, 50);
    PhysicsEngine::RigidBody* rect1 = new PhysicsEngine::RigidBody(rectShape1, 1.0f, PhysicsEngine::Vector2{ (float)screenWidth / 4, (float)screenHeight / 2 });
    rect1->velocity = { 10.0f, 100.0f };
    physicsEngine.addBody(rect1);
    bodies.push_back(rect1);

    PhysicsEngine::Rectangle* rectShape2 = new PhysicsEngine::Rectangle(10000, 20);

    PhysicsEngine::RigidBody* rect2 = new PhysicsEngine::RigidBody(rectShape2, 1.0f, PhysicsEngine::Vector2{ (float)screenWidth, (float)screenHeight - 5 }, true);
    physicsEngine.addBody(rect2);
    bodies.push_back(rect2);

    float accumulator = 0.0f;
    const float fixedDeltaTime = 1.0f / 60.0f;

    while (!WindowShouldClose())
    {
        // Update
        accumulator += GetFrameTime();
        while (accumulator >= fixedDeltaTime)
        {
            physicsEngine.step(fixedDeltaTime);
            accumulator -= fixedDeltaTime;
        }

        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        for (const auto& body : bodies)
        {
            PhysicsEngine::Vector2 position = body->GetPosition();
            if (body->shape->type == PhysicsEngine::ShapeType::RECTANGLE)
            {
                PhysicsEngine::Rectangle* rect = static_cast<PhysicsEngine::Rectangle*>(body->shape);
                DrawRectanglePro(
                    { position.x, position.y, rect->GetWidth(), rect->GetHeight() },
                    { rect->GetWidth() / 2, rect->GetHeight() / 2 },
                    body->orientation * RAD2DEG,
                    body->IsStatic() ? DARKGRAY : BLUE
                );
            }
        }

        DrawText("Two rectangles colliding!", 10, 10, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
