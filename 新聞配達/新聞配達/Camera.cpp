#include "Camera.h"

void UpdateCamera()
{
    VECTOR cameraPos = VGet(0.0f, 5.0f, -10.0f);
    VECTOR target = VGet(0.0f, 0.0f, 0.0f);

    SetCameraPositionAndTarget_UpVecY(cameraPos, target);
}