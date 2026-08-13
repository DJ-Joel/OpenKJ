#pragma once

// spdlog 1.17.0 bundles a newer version of the fmt text-formatting library
// (v12). That newer version stopped guessing how to print unfamiliar types -
// it now requires an explicit "formatter" to be told how. QString (a Qt
// type) has no formatter of its own, so any logger->debug()/info()/warn()/
// etc. call anywhere in this project that passes a QString argument fails
// to compile against the new spdlog/fmt.
//
// This header defines that missing formatter once. It converts the QString
// to a normal std::string and reuses fmt's built-in std::string formatter.
// Any .cpp file that logs a QString value should #include this header.

#include <QString>
#include <spdlog/fmt/fmt.h>

template<>
struct fmt::formatter<QString> : fmt::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const QString &s, FormatContext &ctx) const
    {
        return fmt::formatter<std::string>::format(s.toStdString(), ctx);
    }
};
