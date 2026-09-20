#pragma once
#include "physics/core/forces/gravity.h"
#include "physics/core/world.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace field {
namespace pe = PhysicsEngine;
constexpr float Step = 1.0f / 120.0f;
constexpr std::size_t MaxBodies = 250;
enum class Shape { Circle, Box, Triangle };
struct BodySpec {
    int id = 0;
    std::string name = "Body";
    Shape shape = Shape::Circle;
    float x = 10, y = 2, width = 1.2f, height = 1.2f;
    float angle = 0, vx = 0, vy = 0, spin = 0;
    float density = 1, bounce = .45f, friction = .45f;
    bool fixed = false;
    int color = 0;
};
struct Scene {
    std::string name = "Untitled experiment";
    float gravity = 9.81f;
    std::vector<BodySpec> bodies;
};
struct Entity {
    BodySpec spec;
    std::unique_ptr<pe::Shape> shape;
    std::shared_ptr<pe::RigidBody> body;
    std::vector<pe::Vector2> trail;
};
class Simulation {
  public:
    Simulation();
    void restore(const Scene &scene);
    Scene snapshot() const;
    Entity *find(int id);
    int add(BodySpec spec);
    void erase(int id);
    void replace(int id, BodySpec spec);
    void setGravity(float value);
    void step();
    double kineticEnergy() const;
    std::vector<Entity> &entities() { return entities_; }
    const std::vector<Entity> &entities() const { return entities_; }
    pe::SimulationStatistics statistics() const { return world_->getLastStepStatistics(); }
    std::string name;
    float gravity = 9.81f;
    double time = 0;

  private:
    // World is destroyed before shape owners (reverse member destruction order).
    std::vector<Entity> entities_;
    std::unique_ptr<pe::World> world_;
    pe::Gravity *gravityForce_ = nullptr;
    int nextId_ = 1;
    std::size_t ticks_ = 0;
    void createWorld();
};
class History {
  public:
    void remember(const Scene &current);
    bool undo(Simulation &sim);
    bool redo(Simulation &sim);
    bool canUndo() const { return !past_.empty(); }
    bool canRedo() const { return !future_.empty(); }

  private:
    std::vector<Scene> past_, future_;
};
BodySpec currentSpec(const Entity &entity);
Scene preset(int index);
const char *presetName(int index);
const char *presetDescription(int index);
void validate(const Scene &scene);
void saveScene(const Scene &scene, const std::filesystem::path &path);
void exportMeasurements(const Simulation &sim, const std::filesystem::path &path);
Scene loadScene(const std::filesystem::path &path);
int hitTest(const Simulation &sim, pe::Vector2 point);
} // namespace field
