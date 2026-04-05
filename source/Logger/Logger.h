#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#include "../../include/models.h"

namespace Logger
{
    LogCallback getCallback(std::ostream& stream)
    {
        auto callback = [&stream](const std::string& error) {stream << error << std::endl;};
        return callback;
    }
}

#endif