#pragma once

#include <stdbool.h>
#include "esp_camera.h"

bool cameraInit();
void cameraDeinit();
bool cameraIsReady();
bool cameraCaptureRGB565(camera_fb_t **outFb);
void cameraFbReturn(camera_fb_t *fb);
