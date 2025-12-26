/*
 * Copyright (c) 2010-2014 OTClient <https://github.com/edubart/otclient>
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

#ifdef ANDROID

#include "androidwindow.h"
#include "androidmanager.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include "framework/core/clock.h"
#include <framework/core/eventdispatcher.h>

AndroidWindow& g_androidWindow = (AndroidWindow&) g_window;

AndroidWindow::AndroidWindow() {
    m_minimumSize = Size(600, 480);
    m_size = Size(600, 480);

    m_keyMap[AndroidWindow::KEY_ENTER] = Fw::KeyEnter;
    m_keyMap[AndroidWindow::KEY_BACKSPACE] = Fw::KeyBackspace;
}

AndroidWindow::~AndroidWindow() {
    internalDestroyGLContext();
}

AndroidWindow::KeyCode AndroidWindow::NativeEvent::getKeyCodeFromInt(int keyCode) {
    switch (keyCode) {
        case 66:
            return KEY_ENTER;
        case 67:
            return KEY_BACKSPACE;
        default:
            return KEY_UNDEFINED;
    }
}

AndroidWindow::EventType AndroidWindow::NativeEvent::getEventTypeFromInt(int actionType) {
    switch (actionType) {
        case 0:
            return TOUCH_DOWN;
        case 1:
            return TOUCH_UP;
        case 2:
            return TOUCH_MOTION;
        case 3:
            return TOUCH_LONGPRESS;
        default:
            return EVENT_UNDEFINED;
    }
}

void AndroidWindow::internalInitGL() {
    internalCheckGL();
    internalChooseGL();
    internalCreateGLSurface();
    internalCreateGLContext();
}

void AndroidWindow::internalCheckGL() {
    if (m_eglDisplay != EGL_NO_DISPLAY) {
        return;
    }

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(m_eglDisplay == EGL_NO_DISPLAY)
        g_logger.fatal("EGL not supported");

    if(!eglInitialize(m_eglDisplay, NULL, NULL))
        g_logger.fatal("Unable to initialize EGL");
}

void AndroidWindow::internalChooseGL() {
    static int attrList[] = {
#if OPENGL_ES==2
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
#endif
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 16,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLint numConfig;

    if(!eglChooseConfig(m_eglDisplay, attrList, &m_eglConfig, 1, &numConfig))
        g_logger.fatal("Failed to choose EGL config");

    if(numConfig != 1)
        g_logger.warning("Didn't got the exact EGL config");

    EGLint vid;
    if(!eglGetConfigAttrib(m_eglDisplay, m_eglConfig, EGL_NATIVE_VISUAL_ID, &vid))
        g_logger.fatal("Unable to get visual EGL visual id");
}

void AndroidWindow::internalCreateGLContext() {
    if (m_eglContext != EGL_NO_CONTEXT) {
        return;
    }

    EGLint attrList[] = {
#if OPENGL_ES==2
        EGL_CONTEXT_CLIENT_VERSION, 2,
#else
        EGL_CONTEXT_CLIENT_VERSION, 1,
#endif
        EGL_NONE
    };

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, attrList);
    if(m_eglContext == EGL_NO_CONTEXT )
        g_logger.fatal("Unable to create EGL context: {}", eglGetError());

    internalConnectSurface();
}

void AndroidWindow::internalConnectSurface() {
    if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext))
        g_logger.fatal("Unable to connect EGL context into Android native window");
}

void AndroidWindow::internalDestroyGLContext() {
    if(m_eglDisplay) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        internalDestroySurface();

        if(m_eglContext) {
            eglDestroyContext(m_eglDisplay, m_eglContext);
            m_eglContext = EGL_NO_CONTEXT;
        }

        eglTerminate(m_eglDisplay);
        m_eglDisplay = EGL_NO_DISPLAY;
    }
}

void AndroidWindow::internalDestroySurface() {
    if (m_eglSurface) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }
}

void AndroidWindow::internalCreateGLSurface() {
    if (m_eglSurface != EGL_NO_SURFACE) {
        return;
    }

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_app->window, NULL);
    if(m_eglSurface == EGL_NO_SURFACE)
        g_logger.fatal("Unable to create EGL surface: {}", eglGetError());
}

void AndroidWindow::queryGlSize() {
    int width, height;
    if (EGL_FALSE == eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_WIDTH, &width) ||
        EGL_FALSE ==  eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_HEIGHT, &height)) {
        g_logger.fatal("Unable to query surface to get width and height");
    }
    m_size = Size(width, height);
}

void AndroidWindow::terminate() {
    __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "AndroidWindow::terminate() called - hiding window");
    m_visible = false;
    internalDestroyGLContext();
}

void AndroidWindow::poll() {
    handleNativeEvents();

    while( !m_events.empty() ) {
        m_currentEvent = m_events.front();

        switch( m_currentEvent.type ) {
            case TEXTINPUT:
                processTextInput();
                break;
            case TOUCH_DOWN:
            case TOUCH_LONGPRESS:
            case TOUCH_UP:
                processFingerDownAndUp();
                break;
            case TOUCH_MOTION:
                processFingerMotion();
            case KEY_DOWN:
            case KEY_UP:
            case EVENT_UNDEFINED:
                processKeyDownOrUp();
                break;
        }

        m_events.pop();
    }

    fireKeysPress();
}

void AndroidWindow::processKeyDownOrUp() {
    if(m_currentEvent.type == KEY_DOWN || m_currentEvent.type == KEY_UP) {
        Fw::Key keyCode = Fw::KeyUnknown;
        KeyCode key = m_currentEvent.keyCode;

        if(m_keyMap.find(key) != m_keyMap.end())
            keyCode = m_keyMap[key];

        if(m_currentEvent.type == KEY_DOWN)
            processKeyDown(keyCode);
        else if(m_currentEvent.type == KEY_UP)
            processKeyUp(keyCode);
    }
}

void AndroidWindow::processTextInput() {
    std::string text = m_currentEvent.text;
    KeyCode keyCode = m_currentEvent.keyCode;

    if(text.length() == 0 || keyCode == KEY_ENTER || keyCode == KEY_BACKSPACE)
        return;

    if(m_onInputEvent) {
        m_inputEvent.reset(Fw::KeyTextInputEvent);
        m_inputEvent.keyText = text;
        m_onInputEvent(m_inputEvent);
    }
}

void AndroidWindow::processFingerDownAndUp() {
    bool isTouchdown = m_currentEvent.type == TOUCH_DOWN;

    Fw::MouseButton mouseButton = (m_currentEvent.type == TOUCH_UP && !m_isDragging && stdext::millis() > m_lastPress + 500) ?
        Fw::MouseRightButton : Fw::MouseLeftButton;

    m_inputEvent.reset();
    m_inputEvent.type = (isTouchdown) ? Fw::MousePressInputEvent : Fw::MouseReleaseInputEvent;
    m_inputEvent.mouseButton = mouseButton;

    if (isTouchdown) {
        m_lastPress = g_clock.millis();
        m_mouseButtonStates |= 1 << mouseButton;
    } else if (m_currentEvent.type == TOUCH_UP) {
        if (!m_isDragging) {
            if (stdext::millis() > m_lastPress + 500) {
                mouseButton = Fw::MouseRightButton;
                m_inputEvent.mouseButton = mouseButton;
            }
        }
        m_isDragging = false;
        g_dispatcher.addEvent([this, mouseButton] { m_mouseButtonStates &= ~(1 << mouseButton); });
    }

    handleInputEvent();
}

void AndroidWindow::processFingerMotion() {
    static Point lastMousePos(-1, -1);
    static const int dragThreshold = 5;

    m_inputEvent.reset();
    m_inputEvent.type = Fw::MouseMoveInputEvent;

    Point newMousePos(m_currentEvent.x / m_displayDensity, m_currentEvent.y / m_displayDensity);

    if (lastMousePos.x != -1 && lastMousePos.y != -1) {
        int dx = std::abs(newMousePos.x - lastMousePos.x);
        int dy = std::abs(newMousePos.y - lastMousePos.y);

        if (dx > dragThreshold || dy > dragThreshold) {
            m_isDragging = true;
        }
    }
    lastMousePos = newMousePos;

    handleInputEvent();
}

void AndroidWindow::handleInputEvent() {
    Point newMousePos(m_currentEvent.x / m_displayDensity, m_currentEvent.y / m_displayDensity);
    m_inputEvent.mouseMoved = newMousePos - m_inputEvent.mousePos;
    m_inputEvent.mousePos = newMousePos;

    if (m_onInputEvent)
        m_onInputEvent(m_inputEvent);
}

void AndroidWindow::swapBuffers() {
    static int swapCount = 0;
    if (++swapCount % 60 == 0) {
        __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "swapBuffers called %d times (60 FPS = working)", swapCount);
    }
    eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

void AndroidWindow::setVerticalSync(bool enable) {
    eglSwapInterval(m_eglDisplay, enable ? 1 : 0);
}

std::string AndroidWindow::getClipboardText() {
    try {
        return g_androidManager.getClipboardText();
    } catch (const std::exception& e) {
        return "";
    }
}

void AndroidWindow::setClipboardText(const std::string_view text) {
    try {
        g_androidManager.setClipboardText(text);
    } catch (const std::exception& e) {
    }
}

Size AndroidWindow::getDisplaySize() {
    return m_size;
}

std::string AndroidWindow::getPlatformType() {
    return "ANDROID-EGL";
}

/* Does not apply to Android */
void AndroidWindow::init() {}

