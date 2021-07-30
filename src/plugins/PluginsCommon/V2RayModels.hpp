#pragma once

#include "QvPlugin/Common/CommonTypes.hpp"
#include "QvPlugin/Utils/BindableProps.hpp"

namespace Qv2ray::Models
{
    struct RouteMatrixConfig
    {
        const static inline auto EXTRA_OPTIONS_ID = QStringLiteral("RouteMatrixConfig");
        struct Detail
        {
            Detail(const QList<QString> &_direct = {}, const QList<QString> &_block = {}, const QList<QString> &_proxy = {})
            {
                direct = _direct;
                block = _block;
                proxy = _proxy;
            }
            Bindable<QList<QString>> direct;
            Bindable<QList<QString>> block;
            Bindable<QList<QString>> proxy;
            QJS_COMPARE(Detail, direct, block, proxy);
            QJS_JSON(P(proxy, block, direct))
        };

        RouteMatrixConfig(const Detail &_domains = {}, const Detail &_ips = {}, const QString &ds = {})
        {
            domainStrategy = ds;
            domains = _domains;
            ips = _ips;
        }
        Bindable<QString> domainStrategy;
        Bindable<QString> domainMatcher{ QStringLiteral("mph") };
        Bindable<Detail> domains;
        Bindable<Detail> ips;
        QJS_COMPARE(RouteMatrixConfig, domainStrategy, domainMatcher, domains, ips);
        QJS_JSON(P(domainStrategy, domainMatcher, domains, ips))
        static auto fromJson(const QJsonObject &o)
        {
            RouteMatrixConfig route;
            route.loadJson(o);
            return route;
        }
    };

    // We don't use any of the inheritance features, just here to ensure the Json level compatibility.
    struct V2RayDNSObject : public BasicDNSObject
    {
        enum QueryStrategy
        {
            UseIP,
            UseIPv4,
            UseIPv6,
        };

        // Same as above.
        struct V2RayDNSServerObject : public BasicDNSServerObject
        {
            Bindable<bool> QV2RAY_DNS_IS_COMPLEX_DNS{ false };
            Bindable<bool> SkipFallback{ false };
            Bindable<QList<QString>> domains;
            Bindable<QList<QString>> expectIPs;
            QJS_COMPARE(V2RayDNSServerObject, QV2RAY_DNS_IS_COMPLEX_DNS, SkipFallback, port, address, domains, expectIPs);
            QJS_JSON(P(QV2RAY_DNS_IS_COMPLEX_DNS, SkipFallback), F(address, port, domains, expectIPs))
        };

        Bindable<QList<V2RayDNSServerObject>> servers;
        Bindable<QString> clientIp;
        Bindable<QString> tag;
        Bindable<bool> disableCache{ false };
        Bindable<bool> disableFallback{ false };
        Bindable<QString> queryStrategy{ QStringLiteral("UseIP") };
        QJS_COMPARE(V2RayDNSObject, servers, clientIp, tag, disableCache, disableFallback, queryStrategy, servers, hosts, extraOptions);
        QJS_JSON(P(clientIp, tag, disableCache, disableFallback, queryStrategy), F(servers, hosts, extraOptions));
        static auto fromJson(const QJsonObject &o)
        {
            V2RayDNSObject dns;
            dns.loadJson(o);
            return dns;
        }
    };

    struct V2RayFakeDNSObject
    {
        struct PoolObject
        {
            Bindable<QString> ipPool;
            Bindable<int> poolSize;
            QJS_COMPARE(PoolObject, ipPool, poolSize)
            QJS_JSON(F(ipPool, poolSize))
        };

        Bindable<QList<PoolObject>> pools;
        QJS_JSON(P(pools))
        QJS_COMPARE(V2RayFakeDNSObject, pools)
        static auto fromJson(const QJsonObject &o)
        {
            V2RayFakeDNSObject dns;
            dns.loadJson(o);
            return dns;
        }
    };

