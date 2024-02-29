#pragma once

#include "types.h"
#include "logger.h"
#include "platform/platform.h"

// Based on Randy Gaul's talk on coroutines in c: https://youtu.be/MuCpdoIEpgA

#define COROUTINE_NESTING_LIMIT 8
#define COROUTINE_CALL_OFFSET   1024 * 1024
#define COROUTINE_STACK_SIZE    512ui64

struct Coroutine
{
    // Coroutine Data
    u64  line[COROUTINE_NESTING_LIMIT];
    f64  prev_time;
    u64  depth;

    // Coroutine Stack
    u8  stack_frame[COROUTINE_STACK_SIZE];
    u64 stack_ptr;

    // Meta Data
    bool running;
};

#define coroutine_start(co) switch (co.line[co.depth]) { default: co.running = true;
#define coroutine_end(co) case __LINE__: _co_stop: co.running = false; co.line[co.depth] = __LINE__; } _co_end: co.stack_ptr = 0

#define coroutine_stop(co) goto _co_stop
#define coroutine_reset(co) do { co.line[co.depth] = 0; co.stack_ptr = 0; co.running = false; } while (false)

#define coroutine_yield(co) do { co.line[co.depth] = __LINE__; co.running = true; goto _co_end; case __LINE__:; } while (false)
#define coroutine_wait_until(co, cond) while (!(cond)) { coroutine_yield(co); }
#define coroutine_wait_seconds(co, seconds) do { co.prev_time = platform_get_time(); coroutine_wait_until(co, (platform_get_time() - co.prev_time) >= seconds); } while (false)

#define coroutine_call(co, ...) do { gn_assert_with_message(co.depth + 1 < COROUTINE_NESTING_LIMIT, "Exceeded coroutine nesting limit! (max depth: %)", COROUTINE_NESTING_LIMIT); co.line[co.depth] = __LINE__; co.running = false; case __LINE__: co.depth++; __VA_ARGS__; co.depth--; if (co.running) { goto _co_end; } co.line[co.depth] = __LINE__ + COROUTINE_CALL_OFFSET; co.line[co.depth + 1] = 0; case __LINE__ + COROUTINE_CALL_OFFSET:; } while (false)

template <typename T>
inline T& coroutine_stack_variable(Coroutine& co)
{
    gn_assert_with_message(co.stack_ptr + sizeof(T) < COROUTINE_STACK_SIZE, "Coroutine stack overflow! (max stack size: %, free memory: %)", COROUTINE_STACK_SIZE, COROUTINE_STACK_SIZE - co.stack_ptr);

    void* ptr = (void*) &(co.stack_frame[co.stack_ptr]);
    co.stack_ptr += sizeof(T);

    return *(T*) ptr;
}