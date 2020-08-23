/* Copyright (C) StrawberryHacker */

#ifndef BUTTON_H
#define BUTTON_H

#include "types.h"
#include "list.h"

enum button_event {
    BUTTON_PRESSED,
    BUTTON_RELEASED,
    BUTTON_EVENT
};

struct button_callback {
    enum button_event event;
    void (*callback)(void);

    struct list_node node;
};

void button_init(void);

void button_add_handler(struct button_callback* cb);

#endif
