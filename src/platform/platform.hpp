

#pragma once

#include "helper/parse_json.hpp"

enum class Platform { PC, Android, Console };


NLOHMANN_JSON_SERIALIZE_ENUM(
        Platform,
        {
                {     Platform::PC,      "pc"},
                {Platform::Android, "android"},
                {Platform::Console, "console"}
}
)
