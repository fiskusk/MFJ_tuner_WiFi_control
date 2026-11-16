#include "TimeUtils.h"

#include <time.h>

bool timeValid()
{
    time_t now = time(nullptr);

    return now > 100000;
}

String getFormattedTime()
{
    time_t now = time(nullptr);

    if (!timeValid())
    {
        return "--:--:--";
    }

    struct tm timeinfo;

    localtime_r(&now, &timeinfo);

    char buffer[16];

    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

    return String(buffer);
}

String getFormattedDateTime()
{
    time_t now = time(nullptr);

    if (now < 100000)
    {
        return "--.--.---- --:--:--";
    }

    struct tm timeinfo;

    localtime_r(&now, &timeinfo);

    char buffer[32];

    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);

    return String(buffer);
}
