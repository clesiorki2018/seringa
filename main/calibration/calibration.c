#include "calibration.h"

static float calibration_factor = 1.0;

float get_calibration(void)
{
    return calibration_factor;
}

void increase_calibration(void)
{
    calibration_factor += 0.1;
}

void decrease_calibration(void)
{
    calibration_factor -= 0.1;
}