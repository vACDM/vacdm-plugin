#pragma once

namespace http {

// Informational
constexpr int kContinue = 100;
constexpr int kSwitchingProtocols = 101;

// Success
constexpr int kOk = 200;
constexpr int kCreated = 201;
constexpr int kAccepted = 202;

// Redirection
constexpr int kMovedPermanently = 301;
constexpr int kFound = 302;

// Client errors
constexpr int kBadRequest = 400;
constexpr int kUnauthorized = 401;
constexpr int kForbidden = 403;
constexpr int kNotFound = 404;

// Server errors
constexpr int kInternalServerError = 500;
constexpr int kNotImplemented = 501;
constexpr int kBadGateway = 502;

}  // namespace http