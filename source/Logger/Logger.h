#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#include "../../include/models.h"

namespace Logger
{
    LogCallback getCallback()
    {
        auto callback = [](const std::string& error) {std::cerr << error << std::endl;};
        return callback;
    }
}

#endif