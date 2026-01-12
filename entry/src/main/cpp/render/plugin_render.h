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

#ifndef PLUGIN_RENDER_H
#define PLUGIN_RENDER_H

#include <unordered_map>
#include <string>
#include <napi/native_api.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include "render/egl_core_shader.h"  // Forward declaration yerine tam include

class PluginRender {
public:
    explicit PluginRender(std::string &id);
    ~PluginRender() = default;
    
    static PluginRender* GetInstance(std::string &id);
    static OH_NativeXComponent_Callback* GetNXComponentCallback();
    
    void SetNativeXComponent(OH_NativeXComponent* component);
    void OnSurfaceCreated(OH_NativeXComponent* component, void* window);
    void OnSurfaceChanged(OH_NativeXComponent* component, void* window);
    void OnSurfaceDestroyed(OH_NativeXComponent* component, void* window);
    
    napi_value Export(napi_env env, napi_value exports);
    
    // NAPI functions
    static napi_value NapiMoveLeft(napi_env env, napi_callback_info info);
    static napi_value NapiMoveRight(napi_env env, napi_callback_info info);
    static napi_value NapiRestartGame(napi_env env, napi_callback_info info);
    static napi_value NapiSetGameOverCallback(napi_env env, napi_callback_info info);
    static napi_value NapiSwitchAmbient(napi_env env, napi_callback_info info);
    static napi_value NapiSwitchDiffuse(napi_env env, napi_callback_info info);
    static napi_value NapiSwitchSpecular(napi_env env, napi_callback_info info);

    EGLCore* eglCore_;

private:
    static std::unordered_map<std::string, PluginRender*> instance_;
    static OH_NativeXComponent_Callback callback_;
    std::string id_;
    uint64_t width_;
    uint64_t height_;
};

#endif // PLUGIN_RENDER_H