void AndroidWindow::show() {}

void AndroidWindow::hide() {}

void AndroidWindow::maximize() {}

void AndroidWindow::move(const Point& pos) {}

void AndroidWindow::resize(const Size& size) {}

void AndroidWindow::showMouse() {}

void AndroidWindow::hideMouse() {}

int AndroidWindow::internalLoadMouseCursor(const ImagePtr& image, const Point& hotSpot) { return 0; }

void AndroidWindow::setMouseCursor(int cursorId) {}

void AndroidWindow::restoreMouseCursor() {}

void AndroidWindow::setTitle(const std::string_view title) {}

void AndroidWindow::setMinimumSize(const Size& minimumSize) {}

void AndroidWindow::setFullscreen(bool fullscreen) {}

void AndroidWindow::setIcon(const std::string& iconFile) {}

/* Android specific thngs */
void AndroidWindow::initializeAndroidApp(android_app* app) {
    m_app = app;
    m_app->userData = this;
    m_app->onAppCmd = [](struct android_app * app, int32_t cmd) {
        auto *engine = (AndroidWindow*) app->userData;
        engine->handleCmd(cmd);
    };

    android_app_set_key_event_filter(m_app, NULL);
    android_app_set_motion_event_filter(m_app, NULL);
}

void AndroidWindow::onDisplayDensityChanged(float newDensity) {
    m_baseDisplayDensity = newDensity;
    m_hasBaseDisplayDensity = true;
}

