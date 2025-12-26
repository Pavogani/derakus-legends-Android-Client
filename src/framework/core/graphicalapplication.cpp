/*
 * Copyright (c) 2010-2025 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "graphicalapplication.h"

#include "asyncdispatcher.h"
#include "clock.h"
#include "eventdispatcher.h"
#include "garbagecollection.h"
#include "framework/graphics/drawpoolmanager.h"
#include "framework/graphics/graphics.h"
#include "framework/graphics/image.h"
#include "framework/graphics/particlemanager.h"
#include "framework/graphics/texturemanager.h"
#include "framework/input/mouse.h"
#include "framework/ui/uimanager.h"

#ifdef FRAMEWORK_SOUND
#include <framework/sound/soundmanager.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <framework/html/htmlmanager.h>
#include <framework/platform/platformwindow.h>

GraphicalApplication g_app;

void GraphicalApplication::init(std::vector<std::string>& args, ApplicationContext* context)
{
    Application::init(args, context);

    auto graphicalContext = static_cast<GraphicalApplicationContext*>(context);
    setDrawEvents(graphicalContext->getDrawEvents());

    // setup platform window
    g_window.init();
    g_window.hide();

    // set the window title color
    g_window.setTitleBarColor(Color::black);

    g_window.setOnResize([this](auto&& PH1) {
        if (!m_running) resize(PH1);
        else g_dispatcher.addEvent([&, PH1] { resize(PH1); });
    });

    g_window.setOnInputEvent([this](auto&& PH1) {
        if (!m_running) inputEvent(PH1);
        else g_dispatcher.addEvent([&, PH1] { inputEvent(PH1); });
    });

    g_window.setOnClose([this] { g_dispatcher.addEvent([this] { close(); }); });

    g_mouse.init();

    // initialize ui
    g_ui.init();

    // initialize graphics
    g_graphics.init();
    g_drawPool.init(graphicalContext->getSpriteSize());

    // fire first resize event
    resize(g_window.getSize());

#ifdef FRAMEWORK_SOUND
    // initialize sound
    g_sounds.init();
#endif

    m_mapProcessFrameCounter.init();
    m_graphicFrameCounter.init();
}

void GraphicalApplication::deinit()
{
    // hide the window because there is no render anymore
    g_window.hide();

    Application::deinit();
}

void GraphicalApplication::terminate()
{
    // destroy particles
    g_particles.terminate();

    // destroy any remaining widget
    g_html.terminate();
    g_ui.terminate();

    Application::terminate();
    m_terminated = false;

#ifdef FRAMEWORK_SOUND
    // terminate sound
    g_sounds.terminate();
#endif

    g_mouse.terminate();

    // terminate graphics
    g_drawPool.terminate();
    g_graphics.terminate();
    g_window.terminate();

    m_terminated = true;
}

#if defined(__EMSCRIPTEN__) || defined(ANDROID)
void GraphicalApplication::mainLoop() {
    static int loopCount = 0;
    static bool wasVisible = true;
    loopCount++;

    bool isVisible = g_window.isVisible();

    // Log visibility changes
    if (isVisible != wasVisible) {
        g_logger.info("Window visibility changed: " + std::string(isVisible ? "VISIBLE" : "NOT VISIBLE"));
        wasVisible = isVisible;
    }

    if (loopCount <= 5 || loopCount % 60 == 0) {
        g_logger.info("mainLoop #" + std::to_string(loopCount) +
            " stopping=" + std::to_string(m_stopping ? 1 : 0) +
            " visible=" + std::to_string(isVisible ? 1 : 0));
    }

    if (m_stopping) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        MAIN_THREAD_EM_ASM({ window.location.reload(); });
#endif
        return;
    }

    // Poll events - this will process APP_CMD_INIT_WINDOW if the surface is recreated
    mainPoll();
    poll();

    if (!g_window.isVisible()) {
        // Keep polling frequently to catch window recreation events
        stdext::millisleep(10);
        return;
    }

    const auto FPS = [this] {
        m_mapProcessFrameCounter.setTargetFps(g_window.vsyncEnabled() || getMaxFps() || getTargetFps() ? 500u : 0u);
        return m_graphicFrameCounter.getFps();
    };

#ifdef ANDROID
    // On Android, handle draw events here (replaces mapThread's draw logic)
    if (m_drawEvents->canDraw(DrawPoolType::MAP)) {
        m_drawEvents->preLoad();
        m_drawEvents->draw(DrawPoolType::LIGHT);
        m_drawEvents->draw(DrawPoolType::FOREGROUND);
        m_drawEvents->draw(DrawPoolType::FOREGROUND_MAP);
        m_drawEvents->draw(DrawPoolType::MAP);
    } else if (m_drawEvents->canDraw(DrawPoolType::FOREGROUND)) {
        g_ui.render(DrawPoolType::FOREGROUND);
    }
    m_mapProcessFrameCounter.update();
#endif

    g_drawPool.draw();
    g_window.swapBuffers();

    static int swapCount = 0;
    swapCount++;
    if (swapCount <= 5 || swapCount % 60 == 0) {
        g_logger.info("swapBuffers #" + std::to_string(swapCount));
    }

    if (m_graphicFrameCounter.update()) {
        g_dispatcher.addEvent([this, fps = FPS()] {
            g_lua.callGlobalField("g_app", "onFps", fps);
        });
    }
}
#endif

void GraphicalApplication::run()
{
    g_logger.info("run() entered");

    // run the first poll
    mainPoll();
    poll();
    g_logger.info("run() first poll done");

    // show window
    g_window.show();
    g_logger.info("run() window.show() done");

    // run the second poll
    mainPoll();
    poll();
    g_logger.info("run() second poll done");

    g_lua.callGlobalField("g_app", "onRun");
    g_logger.info("run() onRun callback done");

#ifndef __EMSCRIPTEN__
    const auto FPS = [this] {
        m_mapProcessFrameCounter.setTargetFps(g_window.vsyncEnabled() || getMaxFps() || getTargetFps() ? 500u : 0u);
        return m_graphicFrameCounter.getFps();
    };
#endif
    // THREAD - POOL & MAP
    // On Android, we skip the mapThread and handle poll/draw in mainLoop() instead
    // to avoid SpinLock deadlocks between the main thread and mapThread
#ifndef ANDROID
    g_logger.info("run() about to submit mapThread");
    const auto& mapThread = g_asyncDispatcher.submit_task([this] {
        BS::multi_future<void> tasks;

        g_luaThreadId = g_eventThreadId = stdext::getThreadId();
        while (!m_stopping) {
            poll();

            if (!g_window.isVisible()) {
                stdext::millisleep(10);
                continue;
            }

            if (m_drawEvents->canDraw(DrawPoolType::MAP)) {
                m_drawEvents->preLoad();

                for (const auto type : { DrawPoolType::LIGHT , DrawPoolType::FOREGROUND, DrawPoolType::FOREGROUND_MAP }) {
                    if (m_drawEvents->canDraw(type)) {
                        tasks.emplace_back(g_asyncDispatcher.submit_task([this, type] {
                            m_drawEvents->draw(type);
                        }));
                    }
                }

                m_drawEvents->draw(DrawPoolType::MAP);

                tasks.wait();
                tasks.clear();
            } else if (m_drawEvents->canDraw(DrawPoolType::FOREGROUND)) {
                g_ui.render(DrawPoolType::FOREGROUND);
            }

            m_mapProcessFrameCounter.update();
        }
    });
    g_logger.info("run() mapThread submitted");
#endif

#if defined(__EMSCRIPTEN__)
    m_running = true;
    emscripten_set_main_loop(([] { g_app.mainLoop(); }), 0, 1);
#elif defined(ANDROID)
    g_logger.info("run() Android block reached, setting m_running=true");
    m_running = true;
    g_luaThreadId = g_eventThreadId = stdext::getThreadId();
    // Android uses mainLoop() callback from android_main.cpp
    // No mapThread - poll/draw is handled in mainLoop() to avoid SpinLock deadlocks
    g_logger.info("run() Android returning to android_main");
    return;
#else
    m_running = true;
    while (!m_stopping) {
        mainPoll();

        if (!g_window.isVisible()) {
            stdext::millisleep(10);
            continue;
        }

        g_drawPool.draw();

        // update screen pixels
        g_window.swapBuffers();

        if (m_graphicFrameCounter.update()) {
            g_dispatcher.addEvent([this, fps = FPS()] {
                g_lua.callGlobalField("g_app", "onFps", fps);
            });
        }
    }
#endif

#ifndef ANDROID
    mapThread.wait();
#endif

    m_running = false;
    m_stopping = false;

    g_luaThreadId = g_eventThreadId = -1;
}

void GraphicalApplication::poll()
{
    static bool firstCall = true;
    if (firstCall) g_logger.info("poll() - GarbageCollection");
    GarbageCollection::poll();
    if (firstCall) g_logger.info("poll() - Application::poll");
    Application::poll();

#ifdef FRAMEWORK_SOUND
    if (firstCall) g_logger.info("poll() - sounds");
    g_sounds.poll();
#endif

    if (firstCall) g_logger.info("poll() - particles");
    g_particles.poll();

    if (!g_window.isVisible()) {
        if (firstCall) g_logger.info("poll() - textDispatcher (window not visible)");
        g_textDispatcher.poll();
    }
    if (firstCall) {
        g_logger.info("poll() - done");
        firstCall = false;
    }
}
void GraphicalApplication::mainPoll()
{
    static bool firstCall = true;
    if (firstCall) g_logger.info("mainPoll() - clock.update");
    g_clock.update();
    if (firstCall) g_logger.info("mainPoll() - mainDispatcher.poll");
    g_mainDispatcher.poll();
    if (firstCall) g_logger.info("mainPoll() - window.poll");
    g_window.poll();
    if (firstCall) g_logger.info("mainPoll() - textures.poll");
    g_textures.poll();
    if (firstCall) {
        g_logger.info("mainPoll() - done");
        firstCall = false;
    }
}

void GraphicalApplication::close()
{
    m_onInputEvent = true;
    Application::close();
    m_onInputEvent = false;
}

static constexpr bool USE_FRAMEBUFFER = false;
void GraphicalApplication::resize(const Size& size)
{
    const float scale = g_window.getDisplayDensity();
    g_graphics.resize(size);

    m_onInputEvent = true;
    g_ui.resize(size / scale);
    m_onInputEvent = false;

    g_mainDispatcher.addEvent([size, scale] {
        g_drawPool.get(DrawPoolType::FOREGROUND)->setFramebuffer(size / scale);
    });
}

void GraphicalApplication::inputEvent(const InputEvent& event)
{
    m_onInputEvent = true;
    g_ui.inputEvent(event);
    m_onInputEvent = false;
}

bool GraphicalApplication::isLoadingAsyncTexture() { return m_loadingAsyncTexture || (m_drawEvents && m_drawEvents->isLoadingAsyncTexture()); }
bool GraphicalApplication::isScaled() { return g_window.getDisplayDensity() != 1.f; }
void GraphicalApplication::setLoadingAsyncTexture(bool v) {
    if (m_drawEvents && m_drawEvents->isUsingProtobuf())
        v = true;
    else if (isEncrypted())
        v = false;

    m_loadingAsyncTexture = v;

    if (m_drawEvents)
        m_drawEvents->onLoadingAsyncTextureChanged(v);
}

void GraphicalApplication::doScreenshot(std::string file)
{
    if (file.empty()) {
        file = "screenshot.png";
    }

    g_mainDispatcher.addEvent([file] {
        auto resolution = g_graphics.getViewportSize();
        const int width = resolution.width();
        const int height = resolution.height();
        auto pixels = std::make_shared<std::vector<uint8_t>>(width * height * 4 * sizeof(GLubyte), 0);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels->data());

        g_asyncDispatcher.detach_task([resolution, pixels, file] {
            try {
                Image image(resolution, 4, pixels->data());
                image.flipVertically();
                image.setOpacity(255);
                image.savePNG(file);
            } catch (stdext::exception& e) {
                g_logger.error(std::string("Can't do screenshot: ") + e.what());
            }
        });
    });
}

void GraphicalApplication::doMapScreenshot(std::string fileName)
{
    if (m_drawEvents) m_drawEvents->doMapScreenshot(fileName);
}

float GraphicalApplication::getHUDScale() const { return g_window.getDisplayDensity(); }
void GraphicalApplication::setHUDScale(const float v) {
    g_window.setDisplayDensity(v);
    resize(g_graphics.getViewportSize());
}