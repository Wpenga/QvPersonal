#pragma once

#include "QvPlugin/PluginInterface.hpp"

using namespace Qv2rayPlugin;

class SIP008Decoder : public SubscriptionProvider
{
  public:
    explicit SIP008Decoder() : SubscriptionProvider(){};
    SubscriptionResult DecodeSubscription(const QByteArray &data) const override;
    SubscriptionResult FetchDecodeSubscription(const SubscriptionProviderSettings &) const override
    {
        return {};
    }
};

class OOCv1Decoder : public SubscriptionProvider
{
  public:
    explicit OOCv1Decoder() : SubscriptionProvider(){};
    SubscriptionResult DecodeSubscription(const QByteArray &data) const override;
    SubscriptionResult FetchDecodeSubscription(const SubscriptionProviderSettings &) const override
    {
        return {};
    }
};

class SimpleBase64Decoder : public SubscriptionProvider
{
  public:
    explicit SimpleBase64Decoder() : SubscriptionProvider(){};
    SubscriptionResult DecodeSubscription(const QByteArray &data) const override;
    SubscriptionResult FetchDecodeSubscription(const SubscriptionProviderSettings &) const override
    {
        return {};
    }
};

class BuiltinSubscriptionAdapterInterface : public IPluginSubscriptionInterface
{
  public:
    explicit BuiltinSubscriptionAdapterInterface() = default;

    QList<SubscriptionProviderInfo> GetInfo() const override
    {
        return {
            { SubscriptionProviderId{ "sip008" }, "SIP008", []() { return std::make_unique<SIP008Decoder>(); } },
            { SubscriptionProviderId{ "ooc-v1" }, "Open Online Config v1", []() { return std::make_unique<OOCv1Decoder>(); } },
            { SubscriptionProviderId{ "simple_base64" }, "Base64 Links", []() { return std::make_unique<SimpleBase64Decoder>(); } },
        };
    }
};