void AndroidWindow::onNativeTouch(int actionType,
                                  uint32_t pointerIndex,
                                  GameActivityMotionEvent* motionEvent) {
    float x = GameActivityPointerAxes_getX(&motionEvent->pointers[pointerIndex]);
    float y = GameActivityPointerAxes_getY(&motionEvent->pointers[pointerIndex]);

    EventType type = NativeEvent::getEventTypeFromInt(actionType);

    m_events.push(NativeEvent(type, x, y));
}

void AndroidWindow::onNativeKeyDown( int keyCode ) {
    KeyCode key = NativeEvent::getKeyCodeFromInt(keyCode);

    m_events.push(NativeEvent(KEY_DOWN, key));
}

void AndroidWindow::onNativeKeyUp( int keyCode ) {
    KeyCode key = NativeEvent::getKeyCodeFromInt(keyCode);

    m_events.push(NativeEvent(KEY_UP, key));
}

void AndroidWindow::nativeCommitText(jstring jString) {
    std::string text = g_androidManager.getStringFromJString(jString);
    m_events.push(NativeEvent(TEXTINPUT, text));
}

void AndroidWindow::handleNativeEvents() {
    int events;
    struct android_poll_source* source;

    // Always use non-blocking poll (timeout = 0) to avoid deadlocks
    // The render loop will call this repeatedly, so we don't need to block
    while ((ALooper_pollOnce(0, nullptr, &events, (void**)&source)) >= 0) {
        if (source != nullptr) {
            source->process(m_app, source);
        }

        if (m_app->destroyRequested) {
            internalDestroySurface();
            return;
        }
    }

    processNativeInputEvents();
}