    namespace transfer
    {
        struct HTTPRequestObject
        {
            Bindable<QString> version{ QStringLiteral("1.1") };
            Bindable<QString> method{ QStringLiteral("GET") };
            Bindable<QList<QString>> path{ { QStringLiteral("/") } };
            Bindable<QMap<QString, QStringList>> headers{
                { { QStringLiteral("Host"), { QStringLiteral("www.baidu.com"), QStringLiteral("www.bing.com") } },
                  { QStringLiteral("User-Agent"),
                    { QStringLiteral("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36"),
                      QStringLiteral(
                          "Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/601.1 (KHTML, like Gecko) CriOS/53.0.2785.109 Mobile/14A456 Safari/601.1.46") } },
                  { QStringLiteral("Accept-Encoding"), { QStringLiteral("gzip, deflate") } },
                  { QStringLiteral("Connection"), { QStringLiteral("keep-alive") } },
                  { QStringLiteral("Pragma"), { QStringLiteral("no-cache") } } }
            };
            QJS_COMPARE(HTTPRequestObject, version, method, path, headers)
            QJS_JSON(P(version, method, path, headers))
        };

        struct HTTPResponseObject
        {
            Bindable<QString> version{ QStringLiteral("1.1") };
            Bindable<QString> status{ QStringLiteral("200") };
            Bindable<QString> reason{ QStringLiteral("OK") };
            Bindable<QMap<QString, QStringList>> headers{ { { QStringLiteral("Content-Type"),
                                                              { QStringLiteral("application/octet-stream"), QStringLiteral("video/mpeg") } },
                                                            { QStringLiteral("Transfer-Encoding"), { QStringLiteral("chunked") } },
                                                            { QStringLiteral("Connection"), { QStringLiteral("keep-alive") } },
                                                            { QStringLiteral("Pragma"), { QStringLiteral("no-cache") } } } };
            QJS_COMPARE(HTTPResponseObject, version, reason, status, status)
            QJS_JSON(P(version, reason, status, status))
        };

        struct TCPHeader_Internal
        {
            Bindable<QString> type{ QStringLiteral("none") };
            Bindable<HTTPRequestObject> request;
            Bindable<HTTPResponseObject> response;
            QJS_COMPARE(TCPHeader_Internal, type, request, response)
            QJS_JSON(P(type, request, response))
        };

        struct ObfsHeaderObject
        {
            Bindable<QString> type{ QStringLiteral("none") };
            QJS_COMPARE(ObfsHeaderObject, type);
            QJS_JSON(P(type))
        };

        struct TCPObject
        {
            Bindable<TCPHeader_Internal> header;
            QJS_COMPARE(TCPObject, header);
            QJS_JSON(P(header))
        };

        struct KCPObject
        {
            Bindable<int> mtu{ 1350 };
            Bindable<int> tti{ 50 };
            Bindable<int> uplinkCapacity{ 5 };
            Bindable<int> downlinkCapacity{ 20 };
            Bindable<bool> congestion{ false };
            Bindable<int> readBufferSize{ 2 };
            Bindable<int> writeBufferSize{ 2 };
            Bindable<QString> seed;
            Bindable<ObfsHeaderObject> header;
            QJS_COMPARE(KCPObject, mtu, tti, uplinkCapacity, congestion, readBufferSize, writeBufferSize, seed, header);
            QJS_JSON(P(mtu, tti, uplinkCapacity, congestion, readBufferSize, writeBufferSize, seed, header))
        };

        struct WebSocketObject
        {
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<QMap<QString, QString>> headers;
            Bindable<int> maxEarlyData{ 0 };
            Bindable<bool> useBrowserForwarding{ false };
            Bindable<QString> earlyDataHeaderName;
            QJS_COMPARE(WebSocketObject, path, headers, maxEarlyData, useBrowserForwarding, useBrowserForwarding);
            QJS_JSON(P(path, headers, maxEarlyData, useBrowserForwarding, useBrowserForwarding))
        };

        struct HttpObject
        {
            Bindable<QList<QString>> host;
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<QString> method;
            QJS_COMPARE(HttpObject, host, path, method);
            QJS_JSON(P(host, path, method))
        };

        struct DomainSocketObject
        {
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<bool> abstract{ false };
            Bindable<bool> padding{ false };
            QJS_COMPARE(DomainSocketObject, path, abstract, padding);
            QJS_JSON(P(path, abstract, padding))
        };

        struct QuicObject
        {
            Bindable<QString> security{ QStringLiteral("none") };
            Bindable<QString> key;
            Bindable<ObfsHeaderObject> header;
            QJS_COMPARE(QuicObject, security, key, header);
            QJS_JSON(P(security, key, header))
        };

