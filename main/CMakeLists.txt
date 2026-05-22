# Copyright 2026 clesiorki
#
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root for full license information.

idf_component_register(
    SRCS
        "main.c"
        "calibration/calibration.c"

        "motor/motor.c"
        "motor/motor_hw.c"
        "motor/motor_motion.c"

        "seringa/seringa.c"
        "seringa/seringa_motion.c"

        "storage/storage.c"

        "web/auth.c"
        "web/routes.c"
        "web/routes_api.c"
        "web/routes_static.c"
        "web/web_server.c"

        "wifi/wifi.c"

    INCLUDE_DIRS
        "."
        "calibration"
        "motor"
        "seringa"
        "storage"
        "web"
        "wifi"
)
