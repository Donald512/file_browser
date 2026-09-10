#pragma once

#include "BasicTypes.h"

struct App;

inline void GameLoop( f32 screenW, f32 screenH, f32 mouseX, f32 mouseY, bool isMouseDown, f32 deltaTime, f32 dpi, App& app, bool isMaximized = false);