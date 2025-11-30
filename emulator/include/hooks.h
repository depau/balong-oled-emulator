#pragma once

#include <memory>

class Display;

void set_display(std::unique_ptr<Display> &&value);
Display &get_display();
void setup_hooks();
