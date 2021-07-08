#pragma once

#include "QvPlugin/PluginInterface.hpp"

using namespace Qv2rayPlugin;

class SIP008Decoder : public SubscriptionDecoder
{
  public:
    explicit SIP008Decoder() : SubscriptionDecoder(){};
    SubscriptionDecodeResult DecodeData(const QByteArray &data) const override;
};

class OOCv1Decoder : public SubscriptionDecoder
{
  public:
    explicit OOCv1Decoder() : SubscriptionDecoder(){};
    SubscriptionDecodeResult DecodeData(const QByteArray &data) const override;
};

class SimpleBase64Decoder : public SubscriptionDecoder
{
  public:
    explicit SimpleBase64Decoder() : SubscriptionDecoder(){};
    SubscriptionDecodeResult DecodeData(const QByteArray &data) const override;
};

class BuiltinSubscriptionAdapterInterface : public ISubscriptionHandler
{
  public:
    explicit BuiltinSubscriptionAdapterInterface() = default;

    QList<SubscriptionInfoObject> GetInfo() const override
    {
        return {
            { SubscriptionDecoderId{ "sip008" }, "SIP008", []() { return std::make_unique<SIP008Decoder>(); } },
            { SubscriptionDecoderId{ "ooc-v1" }, "Open Online Config v1", []() { return std::make_unique<OOCv1Decoder>(); } },
            { SubscriptionDecoderId{ "simple_base64" }, "Base64 Links", []() { return std::make_unique<SimpleBase64Decoder>(); } },
        };
    }
};
