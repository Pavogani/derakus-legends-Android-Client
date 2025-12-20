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

#pragma once

#ifdef ANDROID

#include "platformwindow.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <EGL/egl.h>
#include <queue>
#include <game-activity/native_app_glue/android_native_app_glue.h>

class AndroidWindow : public PlatformWindow
{
    enum KeyCode {
        KEY_UNDEFINED,
        KEY_BACKSPACE = 66,
        KEY_ENTER = 67
    };

    enum EventType {
        TOUCH_DOWN,
        TOUCH_UP,
        TOUCH_MOTION,
        TOUCH_LONGPRESS,
        KEY_DOWN,
        KEY_UP,
        TEXTINPUT,
        EVENT_UNDEFINED
    };

    struct NativeEvent {
        NativeEvent() {}

        NativeEvent(EventType type, float x, float y) :
            type(type), text(""), keyCode(KEY_UNDEFINED), x(x), y(y) {}

        NativeEvent(EventType type, std::string text) :
            type(type), text(text), keyCode(KEY_UNDEFINED), x(0), y(0) {}

        NativeEvent(EventType type, KeyCode keyCode) :
            type(type), text(""), keyCode(keyCode), x(0), y(0) {}

        static KeyCode getKeyCodeFromInt(int);
        static EventType getEventTypeFromInt(int);

        EventType type;
        std::string text;
        KeyCode keyCode;
        float x;
        float y;
    };

    void internalInitGL();
    void internalCheckGL();
    void internalChooseGL();
    void internalCreateGLContext();
    void internalCreateGLSurface();
    void internalConnectSurface();
    void queryGlSize();

    void internalDestroyGLContext();
    void internalDestroySurface();

    void processNativeInputEvents();

    void onNativeTouch(int actionType,
                       uint32_t pointerIndex,
                       GameActivityMotionEvent* motionEvent);

    // Pinch-to-zoom helpers
    float calculatePinchDistance(GameActivityMotionEvent* motionEvent);
    void handlePinchGesture(GameActivityMotionEvent* motionEvent, int actionMasked);

    void processKeyDownOrUp();
    void processTextInput();
    void processFingerDownAndUp();
    void processFingerMotion();
    void handleInputEvent();

    void handleNativeEvents();
    void handleCmd(int32_t cmd);
public:
    AndroidWindow();
    ~AndroidWindow();

    void init() override;
    void terminate() override;

    void move(const Point& pos) override;
    void resize(const Size& size) override;
    void show() override;
    void hide() override;
    void maximize() override;
    void poll() override;
    void swapBuffers() override;
    void showMouse() override;
    void hideMouse() override;

    void setMouseCursor(int cursorId) override;
    void restoreMouseCursor() override;

    void setTitle(const std::string_view title) override;
    void setMinimumSize(const Size& minimumSize) override;
    void setFullscreen(bool fullscreen) override;
    void setVerticalSync(bool enable) override;
    void setIcon(const std::string& iconFile) override;
    void setClipboardText(const std::string_view text) override;

    Size getDisplaySize() override;
    std::string getClipboardText() override;
    std::string getPlatformType() override;

    void initializeAndroidApp(android_app* app);
    void nativeCommitText(jstring);
    void onNativeKeyDown(int);
    void onNativeKeyUp(int);
protected:
    int internalLoadMouseCursor(const ImagePtr& image, const Point& hotSpot) override;
    void onDisplayDensityChanged(float newDensity) override;
private:
    android_app* m_app;

    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    EGLDisplay m_eglDisplay;
    EGLSurface m_eglSurface;

    std::queue<NativeEvent> m_events;
    NativeEvent m_currentEvent;

    ticks_t m_lastPress = 0;
    bool m_isDragging = false;

    // Pinch-to-zoom gesture tracking
    bool m_isPinching = false;
    float m_initialPinchDistance = 0.0f;
    float m_lastPinchDistance = 0.0f;

    float m_baseDisplayDensity{ DEFAULT_DISPLAY_DENSITY };
    bool m_hasBaseDisplayDensity{ false };

    void updateDisplayDensityFromSystem(float screenDensity);
};

extern "C" {
void Java_com_derakus_legends_NativeInputConnection_nativeCommitText(
        JNIEnv*, jobject, jstring );

void Java_com_derakus_legends_FakeEditText_onNativeKeyDown(
        JNIEnv*, jobject, jint );

void Java_com_derakus_legends_FakeEditText_onNativeKeyUp(
        JNIEnv*, jobject, jint );
}

extern AndroidWindow& g_androidWindow;

#endif
