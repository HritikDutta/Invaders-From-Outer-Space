#pragma once

#include "types.h"
#include "input.h"
#include "application/application.h"

extern inline void input_get_state(Application& app);
extern inline void input_state_update(Application& app);

extern inline void input_process_key(Key key, bool pressed);
extern inline void input_process_mouse_button(MouseButton btn, bool pressed);
extern inline void input_process_mouse_wheel(s32 z);