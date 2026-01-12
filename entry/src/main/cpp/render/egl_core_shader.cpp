/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <hilog/log.h>
#include <random>
#include <cmath>
#include <functional>
#include "egl_core_shader.h"
#include "plugin_common.h"

#ifndef EGL_GL_COLORSPACE_KHR
#define EGL_GL_COLORSPACE_KHR 0x309D
#endif
#ifndef EGL_GL_COLORSPACE_SRGB_KHR
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#endif

const char *GAME_SYNC_NAME = "openglVSync";
char g_vertexShader[] = "#version 300 es\n"
                      "layout(location = 0) in vec4 a_position;\n"
                      "layout(location = 1) in vec4 a_color;\n"
                      "out vec4 v_color;\n"
                      "void main()\n"
                      "{\n"
                      "   gl_Position = a_position;\n"
                      "   v_color = a_color;\n"
                      "}\n";
char g_fragmentShader[] = "#version 300 es\n"
                        "precision mediump float;\n"
                        "in vec4 v_color;\n"
                        "out vec4 fragColor;\n"
                        "void main()\n"
                        "{\n"
                        "   fragColor = v_color;\n"
                        "}\n";
struct GameObject {
    float x, y;
    float width, height;
    float speedX, speedY;
    bool active;
};

GameObject g_player;
GameObject g_obstacles[25];
int g_score = 0;
bool g_gameOver = false;
int g_frameCount = 0;
std::mt19937 g_rng;
static std::function<void(int)> g_gameOverCallback = nullptr;

void InitGame()
{
    g_player.x = 0.0f;
    g_player.y = -0.8f;
    g_player.width = 0.15f;
    g_player.height = 0.15f;
    g_player.speedX = 0.04f;
    g_player.active = true;
    g_rng.seed(std::random_device{}());
    for (int i = 0; i < 35; i++) {
        g_obstacles[i].active = false;
    }
    g_score = 0;
    g_gameOver = false;
    g_frameCount = 0;
    LOGI("Game initialized");
}

void SpawnObstacle()
{
    std::uniform_real_distribution<float> dist(-0.75f, 0.75f);
    for (int i = 0; i < 25; i++) {
        if (!g_obstacles[i].active) {
            g_obstacles[i].x = dist(g_rng);
            g_obstacles[i].y = 1.2f;
            g_obstacles[i].width = 0.12f;
            g_obstacles[i].height = 0.12f;
            g_obstacles[i].speedY = -0.025f - (g_score / 1000.0f * 0.008f);
            g_obstacles[i].active = true;
            break;
        }
    }
}

bool CheckCollision(GameObject &a, GameObject &b)
{
    return (a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y);
}

void UpdateGame()
{
    if (g_gameOver)
        return;
    g_frameCount++;
    bool allInactive = true;
    for (int i = 0; i < 25; i++) {
        if (g_obstacles[i].active) {
            allInactive = false;
            g_obstacles[i].y += g_obstacles[i].speedY;
            if (CheckCollision(g_player, g_obstacles[i])) {
                g_gameOver = true;
                LOGI("COLLISION! GAME OVER! Final Score: %{public}d", g_score);

                if (g_gameOverCallback) {
                    LOGI("Callback exists, calling it now...");
                    g_gameOverCallback(g_score);
                    LOGI("Callback called");
                } else {
                    LOGE("Callback is NULL!");
                }
                return;
            }

            if (g_obstacles[i].y < -1.5f) {
                g_obstacles[i].active = false;
                g_score += 10;
            }
        }
    }
    if (g_frameCount % 25 == 0 || allInactive)
        SpawnObstacle();
    std::uniform_int_distribution<int> randomSpawn(0, 100);
    if (g_frameCount % 10 == 0 && randomSpawn(g_rng) < 30)
        SpawnObstacle();
}

void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    GLfloat vertices[] = {x - w / 2, y - h / 2, x + w / 2, y - h / 2, x - w / 2, y + h / 2, x + w / 2, y + h / 2};
    GLfloat colors[] = {r, g, b, a, r, g, b, a, r, g, b, a, r, g, b, a};
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

struct SyncParam {
    EGLCore *eglCore = nullptr;
    void *window = nullptr;
};

static EGLConfig GetConfig(EGLDisplay eglDisplay)
{
    int attribList[] = {EGL_SURFACE_TYPE,
                        EGL_WINDOW_BIT,
                        EGL_RED_SIZE,
                        8,
                        EGL_GREEN_SIZE,
                        8,
                        EGL_BLUE_SIZE,
                        8,
                        EGL_ALPHA_SIZE,
                        8,
                        EGL_RENDERABLE_TYPE,
                        EGL_OPENGL_ES2_BIT,
                        EGL_NONE};
    EGLConfig configs = NULL;
    int configsNum;
    if (!eglChooseConfig(eglDisplay, attribList, &configs, 1, &configsNum)) {
        LOGE("eglChooseConfig ERROR");
        return NULL;
    }
    return configs;
}

void EGLCore::SetGameOverCallback(std::function<void(int)> callback)
{
    g_gameOverCallback = callback;
    LOGI("Game over callback SET in EGLCore");
}

