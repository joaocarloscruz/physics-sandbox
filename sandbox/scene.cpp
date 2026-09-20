#include "scene.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <stdexcept>

namespace field {
BodySpec currentSpec(const Entity &e) {
    BodySpec s = e.spec;
    s.x = e.body->position.x;
    s.y = e.body->position.y;
    s.angle = e.body->orientation;
    s.vx = e.body->velocity.x;
    s.vy = e.body->velocity.y;
    s.spin = e.body->angularVelocity;
    return s;
}
static void bounded(float v, float min, float max) {
    if (!std::isfinite(v) || v < min || v > max)
        throw std::runtime_error("Scene contains an invalid or out-of-range value.");
}
void validate(const Scene &s) {
    if (s.name.empty() || s.name.size() > 64 || s.bodies.size() > MaxBodies)
        throw std::runtime_error("Scene name or body count is invalid (maximum 250).");
    bounded(s.gravity, -25, 25);
    std::set<int> ids;
    for (const auto &b : s.bodies) {
        if (b.id <= 0 || b.id > 1000000 || !ids.insert(b.id).second || b.name.empty() || b.name.size() > 64 ||
            static_cast<int>(b.shape) < 0 || static_cast<int>(b.shape) > 2 || b.color < 0 || b.color > 4)
            throw std::runtime_error("Invalid body metadata or duplicate IDs.");
        bounded(b.x, -10000, 10000);
        bounded(b.y, -10000, 10000);
        bounded(b.width, .15f, 50);
        bounded(b.height, .15f, 50);
        bounded(b.angle, -1000000, 1000000);
        bounded(b.vx, -30, 30);
        bounded(b.vy, -30, 30);
        bounded(b.spin, -30, 30);
        bounded(b.density, .05f, 20);
        bounded(b.bounce, 0, 1);
        bounded(b.friction, 0, 1);
    }
}
Simulation::Simulation() { createWorld(); }
void Simulation::createWorld() {
    pe::SimulationConfig config;
    config.fixedTimeStep = Step;
    config.solverIterations = 16;
    config.maxLinearSpeed = 30;
    world_ = std::make_unique<pe::World>(config);
    auto force = std::make_unique<pe::Gravity>(pe::Vector2{0, gravity});
    gravityForce_ = force.get();
    world_->addUniversalForce(std::move(force));
}
void Simulation::restore(const Scene &scene) {
    validate(scene);
    world_.reset();
    entities_.clear();
    nextId_ = 1;
    ticks_ = 0;
    name = scene.name;
    gravity = scene.gravity;
    time = 0;
    createWorld();
    for (auto s : scene.bodies)
        add(s);
}
Scene Simulation::snapshot() const {
    Scene scene{name, gravity, {}};
    for (const auto &e : entities_)
        scene.bodies.push_back(currentSpec(e));
    return scene;
}
Entity *Simulation::find(int id) {
    auto it =
        std::find_if(entities_.begin(), entities_.end(), [id](const auto &e) { return e.spec.id == id; });
    return it == entities_.end() ? nullptr : &*it;
}
int Simulation::add(BodySpec s) {
    if (entities_.size() >= MaxBodies)
        throw std::runtime_error("Body limit reached (250). Delete a body to add another.");
    if (!s.id)
        s.id = nextId_;
    if (find(s.id))
        throw std::runtime_error("Duplicate body ID.");
    validate(Scene{"Body validation", gravity, {s}});
    Entity e;
    e.spec = s;
    if (s.shape == Shape::Circle)
        e.shape = std::make_unique<pe::Circle>(s.width / 2);
    else if (s.shape == Shape::Box)
        e.shape = std::make_unique<pe::Polygon>(pe::Polygon::MakeBox(s.width, s.height));
    else
        e.shape = std::make_unique<pe::Polygon>(pe::Polygon::MakeTriangle(
            {-s.width / 2, s.height / 3}, {0, -2 * s.height / 3}, {s.width / 2, s.height / 3}));
    pe::Material mat{s.density, s.bounce, s.friction, s.friction * .8f};
    e.body = std::make_shared<pe::RigidBody>(e.shape.get(), mat, pe::Vector2{s.x, s.y}, s.fixed);
    e.body->orientation = s.angle;
    e.body->previousOrientation = s.angle;
    e.body->velocity = s.fixed ? pe::Vector2{0, 0} : pe::Vector2{s.vx, s.vy};
    e.body->angularVelocity = s.fixed ? 0 : s.spin;
    world_->addBody(e.body);
    entities_.push_back(std::move(e));
    nextId_ = std::max(nextId_, s.id + 1);
    return s.id;
}
void Simulation::erase(int id) {
    auto it =
        std::find_if(entities_.begin(), entities_.end(), [id](const auto &e) { return e.spec.id == id; });
    if (it == entities_.end())
        return;
    world_->removeBody(it->body);
    entities_.erase(it);
}
void Simulation::replace(int id, BodySpec s) {
    s.id = id;
    validate(Scene{"Body validation", gravity, {s}});
    erase(id);
    add(s);
}
void Simulation::setGravity(float v) {
    bounded(v, -25, 25);
    gravity = v;
    gravityForce_->setGravity({0, v});
}
void Simulation::step() {
    world_->step();
    time += Step;
    ++ticks_;
    if (ticks_ % 4 == 0)
        for (auto &e : entities_) {
            if (e.spec.fixed)
                continue;
            if (e.trail.size() >= 90)
                e.trail.erase(e.trail.begin());
            e.trail.push_back(e.body->position);
        }
}
double Simulation::kineticEnergy() const {
    double energy = 0;
    for (const auto &e : entities_)
        energy += .5 * e.body->mass * e.body->velocity.magnitudeSquared() +
                  .5 * e.body->inertia * e.body->angularVelocity * e.body->angularVelocity;
    return energy;
}
void History::remember(const Scene &current) {
    if (past_.size() == 64)
        past_.erase(past_.begin());
    past_.push_back(current);
    future_.clear();
}
bool History::undo(Simulation &sim) {
    if (past_.empty())
        return false;
    future_.push_back(sim.snapshot());
    sim.restore(past_.back());
    past_.pop_back();
    return true;
}
bool History::redo(Simulation &sim) {
    if (future_.empty())
        return false;
    past_.push_back(sim.snapshot());
    sim.restore(future_.back());
    future_.pop_back();
    return true;
}
const char *presetName(int i) {
    const char *names[] = {"Gravity garden", "Domino effect", "Bounce lab", "Collision course",
                           "Blank canvas"};
    return names[std::clamp(i, 0, 4)];
}
const char *presetDescription(int i) {
    const char *text[] = {"A little world of cause and effect.", "One nudge. A chain reaction.",
                          "Same height. Different restitution.", "Explore momentum in zero gravity.",
                          "Your next experiment starts here."};
    return text[std::clamp(i, 0, 4)];
}
Scene preset(int index) {
    Scene s;
    s.name = presetName(index);
    auto add = [&](Shape shape, float x, float y, float w, float h, int color, bool fixed = false,
                   float angle = 0) -> BodySpec & {
        BodySpec b;
        b.id = static_cast<int>(s.bodies.size()) + 1;
        b.shape = shape;
        b.name = fixed                    ? "Platform"
                 : shape == Shape::Circle ? "Circle"
                 : shape == Shape::Box    ? "Box"
                                          : "Triangle";
        b.x = x;
        b.y = y;
        b.width = w;
        b.height = h;
        b.color = color;
        b.fixed = fixed;
        b.angle = angle;
        s.bodies.push_back(b);
        return s.bodies.back();
    };
    add(Shape::Box, 10, 10.8f, 22, .6f, 4, true).name = "Ground";
    if (index == 0) {
        add(Shape::Box, 5.5f, 6.4f, 6, .35f, 4, true, .22f).name = "Upper ramp";
        add(Shape::Box, 14.5f, 8, 5.5f, .35f, 4, true, -.25f).name = "Lower ramp";
        add(Shape::Circle, 4, 2.6f, 1.5f, 1.5f, 0).name = "Mint ball";
        add(Shape::Circle, 6.5f, 1.4f, .85f, .85f, 1).name = "Apricot ball";
        add(Shape::Box, 10, 9.7f, 1.5f, 1.5f, 2, false, -.12f);
        add(Shape::Box, 10, 8.1f, 1.25f, 1.25f, 0, false, .1f);
        add(Shape::Triangle, 14.5f, 3.5f, 1.8f, 1.6f, 3);
        add(Shape::Circle, 17.5f, 5.5f, 1.1f, 1.1f, 1).bounce = .85f;
    } else if (index == 1) {
        for (int n = 0; n < 12; ++n) {
            auto &b = add(Shape::Box, 3.f + n * 1.2f, 9.5f, .32f, 1.9f, n % 4);
            b.bounce = .05f;
            b.friction = .6f;
        }
        auto &b = add(Shape::Circle, 1.2f, 8.6f, 1, 1, 1);
        b.vx = 4;
    } else if (index == 2) {
        for (int n = 0; n < 4; ++n) {
            auto &b = add(Shape::Circle, 4.f + n * 4, 2, 1.4f, 1.4f, n);
            b.bounce = n / 3.f;
            b.name = "Bounce " + std::to_string(n + 1);
        }
        s.bodies[0].bounce = 1;
    } else if (index == 3) {
        s.gravity = 0;
        auto &a = add(Shape::Circle, 5, 5, 1.6f, 1.6f, 0);
        a.vx = 3;
        a.bounce = 1;
        a.friction = 0;
        auto &b = add(Shape::Circle, 15, 5, 1.6f, 1.6f, 1);
        b.vx = -3;
        b.bounce = 1;
        b.friction = 0;
    }
    return s;
}
int hitTest(const Simulation &sim, pe::Vector2 p) {
    for (auto it = sim.entities().rbegin(); it != sim.entities().rend(); ++it) {
        auto d = p - it->body->position;
        float a = -it->body->orientation;
        pe::Vector2 local{std::cos(a) * d.x - std::sin(a) * d.y, std::sin(a) * d.x + std::cos(a) * d.y};
        if (it->spec.shape == Shape::Circle) {
            if (local.magnitudeSquared() <= it->spec.width * it->spec.width / 4)
                return it->spec.id;
        } else {
            const auto &v = static_cast<pe::Polygon *>(it->shape.get())->getVertices();
            bool positive = false, negative = false;
            for (std::size_t i = 0; i < v.size(); ++i) {
                float cross = (v[(i + 1) % v.size()] - v[i]).cross(local - v[i]);
                positive |= cross > .0001f;
                negative |= cross < -.0001f;
            }
            if (!(positive && negative))
                return it->spec.id;
        }
    }
    return 0;
}
void saveScene(const Scene &scene, const std::filesystem::path &path) {
    validate(scene);
    auto tmp = path;
    tmp += ".tmp";
    std::ofstream out(tmp, std::ios::trunc);
    if (!out)
        throw std::runtime_error("Cannot write here. Move the sandbox to a writable folder.");
    out << std::setprecision(9) << "FIELD_SCENE 1\n"
        << std::quoted(scene.name) << '\n'
        << scene.gravity << ' ' << scene.bodies.size() << '\n';
    for (const auto &b : scene.bodies)
        out << b.id << ' ' << std::quoted(b.name) << ' ' << static_cast<int>(b.shape) << ' ' << b.x << ' '
            << b.y << ' ' << b.width << ' ' << b.height << ' ' << b.angle << ' ' << b.vx << ' ' << b.vy << ' '
            << b.spin << ' ' << b.density << ' ' << b.bounce << ' ' << b.friction << ' ' << b.fixed << ' '
            << b.color << '\n';
    out.close();
    if (!out)
        throw std::runtime_error("Scene save failed. Check disk space.");
    auto backup = path;
    backup += ".bak";
    bool exists = std::filesystem::exists(path);
    if (exists) {
        if (std::filesystem::exists(backup))
            std::filesystem::remove(backup);
        std::filesystem::rename(path, backup);
    }
    try {
        std::filesystem::rename(tmp, path);
    } catch (...) {
        if (exists)
            std::filesystem::rename(backup, path);
        throw;
    }
}
Scene loadScene(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Scene file is too large.");
    std::ifstream in(path);
    std::string magic;
    int version = 0;
    Scene scene;
    std::size_t count = 0;
    if (!(in >> magic >> version) || magic != "FIELD_SCENE" || version != 1 ||
        !(in >> std::quoted(scene.name) >> scene.gravity >> count) || count > MaxBodies)
        throw std::runtime_error("This is not a supported Field scene.");
    for (std::size_t n = 0; n < count; ++n) {
        BodySpec b;
        int shape = -1;
        if (!(in >> b.id >> std::quoted(b.name) >> shape >> b.x >> b.y >> b.width >> b.height >> b.angle >>
              b.vx >> b.vy >> b.spin >> b.density >> b.bounce >> b.friction >> b.fixed >> b.color))
            throw std::runtime_error("The scene is incomplete or damaged.");
        b.shape = static_cast<Shape>(shape);
        scene.bodies.push_back(b);
    }
    in >> std::ws;
    if (!in.eof())
        throw std::runtime_error("Unexpected data at the end of the scene.");
    validate(scene);
    return scene;
}
void exportMeasurements(const Simulation &sim, const std::filesystem::path &path) {
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("Cannot write measurements to this folder.");
    out << "time_s,id,name,shape,fixed,x_m,y_m,vx_m_s,vy_m_s,speed_m_s,angle_rad,spin_rad_s,mass_kg,kinetic_"
           "energy_J\n";
    out << std::setprecision(9);
    for (const auto &e : sim.entities()) {
        const auto b = currentSpec(e);
        std::string name = b.name;
        // Quoting CSV fields does not prevent spreadsheet formula evaluation.
        if (!name.empty() && std::string("=+-@\t\r\n").find(name[0]) != std::string::npos)
            name = "'" + name;
        std::string quoted = "\"";
        for (char c : name) {
            if (c == '"')
                quoted += '"';
            quoted += c;
        }
        quoted += '"';
        const char *shape = b.shape == Shape::Circle ? "circle" : b.shape == Shape::Box ? "box" : "triangle";
        double energy =
            .5 * e.body->mass * e.body->velocity.magnitudeSquared() + .5 * e.body->inertia * b.spin * b.spin;
        out << sim.time << ',' << b.id << ',' << quoted << ',' << shape << ',' << b.fixed << ',' << b.x << ','
            << b.y << ',' << b.vx << ',' << b.vy << ',' << e.body->velocity.magnitude() << ',' << b.angle
            << ',' << b.spin << ',' << e.body->mass << ',' << energy << '\n';
    }
    out.close();
    if (!out)
        throw std::runtime_error("Measurement export failed. Check disk space.");
}
} // namespace field
