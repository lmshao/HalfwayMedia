//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_PROTOCOL_RTSP_COMMON_H
#define HALFWAY_MEDIA_PROTOCOL_RTSP_COMMON_H

#include <map>
#include <string>

/// [[RTSP/1.0 RFC2326](https://datatracker.ietf.org/doc/html/rfc2326)]
/// [[RTSP/2.0 RFC7826](https://datatracker.ietf.org/doc/html/rfc7826)]

// clang-format off
enum StatusCode {
    Continue                           = 100,
    OK                                 = 200,
    Created                            = 201,
    LowOnStorageSpace                  = 250,
    MultipleChoices                    = 300,
    MovedPermanently                   = 301,
    Found                              = 302,
    NotModified                        = 304,
    UseProxy                           = 305,
    BadRequest                         = 400,
    Unauthorized                       = 401,
    PaymentRequired                    = 402,
    Forbidden                          = 403,
    NotFound                           = 404,
    MethodNotAllowed                   = 405,
    NotAcceptable                      = 406,
    ProxyAuthenticationRequired        = 407,
    RequestTimeout                     = 408,
    Gone                               = 410,
    PreconditionFailed                 = 412,
    RequestMessageBodyTooLarge         = 413,
    RequestURITooLong                  = 414,
    UnsupportedMediaType               = 415,
    ParameterNotUnderstood             = 451,
    NotEnoughBandwidth                 = 453,
    SessionNotFound                    = 454,
    MethodNotValidInThisState          = 455,
    HeaderFieldNotValidForResource     = 456,
    InvalidRange                       = 457,
    ParameterIsReadOnly                = 458,
    AggregateOperationNotAllowed       = 459,
    OnlyAggregateOperationAllowed      = 460,
    UnsupportedTransport               = 461,
    DestinationUnreachable             = 462,
    DestinationProhibited              = 463,
    DataTransportNotReadyYet           = 464,
    NotificationReasonUnknown          = 465,
    KeyManagementError                 = 466,
    ConnectionAuthorizationRequired    = 470,
    ConnectionCredentialsNotAccepted   = 471,
    FailureToEstablishSecureConnection = 472,
    InternalServerError                = 500,
    NotImplemented                     = 501,
    BadGateway                         = 502,
    ServiceUnavailable                 = 503,
    GatewayTimeout                     = 504,
    RTSPVersionNotSupported            = 505,
    OptionNotSupported                 = 551,
    ProxyUnavailable                   = 553,
};

extern std::map<StatusCode, std::string> StatusCodeReason;

// RTSP Method Definitions
constexpr const char *OPTIONS       = "OPTIONS";
constexpr const char *DESCRIBE      = "DESCRIBE";
constexpr const char *ANNOUNCE      = "ANNOUNCE";
constexpr const char *SETUP         = "SETUP";
constexpr const char *PLAY          = "PLAY";
constexpr const char *PLAY_NOTIFY   = "NOTIFY";
constexpr const char *PAUSE         = "PAUSE";
constexpr const char *TEARDOWN      = "TEARDOWN";
constexpr const char *GET_PARAMETER = "GET_PARAMETER";
constexpr const char *SET_PARAMETER = "SET_PARAMETER";
constexpr const char *REDIRECT      = "REDIRECT";
constexpr const char *RECORD        = "RECORD";

constexpr const char *RTSP_VERSION = "RTSP/1.0";
constexpr const char *CRLF         = "\r\n";
constexpr const char *SP           = " ";
// clang-format on

#endif // HALFWAY_MEDIA_PROTOCOL_RTSP_COMMON_H