        struct gRPCObject
        {
            Bindable<QString> serviceName{ QStringLiteral("GunService") };
            QJS_COMPARE(gRPCObject, serviceName);
            QJS_JSON(P(serviceName))
        };

        struct SockoptObject
        {
            Bindable<int> mark{ 255 };
            Bindable<int> tcpKeepAliveInterval{ 0 };
            Bindable<bool> tcpFastOpen{ false };
            Bindable<QString> tproxy{ QStringLiteral("off") };
            QJS_COMPARE(SockoptObject, mark, tcpKeepAliveInterval, tcpFastOpen, tproxy);
            QJS_JSON(P(mark, tcpKeepAliveInterval, tcpFastOpen, tproxy))
        };

        struct CertificateObject
        {
            Bindable<QString> usage{ QStringLiteral("encipherment") };
            Bindable<QString> certificateFile;
            Bindable<QString> keyFile;
            Bindable<QList<QString>> certificate;
            Bindable<QList<QString>> key;
            QJS_COMPARE(CertificateObject, usage, certificateFile, keyFile, certificate, key);
            QJS_JSON(P(usage, certificateFile, keyFile, certificate, key))
        };

        struct TLSObject
        {
            Bindable<QString> serverName;
            Bindable<bool> disableSessionResumption{ true };
            Bindable<bool> disableSystemRoot{ false };
            Bindable<QList<QString>> alpn;
            Bindable<QList<CertificateObject>> certificates;
            QJS_COMPARE(TLSObject, serverName, disableSessionResumption, disableSystemRoot, alpn, certificates);
            QJS_JSON(P(serverName, disableSessionResumption, disableSystemRoot, alpn, certificates))
        };
    } // namespace transfer

    struct StreamSettingsObject
    {
        Bindable<QString> network{ QStringLiteral("tcp") };
        Bindable<QString> security{ QStringLiteral("none") };
        Bindable<transfer::SockoptObject> sockopt;
        Bindable<transfer::TLSObject> tlsSettings;
        Bindable<transfer::TCPObject> tcpSettings;
        Bindable<transfer::HttpObject> httpSettings;
        Bindable<transfer::WebSocketObject> wsSettings;
        Bindable<transfer::KCPObject> kcpSettings;
        Bindable<transfer::DomainSocketObject> dsSettings;
        Bindable<transfer::QuicObject> quicSettings;
        Bindable<transfer::gRPCObject> grpcSettings;
        QJS_COMPARE(StreamSettingsObject, network, security, sockopt, tlsSettings, tcpSettings, kcpSettings, wsSettings, httpSettings, dsSettings, quicSettings,
                    grpcSettings);
        QJS_JSON(P(network, security, sockopt, tlsSettings, tcpSettings, kcpSettings, wsSettings, httpSettings, dsSettings, quicSettings, grpcSettings))
        static auto fromJson(const QJsonObject &o)
        {
            StreamSettingsObject stream;
            stream.loadJson(o);
            return stream;
        };

        operator IOStreamSettings()
        {
            return IOStreamSettings{ this->toJson() };
        }
    };

    struct HTTPSOCKSObject
    {
        Bindable<QString> user;
        Bindable<QString> pass;
        Bindable<int> level{ 0 };
        QJS_JSON(P(user, pass, level))
    };

    // ShadowSocks Server
    struct ShadowSocksClientObject
    {
        Bindable<QString> method{ QStringLiteral("aes-256-gcm") };
        Bindable<QString> password;
        QJS_JSON(P(method, password))
    };

    // VLESS Server
    struct VLESSClientObject
    {
        Bindable<QString> id;
        Bindable<QString> encryption{ QStringLiteral("none") };
        QJS_JSON(P(id, encryption))
    };

    // VMess Server
    struct VMessClientObject
    {
        Bindable<QString> id;
        Bindable<QString> experiments;
        Bindable<QString> security{ QStringLiteral("auto") };
        QJS_JSON(F(id, security, experiments))
    };

    struct TrojanclientObject
    {
        Bindable<QString> password;
        QJS_JSON(F(password))
    };
} // namespace Qv2ray::Models