void AndroidWindow::handleCmd(int32_t cmd) {
    switch (cmd) {
        case APP_CMD_START:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_START - activity starting");
            break;
        case APP_CMD_RESUME:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_RESUME - activity resuming, window=%p", m_app->window);
            // If we have a window but no surface, try to recreate it
            if (m_app->window != nullptr && m_eglSurface == EGL_NO_SURFACE) {
                __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_RESUME - recreating surface");
                if (m_eglContext != EGL_NO_CONTEXT) {
                    internalCreateGLSurface();
                    internalConnectSurface();
                    updateDisplayDensityFromSystem(g_androidManager.getScreenDensity());
                    queryGlSize();
                    m_visible = true;
                    __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_RESUME - surface recreated, visible=true");
                }
            }
            break;
        case APP_CMD_INIT_WINDOW:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_INIT_WINDOW received, window=%p", m_app->window);
            if (m_app->window != nullptr) {
                if (m_eglContext != EGL_NO_CONTEXT) {
                    __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "Creating GL surface (context exists)");
                    internalCreateGLSurface();
                    internalConnectSurface();
                } else {
                    __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "Initializing GL (first time)");
                    internalInitGL();
                }
                updateDisplayDensityFromSystem(g_androidManager.getScreenDensity());
                queryGlSize();
                __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "Window visible, size=%dx%d", m_size.width(), m_size.height());
                m_visible = true;
            } else {
                __android_log_print(ANDROID_LOG_ERROR, "OTClientMobile", "APP_CMD_INIT_WINDOW but m_app->window is null!");
                m_visible = false;
            }
            break;
        case APP_CMD_GAINED_FOCUS:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_GAINED_FOCUS");
            break;
        case APP_CMD_LOST_FOCUS:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_LOST_FOCUS");
            break;
        case APP_CMD_PAUSE:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_PAUSE - activity pausing");
            break;
        case APP_CMD_STOP:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_STOP - activity stopping");
            break;
        case APP_CMD_LOW_MEMORY:
            __android_log_print(ANDROID_LOG_WARN, "OTClientMobile", "APP_CMD_LOW_MEMORY received - NOT hiding window");
            // Don't hide window on low memory, just log it
            break;
        case APP_CMD_TERM_WINDOW:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_TERM_WINDOW - hiding window, surface=%p", (void*)m_eglSurface);
            m_visible = false;
            internalDestroySurface();
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONFIG_CHANGED:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "Window resized/config changed");
            updateDisplayDensityFromSystem(g_androidManager.getScreenDensity());
            queryGlSize();
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "New size=%dx%d", m_size.width(), m_size.height());
            break;
        case APP_CMD_DESTROY:
            __android_log_print(ANDROID_LOG_INFO, "OTClientMobile", "APP_CMD_DESTROY - activity being destroyed");
            break;
        default:
            __android_log_print(ANDROID_LOG_DEBUG, "OTClientMobile", "Unhandled APP_CMD: %d", cmd);
            break;
    }
}

void AndroidWindow::updateDisplayDensityFromSystem(float screenDensity) {
    // For mobile, we always use density 1.0 so the UI fills the entire screen
    // The UI elements are already designed to be touch-friendly at pixel resolution
    m_displayDensity = 1.0f;
    m_baseDisplayDensity = 1.0f;
    m_hasBaseDisplayDensity = true;

    __android_log_print(ANDROID_LOG_INFO, "OTClientMobile",
        "Display density set to 1.0 (screen density was %.2f)", screenDensity);
}

