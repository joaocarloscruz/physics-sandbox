#include "ui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

namespace field {
namespace {
constexpr Color Ink{34, 48, 49, 255}, Muted{112, 126, 128, 255}, Line{224, 231, 230, 255};
constexpr Color Paper{255, 255, 253, 255}, Back{245, 248, 246, 255};
constexpr Color Teal{29, 115, 99, 255}, Mint{229, 244, 237, 255};
constexpr Color Palette[] = {{138, 207, 182, 255},
                             {244, 182, 127, 255},
                             {143, 183, 223, 255},
                             {185, 165, 216, 255},
                             {152, 167, 164, 255}};
constexpr Color Edges[] = {
    {50, 131, 105, 255}, {179, 109, 46, 255}, {64, 109, 157, 255}, {123, 97, 161, 255}, {99, 120, 114, 255}};
void panel(Rectangle r, Color color, float round = .16f) { DrawRectangleRounded(r, round, 8, color); }
std::string format(float v, int digits = 2) {
    char out[64];
    std::snprintf(out, sizeof(out), "%.*f", digits, v);
    return out;
}
bool contains(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }
float clamp(float x, float lo, float hi) { return std::clamp(x, lo, hi); }
} // namespace
App::App() {
    font = LoadFontEx("assets/Lato-Regular.ttf", 48, nullptr, 0);
    if (!font.texture.id)
        font = GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    sim.restore(preset(0));
    baseline = sim.snapshot();
}
App::~App() {
    if (font.texture.id != GetFontDefault().texture.id)
        UnloadFont(font);
}
void App::text(const std::string &s, float x, float y, float size, Color c) {
    DrawTextEx(font, s.c_str(), {x, y}, size, .2f, c);
}
float App::textWidth(const std::string &s, float size) { return MeasureTextEx(font, s.c_str(), size, .2f).x; }
void App::label(const std::string &s, float x, float y) { text(s, x, y, 12, Muted); }
void App::icon(int kind, float x, float y, float s, Color c) {
    auto line = [&](float x1, float y1, float x2, float y2) {
        DrawLineEx({x + x1 * s, y + y1 * s}, {x + x2 * s, y + y2 * s}, 1.8f, c);
    };
    switch (kind) {
    case 0:
        line(.15f, .1f, .35f, .85f);
        line(.15f, .1f, .85f, .55f);
        line(.35f, .85f, .5f, .58f);
        line(.5f, .58f, .85f, .55f);
        break;
    case 1:
        DrawCircleLinesV({x + s / 2, y + s / 2}, s * .35f, c);
        break;
    case 2:
        DrawRectangleLinesEx({x + s * .17f, y + s * .17f, s * .66f, s * .66f}, 1.8f, c);
        break;
    case 3:
        line(.5f, .1f, .1f, .85f);
        line(.1f, .85f, .9f, .85f);
        line(.9f, .85f, .5f, .1f);
        break;
    case 4:
        line(.1f, .5f, .9f, .5f);
        for (int n = 0; n < 4; ++n)
            line(.2f + n * .2f, .5f, .1f + n * .2f, .8f);
        break;
    case 5:
        line(.1f, .8f, .9f, .2f);
        line(.9f, .2f, .5f, .2f);
        line(.9f, .2f, .8f, .6f);
        break;
    case 6:
        DrawTriangle({x + s * .25f, y + s * .15f}, {x + s * .25f, y + s * .85f}, {x + s * .85f, y + s * .5f},
                     c);
        break;
    case 7:
        line(.3f, .2f, .3f, .8f);
        line(.7f, .2f, .7f, .8f);
        break;
    case 8:
        DrawCircleSectorLines({x + s * .53f, y + s * .54f}, s * .33f, 40, 320, 20, c);
        line(.25f, .2f, .25f, .48f);
        line(.25f, .48f, .52f, .42f);
        break;
    case 9:
        DrawTriangle({x + s * .15f, y + s * .15f}, {x + s * .15f, y + s * .85f}, {x + s * .65f, y + s * .5f},
                     c);
        line(.82f, .15f, .82f, .85f);
        break;
    case 10:
        line(.8f, .75f, .8f, .45f);
        line(.8f, .45f, .2f, .45f);
        line(.2f, .45f, .42f, .22f);
        line(.2f, .45f, .42f, .68f);
        break;
    case 11:
        line(.2f, .75f, .2f, .45f);
        line(.2f, .45f, .8f, .45f);
        line(.8f, .45f, .58f, .22f);
        line(.8f, .45f, .58f, .68f);
        break;
    case 12:
        line(.25f, .15f, .75f, .15f);
        line(.15f, .3f, .85f, .3f);
        line(.25f, .3f, .3f, .85f);
        line(.75f, .3f, .7f, .85f);
        line(.3f, .85f, .7f, .85f);
        break;
    case 13:
        DrawRectangleLinesEx({x + s * .3f, y + s * .3f, s * .55f, s * .55f}, 1.7f, c);
        line(.1f, .65f, .1f, .1f);
        line(.1f, .1f, .65f, .1f);
        break;
    case 14:
        line(.5f, .15f, .5f, .85f);
        line(.15f, .5f, .85f, .5f);
        break;
    default:
        line(.15f, .5f, .85f, .5f);
        break;
    }
}
bool App::button(Rectangle r, const std::string &title, bool active, const std::string &tip, int symbol,
                 bool enabled) {
    bool hover = contains(r) && !blocked && enabled;
    if (hover) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        tooltip = tip;
    }
    panel(r, active ? Teal : hover ? Mint : Paper, .22f);
    if (!active)
        DrawRectangleRoundedLinesEx(r, .22f, 8, 1, Line);
    Color c = !enabled ? Color{179, 189, 187, 255} : active ? WHITE : Ink;
    float width = textWidth(title, 14), gap = symbol >= 0 && !title.empty() ? 9.f : 0;
    float start = r.x + (r.width - width - (symbol >= 0 ? 19 : 0) - gap) / 2;
    if (symbol >= 0)
        icon(symbol, start, r.y + (r.height - 19) / 2, 19, c);
    if (!title.empty())
        text(title, start + (symbol >= 0 ? 19 + gap : 0), r.y + (r.height - 15) / 2, 14, c);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
bool App::number(const std::string &id, const std::string &title, Rectangle r, float &value, float lo,
                 float hi, const char *unit) {
    label(title, r.x, r.y);
    r.y += 18;
    r.height = 33;
    bool active = editId == id;
    panel(r, active ? Mint : Back, .17f);
    DrawRectangleRoundedLinesEx(r, .17f, 8, 1, active ? Teal : Line);
    if (contains(r) && !blocked) {
        SetMouseCursor(MOUSE_CURSOR_IBEAM);
        tooltip = "Type a value, then Enter. Escape cancels.";
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            editId = id;
            editBuffer = format(value, 3);
            active = true;
        }
    }
    bool changed = false;
    if (active && !blocked) {
        int c;
        while ((c = GetCharPressed()) > 0) {
            if (((c >= '0' && c <= '9') || c == '.' || c == '-') && editBuffer.size() < 14)
                editBuffer += char(c);
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A))
            editBuffer.clear();
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
            if (!editBuffer.empty())
                editBuffer.pop_back();
        if (IsKeyPressed(KEY_ESCAPE)) {
            editId.clear();
            active = false;
        }
        if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !contains(r))) {
            try {
                std::size_t used;
                float v = std::stof(editBuffer, &used);
                if (used != editBuffer.size() || !std::isfinite(v))
                    throw std::runtime_error("number");
                value = clamp(v, lo, hi);
                changed = true;
            } catch (...) {
                notify("Enter a valid number. The previous value was kept.");
            }
            editId.clear();
            active = false;
        }
    }
    text(active ? editBuffer + "|" : format(value), r.x + 10, r.y + 8, 14, Ink);
    if (!active)
        text(unit, r.x + r.width - textWidth(unit, 11) - 9, r.y + 11, 11, Muted);
    return changed;
}
bool App::toggle(float x, float y, float w, const std::string &title, bool &v) {
    text(title, x, y + 5, 14, Ink);
    Rectangle r{x + w - 35, y + 3, 34, 19};
    panel(r, v ? Teal : Line, 1);
    DrawCircleV({r.x + (v ? 24.f : 10.f), r.y + 9.5f}, 6.5f, WHITE);
    if (!blocked && contains({x, y, w, 27}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        v = !v;
        return true;
    }
    return false;
}
void App::notify(const std::string &s) {
    toast = s;
    toastTime = 5;
}
void App::remember() { history.remember(sim.snapshot()); }
void App::edited() {
    dirty = true;
    if (!playing) {
        baseline = sim.snapshot();
        hasBaseline = false;
    }
}
void App::choosePreset(int index) {
    remember();
    presetIndex = std::clamp(index, 0, 4);
    sim.restore(preset(presetIndex));
    baseline = sim.snapshot();
    hasBaseline = false;
    playing = false;
    accumulator = 0;
    selected = 0;
    center = {10, 4};
    zoom = 1;
    energyHistory.clear();
    dirty = true;
    editId.clear();
    inspectorScroll = 0;
}
void App::playPause() {
    if (!playing && !hasBaseline) {
        baseline = sim.snapshot();
        hasBaseline = true;
    }
    playing = !playing;
    accumulator = 0;
    dirty = true;
}
void App::rewind() {
    remember();
    sim.restore(baseline);
    playing = false;
    accumulator = 0;
    hasBaseline = false;
    energyHistory.clear();
    dirty = true;
}
void App::undo(bool redo) {
    playing = false;
    accumulator = 0;
    if (redo ? history.redo(sim) : history.undo(sim)) {
        baseline = sim.snapshot();
        hasBaseline = false;
        selected = 0;
        energyHistory.clear();
        dirty = true;
    }
}
void App::duplicate() {
    if (auto *e = sim.find(selected)) {
        auto s = currentSpec(*e);
        s.id = 0;
        s.x += .6f;
        s.y -= .6f;
        s.name = "Copy of " + s.name.substr(0, 48);
        remember();
        selected = sim.add(s);
        edited();
    }
}
void App::remove() {
    if (sim.find(selected)) {
        remember();
        sim.erase(selected);
        selected = 0;
        edited();
    }
}
void App::openDialog(Dialog d) {
    dialog = d;
    editId.clear();
    playing = false;
    sceneFiles.clear();
    fileScroll = 0;
    if (d == Dialog::Load) {
        std::error_code ec;
        if (std::filesystem::exists("scenes", ec))
            for (const auto &f : std::filesystem::directory_iterator("scenes", ec))
                if (f.path().extension() == ".field")
                    sceneFiles.push_back(f.path().stem().string());
        std::sort(sceneFiles.begin(), sceneFiles.end());
    }
}
void App::load(const std::filesystem::path &path) {
    Scene s = loadScene(path);
    remember();
    sim.restore(s);
    baseline = s;
    hasBaseline = false;
    playing = false;
    selected = 0;
    accumulator = 0;
    dialog = Dialog::None;
    dirty = false;
    energyHistory.clear();
    filename = path.stem().string();
    presetIndex = -1;
    notify("Scene loaded. Press Space to explore.");
}
Vector2 App::screen(pe::Vector2 p) const {
    return {canvas.x + canvas.width / 2 + (p.x - center.x) * scale,
            canvas.y + canvas.height / 2 + (p.y - center.y) * scale};
}
pe::Vector2 App::world(Vector2 p) const {
    return {center.x + (p.x - canvas.x - canvas.width / 2) / scale,
            center.y + (p.y - canvas.y - canvas.height / 2) / scale};
}
void App::header() {
    float w = float(GetScreenWidth());
    DrawRectangle(0, 0, GetScreenWidth(), 76, Paper);
    DrawLine(0, 75, GetScreenWidth(), 75, Line);
    panel({22, 21, 34, 34}, Teal, .3f);
    DrawCircleLinesV({39, 38}, 10, Fade(WHITE, .9f));
    DrawCircleV({45, 31}, 3.5f, WHITE);
    text("field", 68, 17, 29, Ink);
    label("PHYSICS SANDBOX", 70, 48);
    DrawLine(219, 21, 219, 56, Line);
    text("Make room for curiosity.", 243, 30, 16, Muted);
    if (button({w - 491, 22, 36, 33}, "", false, "Undo  /  Ctrl+Z", 10, history.canUndo()))
        undo(false);
    if (button({w - 449, 22, 36, 33}, "", false, "Redo  /  Ctrl+Y", 11, history.canRedo()))
        undo(true);
    if (button({w - 391, 22, 87, 33}, "Open", false, "Open a saved scene  /  Ctrl+O"))
        openDialog(Dialog::Load);
    if (button({w - 295, 22, 103, 33}, "Save scene", false, "Save your experiment  /  Ctrl+S"))
        openDialog(Dialog::Save);
    if (button({w - 183, 22, 77, 33}, "Guide", false, "Controls and ideas  /  F1"))
        openDialog(Dialog::Help);
    panel({w - 93, 22, 70, 33}, Mint, .25f);
    DrawCircleV({w - 79, 38}, 3, Teal);
    text("LOCAL", w - 69, 32, 11, Teal);
}
void App::library() {
    float h = float(GetScreenHeight());
    DrawRectangle(0, 76, 220, GetScreenHeight() - 104, Paper);
    DrawLine(219, 76, 219, int(h - 28), Line);
    label("YOUR TOOLKIT", 22, 101);
    const char *titles[] = {"Select & move", "Circle", "Box", "Triangle", "Platform", "Push"};
    const char *tips[] = {"V  /  Drag bodies to reposition",   "C  /  Click or drag to draw a circle",
                          "B  /  Click or drag to draw a box", "T  /  Click or drag to draw a triangle",
                          "P  /  Draw a fixed platform",       "F  /  Drag from a body to apply an impulse"};
    for (int i = 0; i < 6; ++i) {
        Rectangle r{18, 125.f + i * 43, 184, 36};
        if (button(r, titles[i], int(tool) == i, tips[i], i)) {
            tool = Tool(i);
            editId.clear();
        }
    }
    label("START WITH AN IDEA", 22, 405);
    const char *subs[] = {"Ramps, shapes & gravity", "A satisfying chain reaction", "Compare four materials",
                          "Momentum, made visible", "Build something of your own"};
    for (int i = 0; i < 5; ++i) {
        Rectangle r{18, 428.f + i * 48, 184, 42};
        bool hover = contains(r) && !blocked;
        if (hover || presetIndex == i)
            panel(r, hover ? Mint : Back, .18f);
        text(presetName(i), 30, r.y + 5, 14, presetIndex == i ? Teal : Ink);
        text(subs[i], 30, r.y + 24, 10, Muted);
        if (hover) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            tooltip = "Open experiment (your current work stays in Undo)";
        }
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            choosePreset(i);
    }
    if (h > 810) {
        panel({18, h - 133, 184, 80}, Mint, .16f);
        text("Small changes.", 30, h - 119, 15, Teal);
        text("Interesting outcomes.", 30, h - 100, 15, Teal);
        text("Start paused. Press Space to play.", 30, h - 76, 10, Muted);
    }
}

