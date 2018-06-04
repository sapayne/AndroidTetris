// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform buff_t
{
    mat4 model;
    mat4 view;
    mat4 proj;

} buff;

layout (location = 0) in vec4 pos;
layout (location = 1) out vec4 position;

mat4 deadFunc(mat4 _a, mat4 _b)
{

mat4 mAns;

mAns = _a * _b;
for (int i=0; i < 4; i++)
{
    for (int j=0; j < 4; j++)
    {
    mAns[i][j] = _a[i][j] * _b[i][j];
    }
}
return mAns;

}



void main() {
    //mat4 result =deadFunc(buff.model, buff.view);
    gl_Position = buff.model* buff.view* buff.proj*  pos;
    position = vec4(pos.x, pos.y,pos.z,1.0f);
}