float AndroidWindow::calculatePinchDistance(GameActivityMotionEvent* motionEvent) {
    if (motionEvent->pointerCount < 2) return 0.0f;

    float x1 = GameActivityPointerAxes_getX(&motionEvent->pointers[0]);
    float y1 = GameActivityPointerAxes_getY(&motionEvent->pointers[0]);
    float x2 = GameActivityPointerAxes_getX(&motionEvent->pointers[1]);
    float y2 = GameActivityPointerAxes_getY(&motionEvent->pointers[1]);

    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

void AndroidWindow::handlePinchGesture(GameActivityMotionEvent* motionEvent, int actionMasked) {
    if (motionEvent->pointerCount < 2) {
        // End pinch if we don't have 2 fingers
        if (m_isPinching) {
            m_isPinching = false;
            m_initialPinchDistance = 0.0f;
            m_lastPinchDistance = 0.0f;
        }
        return;
    }

    float currentDistance = calculatePinchDistance(motionEvent);

    if (actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        // Second finger down - start pinch
        m_isPinching = true;
        m_initialPinchDistance = currentDistance;
        m_lastPinchDistance = currentDistance;
    } else if (actionMasked == AMOTION_EVENT_ACTION_MOVE && m_isPinching) {
        // Calculate zoom delta based on distance change
        if (m_lastPinchDistance > 0.0f) {
            float distanceChange = currentDistance - m_lastPinchDistance;
            // Normalize: positive = zoom in, negative = zoom out
            // Scale factor to make zooming feel natural
            float zoomDelta = distanceChange / 100.0f;

            // Only fire event if there's meaningful movement
            if (std::abs(zoomDelta) > 0.01f) {
                m_inputEvent.reset(Fw::PinchZoomInputEvent);
                m_inputEvent.zoomDelta = zoomDelta;

                // Set mouse position to center of pinch
                float x1 = GameActivityPointerAxes_getX(&motionEvent->pointers[0]);
                float y1 = GameActivityPointerAxes_getY(&motionEvent->pointers[0]);
                float x2 = GameActivityPointerAxes_getX(&motionEvent->pointers[1]);
                float y2 = GameActivityPointerAxes_getY(&motionEvent->pointers[1]);
                m_inputEvent.mousePos = Point(
                    static_cast<int>((x1 + x2) / 2.0f / m_displayDensity),
                    static_cast<int>((y1 + y2) / 2.0f / m_displayDensity)
                );

                if (m_onInputEvent)
                    m_onInputEvent(m_inputEvent);
            }
        }
        m_lastPinchDistance = currentDistance;
    } else if (actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
        // Finger lifted - end pinch
        m_isPinching = false;
        m_initialPinchDistance = 0.0f;
        m_lastPinchDistance = 0.0f;
    }
}

void AndroidWindow::processNativeInputEvents() {
    android_input_buffer* inputBuffer = android_app_swap_input_buffers(m_app);

    if (inputBuffer == nullptr) return;

    if (inputBuffer->motionEventsCount != 0) {
        for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i) {
            auto* motionEvent = &inputBuffer->motionEvents[i];
            const int action = motionEvent->action;
            const int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
            uint32_t pointerIndex = GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT;

            // Handle pinch-to-zoom gesture for multi-touch
            if (motionEvent->pointerCount >= 2) {
                handlePinchGesture(motionEvent, actionMasked);

                // Skip normal touch handling during pinch
                if (m_isPinching) {
                    continue;
                }
            } else if (m_isPinching) {
                // End pinch when we go back to single touch
                m_isPinching = false;
                m_initialPinchDistance = 0.0f;
                m_lastPinchDistance = 0.0f;
            }

            switch (actionMasked) {
                case AMOTION_EVENT_ACTION_UP:
                case AMOTION_EVENT_ACTION_DOWN:
                    pointerIndex = 0;
                    break;
                case AMOTION_EVENT_ACTION_POINTER_UP:
                case AMOTION_EVENT_ACTION_POINTER_DOWN:
                    pointerIndex = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                    break;
                case AMOTION_EVENT_ACTION_MOVE: {
                    for (uint32_t innerPointerIndex = 0; innerPointerIndex < motionEvent->pointerCount; innerPointerIndex++) {
                        onNativeTouch(actionMasked, innerPointerIndex, motionEvent);
                    }
                    break;
                }
                default:
                    break;
            }

            if (pointerIndex != GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT) {
                onNativeTouch(actionMasked, pointerIndex, motionEvent);
            }
        }
        android_app_clear_motion_events(inputBuffer);
    }
}

extern "C" {
void Java_com_derakus_legends_NativeInputConnection_nativeCommitText(
        JNIEnv* env, jobject obj, jstring text) {
    ((AndroidWindow&) g_window).nativeCommitText(text);
}

void Java_com_derakus_legends_FakeEditText_onNativeKeyDown(
        JNIEnv* env, jobject obj, jint keyCode ) {
    ((AndroidWindow&) g_window).onNativeKeyDown(keyCode);
}

void Java_com_derakus_legends_FakeEditText_onNativeKeyUp(
        JNIEnv* env, jobject obj, jint keyCode ) {
    ((AndroidWindow&) g_window).onNativeKeyUp(keyCode);
}
}

#endif // ANDROID
