#ifdef ANDROID

#include "framework/platform/androidwindow.h"
#include "framework/core/graphicalapplication.h"
#include "framework/stdext/stdext.h"
#include <cstdlib>
#include <thread>
#include <chrono>

int main(int argc, const char* argv[]);

void android_main(struct android_app* app) {
    g_androidManager.setAndroidApp(app);
    g_androidWindow.initializeAndroidApp(app);
    g_logger.info("android_main: initialized");

    bool terminated = false;
    g_window.setOnClose([&] {
        terminated = true;
    });

    // Wait for window to become visible
    g_logger.info("android_main: waiting for window visible");
    while(!g_window.isVisible() && !terminated) {
        g_window.poll();
    }
    g_logger.info("android_main: window visible, calling main()");

    // Initialize the application
    const char* args[] = { "OTClient" };
    main(1, args);

    g_logger.info("android_main: main() returned, isRunning=" + std::to_string(g_app.isRunning() ? 1 : 0));

    // Run the render loop - mainLoop() handles visibility internally
    // If window is not visible, mainLoop will poll for events and wait
    int loopIter = 0;
    g_logger.info("android_main: starting render loop, visible=" + std::to_string(g_window.isVisible() ? 1 : 0));
    while(!terminated && g_app.isRunning()) {
        loopIter++;
        if (loopIter <= 5 || loopIter % 60 == 0) {
            g_logger.info("android_main: render loop #" + std::to_string(loopIter) +
                " visible=" + std::to_string(g_window.isVisible() ? 1 : 0));
        }
        g_app.mainLoop();
    }

    g_logger.info("android_main: exiting, terminated=" + std::to_string(terminated ? 1 : 0));
    std::exit(0);
}

#endif