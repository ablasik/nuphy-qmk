#pragma once
#include <stdint.h>
typedef uint32_t fast_timer_t;
fast_timer_t timer_read_fast(void);
uint32_t timer_elapsed_fast(fast_timer_t at);
