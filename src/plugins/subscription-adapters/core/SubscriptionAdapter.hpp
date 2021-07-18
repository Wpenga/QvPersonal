#pragma once

#include "QvPlugin/PluginInterface.hpp"

using namespace Qv2rayPlugin;

class SIP008Decoder : public SubscriptionProvider
{
  public:
    SubscriptionResult DecodeSubscription(const QByteArray &data) const override;
};

class SimpleBase64Decoder : public SubscriptionProvider
{
  public:
    SubscriptionResult DecodeSubscription(const QByteArray &data) const override;
};

class OOCv1Decoder : public SubscriptionProvider
{
  public:
    SubscriptionResult FetchDecodeSubscription(const SubscriptionProviderOptions &) const override;
};

class BuiltinSubscriptionAdapterInterface : public IPluginSubscriptionInterface
{
  public:
    explicit BuiltinSubscriptionAdapterInterface() = default;

    const static inline Qv2rayPlugin::Common::EditorCreator::EditorInfoList oocv1_options{};

    QList<SubscriptionProviderInfo> GetInfo() const override
    {
        return {
            SubscriptionProviderInfo::CreateDecoder<SIP008Decoder>(SubscriptionProviderId{ "sip008" }, "SIP008"),
            SubscriptionProviderInfo::CreateDecoder<SimpleBase64Decoder>(SubscriptionProviderId{ "simple_base64" }, "Base64 Links"),
            SubscriptionProviderInfo::CreateFetcherDecoder<OOCv1Decoder>(SubscriptionProviderId{ "ooc-v1" }, "Open Online Config v1", oocv1_options),
        };
    }
};
