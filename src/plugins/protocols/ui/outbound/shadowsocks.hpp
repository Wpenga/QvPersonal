#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_shadowsocks.h"

class ShadowsocksOutboundEditor
    : public Qv2rayPlugin::Gui::PluginProtocolEditor
    , private Ui::shadowsocksOutEditor
{
    Q_OBJECT

  public:
    explicit ShadowsocksOutboundEditor(QWidget *parent = nullptr);

    void Load() override
    {
        shadowsocks.loadJson(settings);
        shadowsocks.method.ReadWriteBind(ss_encryptionMethod, "currentText", &QComboBox::currentTextChanged);
        shadowsocks.password.ReadWriteBind(ss_passwordTxt, "text", &QLineEdit::textEdited);
    }

    void Store() override
    {
        settings = IOProtocolSettings{ shadowsocks.toJson() };
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::ShadowSocksClientObject shadowsocks;
};