void App::inspector() {
    float w = float(GetScreenWidth()), h = float(GetScreenHeight()), x = w - 278, cw = 238;
    DrawRectangle(int(w - 300), 76, 300, int(h - 104), Paper);
    DrawLine(int(w - 300), 76, int(w - 300), int(h - 28), Line);
    if (button({x, 94, 114, 32}, "Object", rightTab == 0)) {
        rightTab = 0;
        inspectorScroll = 0;
        editId.clear();
    }
    if (button({x + 124, 94, 114, 32}, "World", rightTab == 1)) {
        rightTab = 1;
        inspectorScroll = 0;
        editId.clear();
    }
    float contentHeight = (rightTab == 0 && !selected) ? 440.f : 730.f;
    float viewHeight = h - 176;
    if (contains({w - 300, 136, 300, viewHeight}) && !blocked)
        inspectorScroll =
            clamp(inspectorScroll - GetMouseWheelMove() * 35, 0, std::max(0.f, contentHeight - viewHeight));
    bool panelBlocked = blocked;
    blocked = blocked || !contains({w - 298, 138, 296, h - 170});
    BeginScissorMode(int(w - 298), 138, 296, int(h - 170));
    float y = 146 - inspectorScroll;
    if (rightTab == 0) {
        Entity *e = sim.find(selected);
        if (!e) {
            icon(0, x + 92, y + 45, 48, Teal);
            text("Meet your objects.", x + 38, y + 112, 20, Ink);
            text("Select a shape on the canvas to", x + 14, y + 150, 14, Muted);
            text("inspect it, change its material,", x + 14, y + 171, 14, Muted);
            text("or give it a little momentum.", x + 14, y + 192, 14, Muted);
            DrawLine(int(x), int(y + 244), int(x + cw), int(y + 244), Line);
            label("TRY THIS", x, y + 265);
            text("1   Choose Circle on the left", x, y + 294, 14, Ink);
            text("2   Drag on the canvas to draw", x, y + 321, 14, Ink);
            text("3   Press Space and see it fall", x, y + 348, 14, Ink);
            text("Wheel to zoom. Right-drag to pan.", x, y + 396, 12, Muted);
        } else {
            BodySpec s = currentSpec(*e);
            bool changed = false;
            panel({x, y, 36, 36}, Fade(Palette[s.color], .45f), .25f);
            icon(s.fixed ? 4 : int(s.shape) + 1, x + 7, y + 7, 22, Edges[s.color]);
            text(s.name.substr(0, 24), x + 48, y + 1, 20, Ink);
            text(std::string(s.fixed ? "Fixed body" : "Dynamic body") + "  /  #" + std::to_string(s.id),
                 x + 48, y + 27, 11, Muted);
            y += 60;
            changed |= number("x", "POSITION X", {x, y, 114, 50}, s.x, -1000, 1000, "m");
            changed |= number("y", "POSITION Y", {x + 124, y, 114, 50}, s.y, -1000, 1000, "m");
            y += 65;
            changed |= number("width", s.shape == Shape::Circle ? "DIAMETER" : "WIDTH", {x, y, 114, 50},
                              s.width, .15f, 30, "m");
            if (s.shape != Shape::Circle)
                changed |= number("height", "HEIGHT", {x + 124, y, 114, 50}, s.height, .15f, 30, "m");
            else {
                label("MASS", x + 124, y);
                text(format(e->body->mass) + " kg", x + 134, y + 26, 16, Ink);
            }
            y += 65;
            float angle = s.angle * RAD2DEG;
            if (number("angle", "ROTATION", {x, y, cw, 50}, angle, -3600, 3600, "deg")) {
                s.angle = angle * DEG2RAD;
                changed = true;
            }
            y += 67;
            label("MATERIAL", x, y);
            y += 21;
            const char *mats[] = {"Rubber", "Wood", "Steel"};
            for (int i = 0; i < 3; ++i)
                if (button({x + i * 81, y, 76, 29}, mats[i])) {
                    s.density = i == 0 ? 1.1f : i == 1 ? .7f : 7.8f;
                    s.bounce = i == 0 ? .85f : i == 1 ? .3f : .1f;
                    s.friction = i == 0 ? .7f : i == 1 ? .5f : .3f;
                    changed = true;
                }
            y += 41;
            changed |= number("bounce", "BOUNCINESS", {x, y, 114, 50}, s.bounce, 0, 1, "");
            changed |= number("friction", "FRICTION", {x + 124, y, 114, 50}, s.friction, 0, 1, "");
            y += 65;
            changed |= number("density", "DENSITY (2D)", {x, y, cw, 50}, s.density, .05f, 20, "kg/m2");
            y += 63;
            changed |= toggle(x, y, cw, "Fixed in place", s.fixed);
            y += 45;
            label("COLOR", x, y + 7);
            for (int i = 0; i < 5; ++i) {
                Vector2 p{x + 95 + i * 30, y + 12};
                DrawCircleV(p, 10, Palette[i]);
                if (s.color == i)
                    DrawCircleLinesV(p, 13, Edges[i]);
                if (!blocked && CheckCollisionPointCircle(GetMousePosition(), p, 13) &&
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    s.color = i;
                    changed = true;
                }
            }
            y += 43;
            DrawLine(int(x), int(y), int(x + cw), int(y), Line);
            y += 18;
            label("INITIAL / LIVE VELOCITY", x, y);
            y += 21;
            changed |= number("vx", "HORIZONTAL", {x, y, 114, 50}, s.vx, -25, 25, "m/s");
            changed |= number("vy", "VERTICAL", {x + 124, y, 114, 50}, s.vy, -25, 25, "m/s");
            y += 62;
            text("Speed", x, y, 13, Muted);
            text(format(e->body->velocity.magnitude()) + " m/s", x + 145, y, 14, Ink);
            y += 23;
            text("Mass", x, y, 13, Muted);
            text(s.fixed ? "Fixed" : format(e->body->mass) + " kg", x + 145, y, 14, Ink);
            y += 36;
            if (button({x, y, 114, 34}, "Duplicate", false, "Duplicate selected body  /  Ctrl+D", 13))
                duplicate();
            if (button({x + 124, y, 114, 34}, "Delete", false, "Delete selected body  /  Delete", 12))
                remove();
            if (changed && sim.find(s.id)) {
                remember();
                sim.replace(s.id, s);
                edited();
            }
        }
    } else {
        text("A world of possibilities.", x, y, 20, Ink);
        y += 32;
        text("Tune the rules. Watch what changes.", x, y, 12, Muted);
        y += 42;
        float g = sim.gravity;
        if (number("gravity", "GRAVITY", {x, y, cw, 50}, g, -25, 25, "m/s2")) {
            remember();
            sim.setGravity(g);
            edited();
        }
        y += 64;
        const char *worlds[] = {"Earth", "Moon", "Zero"};
        float gs[] = {9.81f, 1.62f, 0};
        for (int i = 0; i < 3; ++i)
            if (button({x + i * 81, y, 76, 30}, worlds[i], std::abs(g - gs[i]) < .01f)) {
                remember();
                sim.setGravity(gs[i]);
                edited();
            }
        y += 61;
        label("VIEW OPTIONS", x, y);
        y += 24;
        toggle(x, y, cw, "Meter grid", showGrid);
        y += 36;
        toggle(x, y, cw, "Velocity vectors", showVectors);
        y += 36;
        toggle(x, y, cw, "Motion trails", showTrails);
        y += 55;
        label("LIVE MEASUREMENTS", x, y);
        y += 28;
        text("Kinetic energy", x, y, 14, Muted);
        text(format(float(sim.kineticEnergy()), 1) + " J", x + 157, y, 15, Ink);
        y += 26;
        panel({x, y, cw, 96}, Back, .12f);
        float peak = 1;
        for (float n : energyHistory)
            peak = std::max(peak, n);
        for (std::size_t n = 1; n < energyHistory.size(); ++n) {
            float dx = (cw - 18) / 119;
            DrawLineEx({x + 9 + (n - 1) * dx, y + 84 - energyHistory[n - 1] / peak * 66},
                       {x + 9 + n * dx, y + 84 - energyHistory[n] / peak * 66}, 2, Teal);
        }
        text("last 12 seconds", x + 10, y + 8, 10, Muted);
        y += 114;
        auto stats = sim.statistics();
        text("Bodies", x, y, 13, Muted);
        text(std::to_string(sim.entities().size()) + " / 250", x + 157, y, 14, Ink);
        y += 27;
        text("Active contacts", x, y, 13, Muted);
        text(std::to_string(stats.activeContactCount), x + 157, y, 14, Ink);
        y += 27;
        text("Physics timestep", x, y, 13, Muted);
        text("120 Hz", x + 157, y, 14, Ink);
        y += 43;
        text("Positive Y points down. One grid", x, y, 12, Muted);
        text("square is one meter. Energy includes", x, y + 18, 12, Muted);
        text("translation and rotation.", x, y + 36, 12, Muted);
        if (button({x, y + 72, cw, 36}, "Export measurements", false,
                   "Save this moment as a CSV spreadsheet")) {
            std::filesystem::create_directories("exports");
            auto stamp = std::chrono::system_clock::now().time_since_epoch();
            auto path =
                std::filesystem::path("exports") /
                ("measurements-" +
                 std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stamp).count()) +
                 ".csv");
            exportMeasurements(sim, path);
            notify("Measurements exported to the exports folder.");
        }
    }
    EndScissorMode();
    blocked = panelBlocked;
    if (contentHeight > viewHeight) {
        float track = viewHeight - 8, thumb = std::max(30.f, track * viewHeight / contentHeight);
        panel({w - 7, 142 + inspectorScroll / (contentHeight - viewHeight) * (track - thumb), 3, thumb}, Line,
              1);
        if (inspectorScroll < contentHeight - viewHeight - 5) {
            DrawRectangle(int(w - 299), int(h - 55), 298, 27, Fade(Paper, .96f));
            text("Scroll for more properties", x + 47, h - 49, 11, Muted);
        }
    }
}
void App::bodyDrawing(const BodySpec &b, bool highlight, float opacity) {
    Vector2 p = screen({b.x, b.y});
    float w = b.width * scale, h = b.height * scale;
    Color fill = Fade(Palette[b.color], opacity), edge = Fade(Edges[b.color], opacity);
    if (b.shape == Shape::Circle) {
        DrawCircleV({p.x + 2, p.y + 5}, w / 2, Fade(Ink, .06f * opacity));
        DrawCircleV(p, w / 2, fill);
        DrawCircleLinesV(p, w / 2, edge);
        DrawCircleV({p.x - w * .16f, p.y - w * .2f}, w * .08f, Fade(WHITE, .48f * opacity));
        Vector2 end{p.x + std::cos(b.angle) * w * .36f, p.y + std::sin(b.angle) * w * .36f};
        DrawLineEx(p, end, 1.4f, Fade(edge, .5f));
        if (highlight)
            DrawCircleLinesV(p, w / 2 + 5, Teal);
    } else {
        std::vector<Vector2> v;
        std::vector<Vector2> local =
            b.shape == Shape::Box
                ? std::vector<Vector2>{{-w / 2, -h / 2}, {w / 2, -h / 2}, {w / 2, h / 2}, {-w / 2, h / 2}}
                : std::vector<Vector2>{{-w / 2, h / 3}, {0, -h * 2 / 3}, {w / 2, h / 3}};
        for (auto q : local)
            v.push_back({p.x + q.x * std::cos(b.angle) - q.y * std::sin(b.angle),
                         p.y + q.x * std::sin(b.angle) + q.y * std::cos(b.angle)});
        for (std::size_t i = 1; i + 1 < v.size(); ++i)
            DrawTriangle(v[0], v[i + 1], v[i], fill);
        for (std::size_t i = 0; i < v.size(); ++i)
            DrawLineEx(v[i], v[(i + 1) % v.size()], highlight ? 2.5f : 1.3f, highlight ? Teal : edge);
        if (b.fixed && w > 55)
            for (int i = -2; i <= 2; ++i) {
                float dx = i * 9.f;
                Vector2 a{p.x + dx * std::cos(b.angle) - 3 * std::sin(b.angle),
                          p.y + dx * std::sin(b.angle) + 3 * std::cos(b.angle)};
                DrawCircleV(a, 1.1f, Fade(edge, .6f));
            }
    }
    if (highlight) {
        DrawCircleV(p, 2.5f, Teal);
    }
}
void App::stage() {
    BeginScissorMode(int(canvas.x), int(canvas.y), int(canvas.width), int(canvas.height));
    DrawRectangleRec(canvas, Back);
    if (showGrid) {
        auto a = world({canvas.x, canvas.y}), b = world({canvas.x + canvas.width, canvas.y + canvas.height});
        for (int x = int(std::floor(a.x)); x <= int(std::ceil(b.x)); ++x)
            for (int y = int(std::floor(a.y)); y <= int(std::ceil(b.y)); ++y) {
                Vector2 p = screen({float(x), float(y)});
                DrawCircleV(p, 1, Color{211, 222, 217, 255});
            }
    }
    if (showTrails)
        for (const auto &e : sim.entities())
            for (std::size_t n = 1; n < e.trail.size(); ++n)
                DrawLineEx(screen(e.trail[n - 1]), screen(e.trail[n]), 1.5f,
                           Fade(Edges[e.spec.color], .45f * float(n) / e.trail.size()));
    for (const auto &e : sim.entities())
        bodyDrawing(currentSpec(e), e.spec.id == selected);
    if (showVectors)
        for (const auto &e : sim.entities())
            if (!e.spec.fixed) {
                auto v = e.body->velocity;
                if (v.magnitude() < .05f)
                    continue;
                Vector2 a = screen(e.body->position), b = screen(e.body->position + v * .22f);
                DrawLineEx(a, b, 2, Edges[e.spec.color]);
                auto dir = v.normalized();
                DrawLineEx(b, {b.x - dir.x * 8 + dir.y * 4, b.y - dir.y * 8 - dir.x * 4}, 2,
                           Edges[e.spec.color]);
                DrawLineEx(b, {b.x - dir.x * 8 - dir.y * 4, b.y - dir.y * 8 + dir.x * 4}, 2,
                           Edges[e.spec.color]);
            }
    if (drawing && int(tool) >= 1 && int(tool) <= 4) {
        auto p = world(GetMousePosition());
        BodySpec s;
        s.shape = tool == Tool::Circle     ? Shape::Circle
                  : tool == Tool::Triangle ? Shape::Triangle
                                           : Shape::Box;
        s.x = (p.x + gestureStart.x) / 2;
        s.y = (p.y + gestureStart.y) / 2;
        s.width = clamp(std::abs(p.x - gestureStart.x), .15f, 20);
        s.height = clamp(std::abs(p.y - gestureStart.y), .15f, 20);
        if (s.shape == Shape::Circle)
            s.width = std::max(s.width, s.height);
        s.fixed = tool == Tool::Platform;
        s.color = s.fixed ? 4 : 0;
        bodyDrawing(s, true, .5f);
        auto pos = screen({s.x, s.y});
        text(format(s.width) + " m", pos.x + 12, pos.y - 28, 12, Teal);
    }
    if (drawing && tool == Tool::Push) {
        auto a = screen(gestureStart);
        auto b = GetMousePosition();
        DrawLineEx(a, b, 3, Teal);
        DrawCircleV(b, 5, Teal);
        text("Release to push", b.x + 12, b.y - 20, 12, Teal);
    }
    // Canvas headings have an opaque backing so panning never obscures labels.
    DrawRectangle(int(canvas.x + 16), int(canvas.y + 12), std::min(390, int(canvas.width) - 32), 80,
                  Fade(Back, .96f));
    label("THE PLAYGROUND", canvas.x + 28, canvas.y + 25);
    text(sim.name.substr(0, 38), canvas.x + 28, canvas.y + 45, 30, Ink);
    if (canvas.width > 620)
        text(presetIndex < 0 ? "Your saved experiment, ready to explore." : presetDescription(presetIndex),
             canvas.x + 28, canvas.y + 82, 12, Muted);
    auto ground = screen({0, 10.5f});
    if (ground.y < canvas.y + canvas.height - 110 && ground.y > canvas.y + 130)
        text("GROUND", canvas.x + 29, ground.y + 30, 10, Muted);
    float sy = canvas.y + canvas.height - 33;
    DrawLineEx({canvas.x + 29, sy}, {canvas.x + 29 + scale, sy}, 1.5f, Muted);
    DrawLineEx({canvas.x + 29, sy - 4}, {canvas.x + 29, sy + 4}, 1.5f, Muted);
    DrawLineEx({canvas.x + 29 + scale, sy - 4}, {canvas.x + 29 + scale, sy + 4}, 1.5f, Muted);
    text("1 m", canvas.x + 31, sy - 21, 11, Muted);
    EndScissorMode();
}
void App::playback() {
    float x = transport.x, y = transport.y;
    panel({x, y + 4, transport.width, 54}, Fade(Ink, .06f), .25f);
    panel(transport, Paper, .25f);
    DrawRectangleRoundedLinesEx(transport, .25f, 8, 1, Line);
    if (button({x + 8, y + 8, 94, 38}, playing ? "Pause" : "Play", true, "Play / pause  /  Space",
               playing ? 7 : 6))
        playPause();
    if (button({x + 110, y + 8, 36, 38}, "", false, "Advance one physics step  /  .", 9, !playing)) {
        if (!hasBaseline) {
            baseline = sim.snapshot();
            hasBaseline = true;
        }
        sim.step();
        dirty = true;
    }
    if (button({x + 152, y + 8, 36, 38}, "", false, "Rewind to the start of this run  /  R", 8))
        rewind();
    DrawLine(int(x + 201), int(y + 13), int(x + 201), int(y + 41), Line);
    if (button({x + 212, y + 8, 57, 38}, format(speed, speed < 1 ? 2 : 0) + "x", false,
               "Cycle speed: 0.25x, 0.5x, 1x, 2x"))
        speed = speed == .25f ? .5f : speed == .5f ? 1 : speed == 1 ? 2 : .25f;
    text(format(float(sim.time)) + " s", x + 287, y + 19, 15, Ink);
    float zx = canvas.x + canvas.width - 142;
    if (button({zx, canvas.y + canvas.height - 48, 31, 29}, "-", false, "Zoom out"))
        zoom = clamp(zoom / 1.2f, .45f, 3);
    if (button({zx + 36, canvas.y + canvas.height - 48, 57, 29}, std::to_string(int(zoom * 100)) + "%", false,
               "Reset camera  /  Home")) {
        zoom = 1;
        center = {10, 4};
    }
    if (button({zx + 98, canvas.y + canvas.height - 48, 31, 29}, "+", false, "Zoom in"))
        zoom = clamp(zoom * 1.2f, .45f, 3);
    const char *hints[] = {"Drag to move  /  Wheel to zoom  /  Right-drag to pan",
                           "Click or drag to draw a circle",
                           "Click or drag to draw a box",
                           "Click or drag to draw a triangle",
                           "Drag to draw a fixed platform",
                           "Drag from a body in the direction of the push"};
    float tw = textWidth(hints[int(tool)], 11);
    panel({canvas.x + (canvas.width - tw) / 2 - 9, transport.y - 29, tw + 18, 20}, Fade(Paper, .94f), .3f);
    text(hints[int(tool)], canvas.x + (canvas.width - tw) / 2, transport.y - 24, 11, Muted);
}
void App::modal() {
    if (dialog == Dialog::None)
        return;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(Ink, .32f));
    bool help = dialog == Dialog::Help;
    float w = help ? 590.f : 520.f, h = help ? 540.f : 430.f;
    float x = (GetScreenWidth() - w) / 2, y = (GetScreenHeight() - h) / 2;
    panel({x, y + 7, w, h}, Fade(Ink, .13f), .045f);
    panel({x, y, w, h}, Paper, .045f);
    bool oldBlocked = blocked;
    blocked = false;
    const char *title = help                     ? "A little guide to Field"
                        : dialog == Dialog::Save ? "Keep your experiment"
                        : dialog == Dialog::Load ? "Back to your experiments"
                                                 : "Before you go";
    text(title, x + 30, y + 28, 26, Ink);
    if (button({x + w - 61, y + 24, 31, 31}, "x", false, "Close dialog  /  Escape"))
        dialog = Dialog::None;
    if (help) {
        text("Build a world, change one thing, see what happens.", x + 30, y + 69, 14, Muted);
        const char *keys[] = {"V / C / B / T / P / F", "Space / . / R",   "Ctrl+Z / Ctrl+Y",
                              "Ctrl+D / Delete",       "Ctrl+S / Ctrl+O", "Wheel / Right-drag",
                              "Home / Escape"};
        const char *descriptions[] = {"Move, circle, box, triangle, platform, push",
                                      "Play or pause / single step / rewind",
                                      "Undo / redo edits (64 steps)",
                                      "Duplicate / remove selected body",
                                      "Save / open scenes",
                                      "Zoom at cursor / pan the canvas",
                                      "Reset view / cancel gesture or edit"};
        for (int i = 0; i < 7; ++i) {
            float yy = y + 115 + i * 39;
            text(keys[i], x + 30, yy, 13, Teal);
            text(descriptions[i], x + 206, yy, 13, Ink);
        }
        DrawLine(int(x + 30), int(y + 396), int(x + w - 30), int(y + 396), Line);
        text("Try Bounce lab: compare how much energy each ball keeps.", x + 30, y + 416, 13, Ink);
        text("Click a numeric field, Ctrl+A, type, then Enter to apply.", x + 30, y + 441, 13, Muted);
        text("Drop a .field scene onto the window to open it.", x + 30, y + 463, 13, Muted);
        if (button({x + w - 129, y + h - 49, 99, 32}, "Got it", true))
            dialog = Dialog::None;
    } else if (dialog == Dialog::Exit) {
        text("Your current experiment has unsaved changes.", x + 30, y + 86, 16, Ink);
        text("Save it, or keep exploring a little longer.", x + 30, y + 116, 14, Muted);
        if (button({x + 30, y + 183, w - 60, 42}, "Save before closing", true)) {
            dialog = Dialog::Save;
            notify("Save your scene, then close the window.");
        }
        if (button({x + 30, y + 240, w - 60, 42}, "Close without saving"))
            wantsExit = true;
        if (button({x + 30, y + 297, w - 60, 42}, "Keep exploring"))
            dialog = Dialog::None;
    } else {
        text(dialog == Dialog::Save ? "Scenes are saved beside the app in the scenes folder."
                                    : "Choose a saved scene, or type its name below.",
             x + 30, y + 72, 13, Muted);
        label("SCENE NAME", x + 30, y + 108);
        Rectangle input{x + 30, y + 129, w - 60, 43};
        panel(input, Back, .15f);
        DrawRectangleRoundedLinesEx(input, .15f, 8, 1, Teal);
        int c;
        while ((c = GetCharPressed()) > 0)
            if (filename.size() < 48 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                         (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_'))
                filename += char(c);
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A))
            filename.clear();
        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
            if (!filename.empty())
                filename.pop_back();
        text(filename + (int(GetTime() * 2) % 2 ? "|" : ""), x + 43, y + 141, 16, Ink);
        text(".field", x + w - 79, y + 144, 12, Muted);
        if (dialog == Dialog::Load) {
            if (contains({x + 30, y + 184, w - 60, 159}))
                fileScroll = std::clamp(fileScroll - int(GetMouseWheelMove()), 0,
                                        std::max(0, int(sceneFiles.size()) - 4));
            if (sceneFiles.empty()) {
                text("No saved scenes yet.", x + 30, y + 218, 16, Ink);
                text("Create something, then use Save scene.", x + 30, y + 245, 13, Muted);
            }
            for (int i = fileScroll; i < std::min(int(sceneFiles.size()), fileScroll + 4); ++i)
                if (button({x + 30, y + 184 + (i - fileScroll) * 38, w - 60, 32}, sceneFiles[i].substr(0, 42),
                           filename == sceneFiles[i]))
                    filename = sceneFiles[i];
        } else {
            panel({x + 30, y + 196, w - 60, 106}, Mint, .12f);
            text("Everything you need to pick up where you left off.", x + 45, y + 214, 14, Teal);
            text("Shapes, materials, positions, velocities and gravity.", x + 45, y + 239, 12, Muted);
            text("Replacing a scene keeps its previous save as a .bak file.", x + 45, y + 266, 12, Muted);
        }
        bool enter = IsKeyPressed(KEY_ENTER);
        if (button({x + w - 252, y + h - 63, 100, 36}, "Cancel"))
            dialog = Dialog::None;
        if (button({x + w - 140, y + h - 63, 110, 36}, dialog == Dialog::Load ? "Open scene" : "Save scene",
                   true) ||
            enter) {
            if (filename.empty() || filename.find_first_not_of(' ') == std::string::npos)
                notify("Give your experiment a name first.");
            else {
                auto path = std::filesystem::path("scenes") / (filename + ".field");
                if (dialog == Dialog::Load)
                    load(path);
                else {
                    std::filesystem::create_directories("scenes");
                    auto scene = sim.snapshot();
                    scene.name = filename;
                    saveScene(scene, path);
                    sim.name = filename;
                    dirty = false;
                    dialog = Dialog::None;
                    notify("Saved to scenes / " + filename + ".field");
                }
            }
        }
    }
    blocked = oldBlocked;
}
void App::keyboard() {
    if (dialog != Dialog::None) {
        if (IsKeyPressed(KEY_ESCAPE))
            dialog = Dialog::None;
        return;
    }
    if (!editId.empty())
        return;
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl) {
        if (IsKeyPressed(KEY_Z))
            undo(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
        if (IsKeyPressed(KEY_Y))
            undo(true);
        if (IsKeyPressed(KEY_D))
            duplicate();
        if (IsKeyPressed(KEY_S))
            openDialog(Dialog::Save);
        if (IsKeyPressed(KEY_O))
            openDialog(Dialog::Load);
        return;
    }
    if (IsKeyPressed(KEY_SPACE))
        playPause();
    if (IsKeyPressed(KEY_R))
        rewind();
    if (IsKeyPressed(KEY_PERIOD) && !playing) {
        if (!hasBaseline) {
            baseline = sim.snapshot();
            hasBaseline = true;
        }
        sim.step();
        dirty = true;
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))
        remove();
    if (IsKeyPressed(KEY_V))
        tool = Tool::Select;
    if (IsKeyPressed(KEY_C))
        tool = Tool::Circle;
    if (IsKeyPressed(KEY_B))
        tool = Tool::Box;
    if (IsKeyPressed(KEY_T))
        tool = Tool::Triangle;
    if (IsKeyPressed(KEY_P))
        tool = Tool::Platform;
    if (IsKeyPressed(KEY_F))
        tool = Tool::Push;
    if (IsKeyPressed(KEY_F1))
        openDialog(Dialog::Help);
    if (IsKeyPressed(KEY_HOME)) {
        zoom = 1;
        center = {10, 4};
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (dragging && dragChanged && dragBefore)
            sim.replace(selected, *dragBefore);
        tool = Tool::Select;
        selected = 0;
        drawing = false;
        dragging = false;
        panning = false;
        dragBefore.reset();
    }
    if (auto *e = sim.find(selected)) {
        float dx = 0, dy = 0;
        if (IsKeyPressed(KEY_LEFT))
            dx = -.1f;
        if (IsKeyPressed(KEY_RIGHT))
            dx = .1f;
        if (IsKeyPressed(KEY_UP))
            dy = -.1f;
        if (IsKeyPressed(KEY_DOWN))
            dy = .1f;
        if (dx || dy) {
            remember();
            e->body->position = e->body->position + pe::Vector2{dx, dy};
            edited();
        }
    }
}
void App::canvasInput() {
    if (blocked)
        return;
    Vector2 mouse = GetMousePosition();
    bool over = contains(canvas) && mouse.y < transport.y - 30;
    if (over) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            auto before = world(mouse);
            float old = zoom;
            zoom = clamp(zoom * std::pow(1.15f, wheel), .45f, 3);
            scale *= zoom / old;
            auto after = world(mouse);
            center = center + (before - after);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
            panning = true;
            panMouse = mouse;
        }
    }
    if (panning) {
        SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        center = center - pe::Vector2{(mouse.x - panMouse.x) / scale, (mouse.y - panMouse.y) / scale};
        panMouse = mouse;
        if (!IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
            panning = false;
    }
    if (over && !panning) {
        if (tool != Tool::Select)
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            editId.clear();
            gestureStart = world(mouse);
            if (tool == Tool::Select || tool == Tool::Push) {
                selected = hitTest(sim, gestureStart);
                rightTab = 0;
                inspectorScroll = 0;
                if (auto *e = sim.find(selected)) {
                    dragOffset = e->body->position - gestureStart;
                    if (tool == Tool::Select) {
                        dragging = true;
                        dragChanged = false;
                        dragBefore = currentSpec(*e);
                    } else {
                        remember();
                        drawing = true;
                    }
                }
            } else
                drawing = true;
        }
    }
    if (dragging) {
        if (!dragChanged && (world(mouse) - gestureStart).magnitude() > .05f) {
            remember();
            dragChanged = true;
            playing = false;
            accumulator = 0;
        }
        if (auto *e = sim.find(selected); e && dragChanged) {
            auto p = world(mouse) + dragOffset;
            e->body->SetPosition({clamp(p.x, -1000, 1000), clamp(p.y, -1000, 1000)});
            e->body->SetVelocity({0, 0});
            e->body->SetAngularVelocity(0);
            e->trail.clear();
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            dragging = false;
            if (dragChanged)
                edited();
            dragBefore.reset();
        }
    }
    if (drawing && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        drawing = false;
        auto end = world(mouse);
        if (tool == Tool::Push) {
            if (auto *e = sim.find(selected)) {
                auto delta = end - gestureStart;
                float m = delta.magnitude();
                if (m > 12)
                    delta = delta * (12 / m);
                e->body->ApplyImpulse(delta * e->body->mass * 2, gestureStart - e->body->position);
                auto v = e->body->velocity;
                if (v.magnitude() > 30)
                    e->body->velocity = v.normalized() * 30;
                e->body->angularVelocity = clamp(e->body->angularVelocity, -30, 30);
                edited();
            }
        } else if (over) {
            BodySpec s;
            s.shape = tool == Tool::Circle     ? Shape::Circle
                      : tool == Tool::Triangle ? Shape::Triangle
                                               : Shape::Box;
            s.fixed = tool == Tool::Platform;
            s.color = s.fixed ? 4 : (int(sim.entities().size()) % 4);
            s.name = s.fixed                  ? "Platform"
                     : tool == Tool::Circle   ? "Circle"
                     : tool == Tool::Triangle ? "Triangle"
                                              : "Box";
            s.x = (gestureStart.x + end.x) / 2;
            s.y = (gestureStart.y + end.y) / 2;
            s.width = clamp(std::abs(end.x - gestureStart.x), .15f, 20);
            s.height = clamp(std::abs(end.y - gestureStart.y), .15f, 20);
            if ((end - gestureStart).magnitude() < .2f) {
                s.x = end.x;
                s.y = end.y;
                s.width = s.fixed ? 3 : 1.2f;
                s.height = s.fixed ? .3f : 1.2f;
            }
            if (s.shape == Shape::Circle)
                s.width = std::max(s.width, s.height);
            remember();
            selected = sim.add(s);
            edited();
            rightTab = 0;
            inspectorScroll = 0;
        }
    }
}
void App::frame() {
    if (WindowShouldClose()) {
        if (dirty)
            openDialog(Dialog::Exit);
        else
            wantsExit = true;
    }
    float w = float(GetScreenWidth()), h = float(GetScreenHeight());
    canvas = {220, 76, w - 520, h - 104};
    transport = {canvas.x + (canvas.width - 384) / 2, canvas.y + canvas.height - 104, 384, 54};
    scale = std::min(canvas.width / 22, canvas.height / 16) * zoom;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    tooltip.clear();
    float dt = std::min(GetFrameTime(), .1f);
    toastTime = std::max(0.f, toastTime - dt);
    try {
        keyboard();
        blocked = dialog != Dialog::None;
        canvasInput();
        if (IsFileDropped()) {
            auto files = LoadDroppedFiles();
            if (files.count > 0) {
                std::string path = files.paths[0];
                UnloadDroppedFiles(files);
                load(path);
            } else
                UnloadDroppedFiles(files);
        }
        if (playing && !(dragging && dragChanged)) {
            accumulator += dt * speed;
            int steps = 0;
            while (accumulator >= Step && steps < 24) {
                sim.step();
                accumulator -= Step;
                ++steps;
            }
            if (steps == 24)
                accumulator = 0;
            sampleTime += steps * Step;
            if (sampleTime >= .1f) {
                sampleTime = 0;
                energyHistory.push_back(float(sim.kineticEnergy()));
                if (energyHistory.size() > 120)
                    energyHistory.pop_front();
            }
        }
    } catch (const std::exception &error) {
        playing = false;
        drawing = false;
        dragging = false;
        notify(error.what());
    }
    BeginDrawing();
    ClearBackground(Back);
    try {
        stage();
        header();
        library();
        bool b = blocked;
        // Scissoring alone clips pixels, not hit testing; block off-panel inputs too.
        blocked = b || !contains({w - 300, 76, 300, h - 104});
        inspector();
        blocked = b;
        playback();
        modal();
    } catch (const std::exception &error) {
        EndScissorMode();
        playing = false;
        notify(error.what());
    }
    DrawRectangle(0, int(h - 28), int(w), 28, Paper);
    DrawLine(0, int(h - 28), int(w), int(h - 28), Line);
    DrawCircleV({17, h - 14}, 3, playing ? Teal : Muted);
    text(playing ? "SIMULATING" : "PAUSED", 27, h - 20, 10, Muted);
    text(std::to_string(sim.entities().size()) + " bodies", 136, h - 20, 10, Muted);
    text("120 Hz physics", 220, h - 20, 10, Muted);
    text(dirty ? "Unsaved changes" : "Ready to explore", w - 180, h - 20, 10, Muted);
    if (toastTime > 0) {
        float tw = std::min(w - 40, textWidth(toast, 13) + 34);
        panel({(w - tw) / 2, h - 80, tw, 34}, Ink, .2f);
        text(toast, (w - tw) / 2 + 17, h - 71, 13, WHITE);
    } else if (!tooltip.empty() && dialog == Dialog::None) {
        float tw = textWidth(tooltip, 12) + 22;
        auto m = GetMousePosition();
        float xx = clamp(m.x - tw / 2, 8, w - tw - 8), yy = std::min(h - 65, m.y + 24);
        panel({xx, yy, tw, 28}, Ink, .2f);
        text(tooltip, xx + 11, yy + 7, 12, WHITE);
    }
    EndDrawing();
}
} // namespace field

