#pragma once
#include <stdbool.h>
typedef struct {bool (*is_empty)(void);} rf_queue_t;
extern rf_queue_t rf_queue;
