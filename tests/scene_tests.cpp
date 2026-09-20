#include "scene.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace field;
static int checks = 0;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> static void rejects(F fn, const char *message) {
    bool rejected = false;
    try {
        fn();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, message);
}
int main() {
    try {
        Simulation sim;
        Scene freefall;
        BodySpec b;
        b.id = 1;
        b.x = 0;
        b.y = 0;
        freefall.bodies = {b};
        sim.restore(freefall);
        for (int i = 0; i < 120; ++i)
            sim.step();
        check(std::abs(sim.find(1)->body->position.y - 4.905f) < .002f,
              "Free fall position must match 1/2 g t squared");
        check(std::abs(sim.find(1)->body->velocity.y - 9.81f) < .002f, "Free fall velocity must match g t");
        sim.restore(preset(3));
        for (int i = 0; i < 250; ++i)
            sim.step();
        check(sim.find(2)->body->velocity.x < -2.8f && sim.find(3)->body->velocity.x > 2.8f,
              "Elastic collision must exchange velocities");
        check(std::abs(sim.find(2)->body->velocity.x + sim.find(3)->body->velocity.x) < .01f,
              "Collision momentum must be conserved");
        for (int p = 0; p < 5; ++p) {
            sim.restore(preset(p));
            for (int i = 0; i < 1200; ++i)
                sim.step();
            for (const auto &e : sim.entities()) {
                check(std::isfinite(e.body->position.x) && std::isfinite(e.body->position.y) &&
                          std::isfinite(e.body->orientation),
                      "Preset must stay finite");
                if (e.spec.fixed)
                    check(e.body->position.x == e.spec.x && e.body->position.y == e.spec.y,
                          "Fixed geometry must not move");
                if (p == 2)
                    check(e.body->position.y < 11, "Bounce lab balls must not pass through the ground");
            }
        }
        sim.restore(preset(0));
        auto initial = sim.snapshot();
        History history;
        history.remember(initial);
        sim.erase(4);
        check(!sim.find(4), "Deleting a body must remove it");
        check(history.undo(sim) && sim.find(4), "Undo must restore removed body");
        check(history.redo(sim) && !sim.find(4), "Redo must reapply removal");
        for (int i = 0; i < 100; ++i) {
            auto copy = initial.bodies[3];
            copy.id = 0;
            int id = sim.add(copy);
            sim.erase(id);
            sim.step();
        }
        check(sim.entities().size() == initial.bodies.size() - 1,
              "Repeated body creation must not leave ghost bodies");
        sim.restore(initial);
        auto shape = currentSpec(*sim.find(4));
        shape.shape = Shape::Box;
        shape.angle = .7f;
        shape.width = 2;
        shape.height = .5f;
        sim.replace(4, shape);
        check(hitTest(sim, {shape.x, shape.y}) == 4, "Rotated body must be selectable at its center");
        check(hitTest(sim, {shape.x + 1.4f, shape.y - 1.4f}) != 4,
              "Hit testing must exclude outside rotated geometry");
        auto path = std::filesystem::temp_directory_path() / "field-scene-integration.field";
        saveScene(sim.snapshot(), path);
        auto roundtrip = loadScene(path);
        check(roundtrip.bodies.size() == sim.entities().size(), "Save round trip must preserve bodies");
        check(roundtrip.bodies.back().angle == shape.angle, "Save round trip must preserve rotation");
        roundtrip.name = "A second save";
        saveScene(roundtrip, path);
        check(loadScene(path).name == "A second save", "Save must replace existing scene");
        auto backup = path;
        backup += ".bak";
        check(loadScene(backup).name == initial.name, "Overwrite must retain recovery backup");
        auto csv = path;
        csv += ".csv";
        exportMeasurements(sim, csv);
        {
            std::ifstream in(csv);
            std::string row;
            std::getline(in, row);
            check(row.find("kinetic_energy_J") != std::string::npos,
                  "CSV must include units and kinetic energy");
            std::size_t rows = 0;
            while (std::getline(in, row))
                ++rows;
            check(rows == sim.entities().size(), "CSV must contain one row per body");
        }
        std::filesystem::remove(csv);
        auto bad = initial;
        bad.bodies[0].width = 0;
        rejects([&] { sim.restore(bad); }, "Zero-size shape must be rejected");
        check(sim.entities().size() == initial.bodies.size(),
              "Invalid restore must leave live scene untouched");
        bad = initial;
        bad.bodies[1].id = bad.bodies[0].id;
        rejects([&] { validate(bad); }, "Duplicate IDs must be rejected");
        bad = initial;
        bad.gravity = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { validate(bad); }, "NaN must be rejected");
        bad = initial;
        bad.bodies[1].bounce = 2;
        rejects([&] { validate(bad); }, "Invalid material must be rejected");
        {
            std::ofstream out(path);
            out << "FIELD_SCENE 1\n\"Broken\"\n9.81 999999999\n";
        }
        rejects([&] { loadScene(path); }, "Oversized scene count must be rejected before allocation");
        {
            std::ofstream out(path);
            out << "FIELD_SCENE 1\n\"Broken\"\n9.81 1\n";
        }
        rejects([&] { loadScene(path); }, "Truncated files must be rejected");
        std::filesystem::remove(path);
        std::filesystem::remove(backup);
        std::cout << "PASS: " << checks
                  << " integration checks; five presets simulated for 10 seconds each.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
