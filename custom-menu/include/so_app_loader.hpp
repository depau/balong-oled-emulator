#pragma once
#include "apps/app_api.hpp"

app_descriptor *
load_app_shared_object(void *userptr, app_api_t controller_api, const char *app_path, void **app_userptr);
