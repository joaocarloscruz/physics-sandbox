#include "ui.h"
#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
    int frames = 0, example = 0, width = 1440, height = 900;
    bool smoke = false;
    std::string screenshot;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc)
            screenshot = argv[++i];
        else if (arg == "--frames" && i + 1 < argc)
            frames = std::max(1, std::atoi(argv[++i]));
        else if (arg == "--preset" && i + 1 < argc)
            example = std::atoi(argv[++i]);
        else if (arg == "--width" && i + 1 < argc)
            width = std::max(1120, std::atoi(argv[++i]));
        else if (arg == "--height" && i + 1 < argc)
            height = std::max(740, std::atoi(argv[++i]));
        else if (arg == "--smoke-test")
            smoke = true;
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT |
                   ((frames || smoke) ? FLAG_WINDOW_HIDDEN | FLAG_WINDOW_ALWAYS_RUN : 0));
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, "Field - Physics Sandbox");
    SetWindowMinSize(1120, 740);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    // Resources and scene files follow the executable, independent of launch cwd.
    ChangeDirectory(GetApplicationDirectory());
    try {
        field::App app;
        if (smoke) {
            app.smokeTest();
            app.wantsExit = true;
        }
        if (example)
            app.choosePreset(example);
        int frame = 0;
        while (!app.wantsExit) {
            app.frame();
            if (frames && ++frame >= frames) {
                if (!screenshot.empty()) {
                    Image capture = LoadImageFromScreen();
                    bool saved = ExportImage(capture, screenshot.c_str());
                    UnloadImage(capture);
                    if (!saved)
                        throw std::runtime_error("Could not save screenshot.");
                }
                break;
            }
        }
    } catch (const std::exception &error) {
        std::cerr << "Field: " << error.what() << '\n';
        CloseWindow();
        return 1;
    }
    CloseWindow();
    return 0;
}