void EGLCore::OnSurfaceCreated(void *window, int w, int h)
{
    LOGD("EGLCore::OnSurfaceCreated w=%{public}d, h=%{public}d", w, h);
    width_ = w;
    height_ = h;
    InitGame();
    SyncParam *param = new SyncParam();
    param->eglCore = this;
    param->window = window;
    mVsync = OH_NativeVSync_Create(GAME_SYNC_NAME, 3);
    if (!mVsync) {
        LOGE("Create mVsync failed");
        return;
    }
    OH_NativeVSync_RequestFrame(
        mVsync,
        [](long long timestamp, void *data) {
            SyncParam *syncParam = (SyncParam *)data;
            if (!syncParam)
                return;
            EGLCore *eglCore = syncParam->eglCore;
            void *window = syncParam->window;
            if (!eglCore || !window)
                return;
            eglCore->mEglWindow = static_cast<EGLNativeWindowType>(window);
            eglCore->mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (eglCore->mEGLDisplay == EGL_NO_DISPLAY) {
                LOGE("Unable to get EGL display");
                return;
            }
            EGLint eglMajVers, eglMinVers;
            if (!eglInitialize(eglCore->mEGLDisplay, &eglMajVers, &eglMinVers)) {
                LOGE("Unable to initialize display");
                return;
            }
            eglCore->mEGLConfig = GetConfig(eglCore->mEGLDisplay);
            if (!eglCore->mEGLConfig) {
                LOGE("Config ERROR");
                return;
            }
            EGLint winAttribs[] = {EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR, EGL_NONE};
            eglCore->mEGLSurface =
                eglCreateWindowSurface(eglCore->mEGLDisplay, eglCore->mEGLConfig, eglCore->mEglWindow, winAttribs);

            if (!eglCore->mEGLSurface) {
                LOGE("eglSurface is null");
                return;
            }
            int attrib3_list[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
            eglCore->mEGLContext =
                eglCreateContext(eglCore->mEGLDisplay, eglCore->mEGLConfig, eglCore->mSharedEGLContext, attrib3_list);
            if (!eglMakeCurrent(eglCore->mEGLDisplay, eglCore->mEGLSurface, eglCore->mEGLSurface,
                                eglCore->mEGLContext)) {
                LOGE("eglMakeCurrent error = %{public}d", eglGetError());
                return;
            }
            eglCore->mProgramHandle = eglCore->CreateProgram(g_vertexShader, g_fragmentShader);
            if (!eglCore->mProgramHandle) {
                LOGE("Could not create program");
                return;
            }
            LOGI("EGL initialized successfully, starting game loop");
            eglCore->GameLoop();
        },
        param);
}

void EGLCore::GameLoop()
{
    if (!eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext)) {
        LOGE("GameLoop: eglMakeCurrent error = %{public}d", eglGetError());
        return;
    }
    UpdateGame();
    glViewport(0, 0, width_, height_);
    glClearColor(0.04f, 0.04f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(mProgramHandle);
    if (g_player.active)
        DrawRect(g_player.x, g_player.y, g_player.width, g_player.height, 0.0f, 1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 25; i++) {
        if (g_obstacles[i].active)
            DrawRect(g_obstacles[i].x, g_obstacles[i].y, g_obstacles[i].width, g_obstacles[i].height, 1.0f, 0.2f, 0.2f, 1.0f);
    }
    glFlush();
    glFinish();
    eglSwapBuffers(mEGLDisplay, mEGLSurface);
    if (!g_gameOver) {
        OH_NativeVSync_RequestFrame(
            mVsync, [](long long timestamp, void *data) { (reinterpret_cast<EGLCore *>(data))->GameLoop(); },
            (void *)this);
    } else {
        LOGI("Game loop stopped - Game Over");
    }
}

void EGLCore::MovePlayerLeft()
{
    if (g_gameOver || !g_player.active)
        return;
    g_player.x -= g_player.speedX;
    if (g_player.x - g_player.width / 2 < -1.0f)
        g_player.x = -1.0f + g_player.width / 5;
}

void EGLCore::MovePlayerRight()
{
    if (g_gameOver || !g_player.active)
        return;
    g_player.x += g_player.speedX;
    if (g_player.x + g_player.width / 2 > 1.0f) {
        g_player.x = 1.0f - g_player.width / 5;
    }
}

void EGLCore::RestartGame()
{
    LOGI("Restarting game...");
    InitGame();
    if (mVsync)
        GameLoop();
}

void EGLCore::Update()
{
    eglSwapBuffers(mEGLDisplay, mEGLSurface);
}

GLuint EGLCore::LoadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Shader compile error: %{public}s", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint EGLCore::CreateProgram(const char *vertexShader, const char *fragShader)
{
    GLuint vertex = LoadShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fragment = LoadShader(GL_FRAGMENT_SHADER, fragShader);
    GLuint program = glCreateProgram();
    if (vertex == 0 || fragment == 0 || program == 0)
        return 0;
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = (char *)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("Program link error: %{public}s", infoLog);
            free(infoLog);
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteProgram(program);
        return 0;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

GLuint EGLCore::CreateProgramError(const char *vertexShader, const char *fragShader)
{
    return 0;
}

void EGLCore::OnSurfaceDestroyed()
{
    LOGI("EGLCore::OnSurfaceDestroyed");
    if (mVsync) {
        OH_NativeVSync_Destroy(mVsync);
        mVsync = nullptr;
    }
    if (mEGLContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEGLDisplay, mEGLContext);
        mEGLContext = EGL_NO_CONTEXT;
    }
    if (mEGLSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEGLDisplay, mEGLSurface);
        mEGLSurface = EGL_NO_SURFACE;
    }
}

void EGLCore::OnSurfaceChanged(void *window, int32_t w, int32_t h)
{
    width_ = w;
    height_ = h;
}

void EGLCore::DrawSquare() {}
void EGLCore::SwitchDiffuse() {}
void EGLCore::SwitchAmbient() {}
void EGLCore::SwitchSpecular() {}