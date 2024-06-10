#pragma once

#include <json/json.h>

#include <string>

namespace vacdm::utils {
class JsonUtil {
   public:
    JsonUtil() = delete;
    JsonUtil(const JsonUtil &) = delete;
    JsonUtil(JsonUtil &&) = delete;
    JsonUtil &operator=(const JsonUtil &) = delete;
    JsonUtil &operator=(JsonUtil &&) = delete;

    static const bool JsonHasKey(const Json::Value &json, const std::string &key) {
        return json.isObject() && json.isMember(key);
    }

    static const bool JsonHasAllKeys(const Json::Value &json, const std::vector<std::string> &keys) {
        if (!json.isObject()) {
            return false;
        }
        for (const auto &key : keys) {
            if (!json.isMember(key)) {
                return false;
            }
        }
        return true;
    }
};
}  // namespace vacdm::utils