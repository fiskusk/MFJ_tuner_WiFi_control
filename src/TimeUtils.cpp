#include "TimeUtils.h"

#include <time.h>

String getFormattedTime()
{
    time_t now = time(nullptr);

    if (now < 100000)
    {
        return "--:--:--";
    }

    struct tm timeinfo;

    localtime_r(
        &now,
        &timeinfo
    );

    char buffer[16];

    strftime(
        buffer,
        sizeof(buffer),
        "%H:%M:%S",
        &timeinfo
    );

    return String(buffer);
}
