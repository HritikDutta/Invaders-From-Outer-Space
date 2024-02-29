#include "sprite.h"

#include "containers/darray.h"
#include "containers/string.h"
#include "graphics/texture.h"
#include "math/common.h"
#include "rect.h"

void animation_start_instance(Animation2D::Instance& instance, f32 time)
{
    instance.current_frame_index = 0;
    instance.start_time = time;
    instance.loop_count = 0;
}

void animation_step_instance(const Animation2D& animation, Animation2D::Instance& instance, f32 time)
{
    const f32 diff = time - instance.start_time;
    const u32 frames_passed = Math::floor(diff / animation.frame_rate);

    switch (animation.loop_type)
    {
        case Animation2D::LoopType::NONE:
        {
            instance.current_frame_index = min(frames_passed, (u32) animation.sprites.size - 1);
            instance.loop_count = (u32) (frames_passed >= animation.sprites.size);
        } break;

        case Animation2D::LoopType::CYCLE:
        {
            instance.current_frame_index = frames_passed % animation.sprites.size;
            instance.loop_count = frames_passed / animation.sprites.size;
        } break;

        case Animation2D::LoopType::PING_PONG:
        {
            f32 frame_index = frames_passed % (2 * animation.sprites.size);

            if (frame_index >= animation.sprites.size)
                frame_index = 2 * animation.sprites.size - frame_index - 1;
            
            instance.current_frame_index = frame_index;

            instance.loop_count = frames_passed / (2 * animation.sprites.size);
        } break;
    }
}

void free(Animation2D& animation)
{
    // Atlas has to be freed separately
    free(animation.sprites);
    free(animation.name);
}