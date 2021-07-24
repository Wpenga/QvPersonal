#include "SubscriptionAdapter.hpp"

#include "BuiltinSubscriptionAdapter.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSslKey>
#include <QUrl>
#include <QUrlQuery>

const inline QStringList SplitLines(const QString &_string)
{
    return _string.split(QRegularExpression(QStringLiteral("[\r\n]")), Qt::SkipEmptyParts);
}

QString SafeBase64Decode(QString string)
{
    QByteArray ba = string.replace(QChar('-'), QChar('+')).replace(QChar('_'), QChar('/')).toUtf8();
    return QByteArray::fromBase64(ba, QByteArray::Base64Option::OmitTrailingEquals);
}

QString SafeBase64Encode(const QString &string)
{
    QString base64 = string.toUtf8().toBase64();
    return base64.replace(QChar('+'), QChar('-')).replace(QChar('/'), QChar('_'));
}

// Simple Base64 Decoder
SubscriptionResult SimpleBase64Decoder::DecodeSubscription(const QByteArray &data) const
{
    auto source = QString::fromUtf8(data).trimmed();
    const auto resultList = source.contains(QStringLiteral("://")) ? source : SafeBase64Decode(source);
    //
    SubscriptionResult result;
    result.SetValue<SR_Links>(SplitLines(resultList));
    return result;
}

// SIP008 Decoder
SubscriptionResult SIP008Decoder::DecodeSubscription(const QByteArray &data) const
{
    const auto root = QJsonDocument::fromJson(data).object();
    //
    // const auto version = root["version"].toString();
    // const auto username = root["username"].toString();
    // const auto user_uuid = root["user_uuid"].toString();
    const auto servers = root[QStringLiteral("servers")].toArray();

    // ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpwYXNzQGhvc3Q6MTIzNA/?plugin=plugin%3Bopt#sssip003

    SubscriptionResult result;
    QStringList links;
    links.reserve(servers.size());
    for (const auto &servVal : servers)
    {
        const auto serverObj = servVal.toObject();
#define GetVal(x) const auto x = serverObj[u## #x##_qs].toString()
        GetVal(server);
        GetVal(password);
        GetVal(method);
        GetVal(plugin);
        GetVal(plugin_opts);
        GetVal(remarks);
        // GetVal(id);
#undef GetVal

        const auto server_port = serverObj[QStringLiteral("server_port")].toInt();
        bool isSIP003 = !plugin.isEmpty();
        const auto userInfo = SafeBase64Encode(method + QStringLiteral(":") + password);
        //
        QUrl link;
        link.setScheme(QStringLiteral("ss"));
        link.setUserInfo(userInfo);
        link.setHost(server);
        link.setPort(server_port);
        link.setFragment(remarks);
        if (isSIP003)
        {
            QUrlQuery q;
            q.addQueryItem(QStringLiteral("plugin"), QUrl::toPercentEncoding(plugin + QStringLiteral(";") + plugin_opts));
            link.setQuery(q);
        }
        links << link.toString(QUrl::FullyEncoded);
    }
    result.SetValue<SR_Links>(links);
    return result;
}

// OOCv1 Decoder
SubscriptionResult OOCProvider::FetchDecodeSubscription(const SubscriptionProviderOptions &options) const
{
#ifdef OOCv1_DETAIL_CONFIGURATION
    const auto url = u"%1/%2/ooc/v%3/%4"_qs.arg(options.value(u"baseUrl"_qs).toString(),               //
                                                options.value(u"secret"_qs).toString(),                //
                                                QString::number(options.value(u"version"_qs).toInt()), //
                                                options.value(u"userId"_qs).toString());
#else
    const auto json = QJsonDocument::fromJson(options.value(u"token"_qs).toString().toUtf8()).object();
    const auto url = u"%1/%2/ooc/v%3/%4"_qs.arg(json.value(u"baseUrl"_qs).toString(),               //
                                                json.value(u"secret"_qs).toString(),                //
                                                QString::number(json.value(u"version"_qs).toInt()), //
                                                json.value(u"userId"_qs).toString());

#endif

    const auto pinnedCertChecker = [](QNetworkReply *reply)
    {
        QSslCertificate cert = reply->sslConfiguration().peerCertificate();
        QString serverHash = QCryptographicHash::hash(cert.publicKey().toDer(), QCryptographicHash::Sha256).toHex();
        qWarning() << "Pinned certificate not checked, not implemented";
        Q_UNUSED(serverHash);
    };

    const auto &[err, errorString, data] = InternalSubscriptionSupportPlugin::NetworkRequestHelper()->Get(url, pinnedCertChecker);
    qDebug() << data;
    if (err != QNetworkReply::NoError)
        InternalSubscriptionSupportPlugin::Log(errorString);

    const auto shadowsocks = QJsonDocument::fromJson(data).object().value(u"shadowsocks"_qs).toArray();
    QStringList unsupportedNodes;

    SubscriptionResult result;
    SubscriptionResult::result_type_t<SR_Tags> tags;
    SubscriptionResult::result_type_t<SR_OutboundObjects> outbounds;
    for (const auto &ss : shadowsocks)
    {
        // id, group, owner omitted.
        const auto name = ss[u"name"_qs].toString();
        if (ss.toObject().contains(u"pluginName"_qs))
        {
            // SIP003 plugins not supported.
            qDebug() << "Unsupported node:" << name;
            unsupportedNodes << name;
            continue;
        }

        tags.insert(name, ss[u"tags"_qs].toVariant().toStringList());

        IOProtocolSettings protocolSettings;
        protocolSettings.insert(u"method"_qs, ss[u"method"_qs].toString());
        protocolSettings.insert(u"password"_qs, ss[u"password"_qs].toString());

        IOConnectionSettings connectionSettings;
        connectionSettings.address = ss[u"address"_qs].toString();
        connectionSettings.port = ss[u"port"_qs].toInt();
        connectionSettings.protocol = u"shadowsocks"_qs;
        connectionSettings.protocolSettings = protocolSettings;

        outbounds.insert(name, { connectionSettings });
    }
    result.SetValue<SR_Tags>(tags);
    result.SetValue<SR_OutboundObjects>(outbounds);
    return result;
}