namespace field {
// Deterministic, in-process UI checks. Raylib events update this application's
// input state only; they do not send input to Windows or move the system cursor.
void App::smokeTest() {
    SetTargetFPS(0);
    int assertions = 0, mouseX = 0, mouseY = 0;
    auto check = [&](bool ok, const char *why) {
        ++assertions;
        if (!ok)
            throw std::runtime_error(why);
    };
    auto event = [](unsigned type, int a = 0, int b = 0) {
        AutomationEvent e{};
        e.type = type;
        e.params[0] = a;
        e.params[1] = b;
        PlayAutomationEvent(e);
    };
    auto tick = [&] {
        event(7, mouseX, mouseY);
        frame();
    };
    auto click = [&](int x, int y) {
        mouseX = x;
        mouseY = y;
        event(6, 0);
        tick();
        event(5, 0);
        tick();
    };
    auto key = [&](int k, bool ctrl = false) {
        if (ctrl)
            event(2, KEY_LEFT_CONTROL);
        event(2, k);
        tick();
        event(1, k);
        if (ctrl)
            event(1, KEY_LEFT_CONTROL);
        tick();
    };
    tick();
    auto count = sim.entities().size();
    click(100, 185);
    check(tool == Tool::Circle, "Circle toolbar button");
    click(450, 290);
    check(sim.entities().size() == count + 1, "Canvas click must create a body");
    int created = selected;
    key(KEY_V);
    auto beforeDrag = currentSpec(*sim.find(created));
    mouseX = 450;
    mouseY = 290;
    event(6, 0);
    tick();
    mouseX = 500;
    mouseY = 310;
    tick();
    event(5, 0);
    tick();
    check(sim.find(created)->body->position.x > beforeDrag.x + .5f, "Dragging moves the selected body");
    key(KEY_Z, true);
    check(std::abs(sim.find(created)->body->position.x - beforeDrag.x) < .001f,
          "Undo restores a dragged body");
    selected = created;
    key(KEY_D, true);
    check(sim.entities().size() == count + 2, "Duplicate keyboard shortcut");
    key(KEY_Z, true);
    check(sim.entities().size() == count + 1, "Undo keyboard shortcut");
    key(KEY_Y, true);
    check(sim.entities().size() == count + 2, "Redo keyboard shortcut");
    selected = created;
    rightTab = 0;
    click(GetScreenWidth() - 250, 240);
    check(editId == "x", "Inspector number focus");
    editBuffer = "7.5";
    key(KEY_ENTER);
    check(std::abs(sim.find(created)->body->position.x - 7.5f) < .001f,
          "Numeric property edit must reach engine");
    click(GetScreenWidth() - 98, 110);
    check(rightTab == 1, "World tab");
    click(GetScreenWidth() - 158, 298);
    check(std::abs(sim.gravity - 1.62f) < .01f, "Moon gravity button");
    key(KEY_SPACE);
    check(playing, "Space starts playback");
    accumulator = Step * 2;
    tick();
    check(sim.time > 0, "Playback advances the engine");
    key(KEY_SPACE);
    check(!playing, "Space pauses playback");
    key(KEY_R);
    check(sim.time == 0, "Rewind resets time");
    key(KEY_S, true);
    check(dialog == Dialog::Save, "Save shortcut opens dialog");
    filename = "__field_smoke_scene";
    key(KEY_ENTER);
    check(dialog == Dialog::None && !dirty, "Save dialog completes");
    auto saved = sim.snapshot();
    choosePreset(4);
    key(KEY_O, true);
    check(dialog == Dialog::Load, "Open shortcut opens dialog");
    key(KEY_ENTER);
    check(dialog == Dialog::None && sim.entities().size() == saved.bodies.size(),
          "Load dialog restores saved bodies");
    check(std::abs(sim.gravity - saved.gravity) < .001f, "Load preserves gravity");
    key(KEY_F1);
    check(dialog == Dialog::Help, "Help shortcut");
    key(KEY_ESCAPE);
    check(dialog == Dialog::None, "Escape dismisses dialog");
    auto smoke = std::filesystem::path("scenes") / "__field_smoke_scene.field";
    std::filesystem::remove(smoke);
    smoke += ".bak";
    std::filesystem::remove(smoke);
    std::cout << "PASS: " << assertions << " in-process UI checks (no OS input).\n";
}
} // namespace field
