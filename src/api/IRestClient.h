#pragma once
#include <map>
#include <string>

namespace interfaces {
struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
};

class IRestClient {
   public:
    virtual ~IRestClient() = default;

    virtual HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers = {}) = 0;

    virtual HttpResponse post(const std::string& url, const std::string& body,
                              const std::map<std::string, std::string>& headers = {}) = 0;

    virtual HttpResponse put(const std::string& url, const std::string& body,
                             const std::map<std::string, std::string>& headers = {}) = 0;

    virtual HttpResponse patch(const std::string& url, const std::string& body,
                               const std::map<std::string, std::string>& headers = {}) = 0;

    virtual HttpResponse del(const std::string& url, const std::map<std::string, std::string>& headers = {}) = 0;
};

}  // namespace interfaces
