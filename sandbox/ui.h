#pragma once
#include "raylib.h"
#include "scene.h"
#include <deque>
#include <optional>

namespace field {
class App {
  public:
    App();
    ~App();
    void frame();
    void choosePreset(int index);
    void smokeTest();
    bool wantsExit = false;

  private:
    enum class Tool { Select, Circle, Box, Triangle, Platform, Push };
    enum class Dialog { None, Save, Load, Help, Exit };
    Simulation sim;
    Scene baseline;
    History history;
    Font font{};
    Tool tool = Tool::Select;
    Dialog dialog = Dialog::None;
    int selected = 4, presetIndex = 0, rightTab = 0;
    bool playing = false, hasBaseline = false, showGrid = true, showVectors = false, showTrails = false;
    bool drawing = false, dragging = false, panning = false, dirty = false;
    bool dragChanged = false;
    std::optional<BodySpec> dragBefore;
    float speed = 1, zoom = 1, accumulator = 0, inspectorScroll = 0;
    pe::Vector2 center{10, 4}, gestureStart{}, dragOffset{};
    Vector2 panMouse{};
    Rectangle canvas{}, transport{};
    float scale = 40;
    std::string tooltip, toast, editId, editBuffer, filename = "My experiment";
    float toastTime = 0;
    std::vector<std::string> sceneFiles;
    int fileScroll = 0;
    std::deque<float> energyHistory;
    float sampleTime = 0;
    bool blocked = false;
    void text(const std::string &s, float x, float y, float size, Color color);
    void label(const std::string &s, float x, float y);
    float textWidth(const std::string &s, float size);
    void icon(int kind, float x, float y, float size, Color color);
    bool button(Rectangle r, const std::string &title, bool active = false, const std::string &tip = "",
                int symbol = -1, bool enabled = true);
    bool number(const std::string &id, const std::string &title, Rectangle r, float &value, float lo,
                float hi, const char *unit);
    bool toggle(float x, float y, float w, const std::string &title, bool &value);
    void remember();
    void edited();
    void notify(const std::string &s);
    void playPause();
    void rewind();
    void undo(bool redo);
    void duplicate();
    void remove();
    void openDialog(Dialog d);
    void load(const std::filesystem::path &path);
    void header();
    void library();
    void inspector();
    void stage();
    void playback();
    void modal();
    void keyboard();
    void canvasInput();
    Vector2 screen(pe::Vector2 p) const;
    pe::Vector2 world(Vector2 p) const;
    void bodyDrawing(const BodySpec &b, bool highlight, float opacity = 1);
};
} // namespace field
