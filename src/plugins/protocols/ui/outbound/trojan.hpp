#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "src/plugins/PluginsCommon/V2RayModels.hpp"
#include "ui_trojan.h"

class TrojanOutboundEditor
    : public Qv2rayPlugin::Gui::PluginProtocolEditor
    , private Ui::TrojanOutEditor
{
    Q_OBJECT

  public:
    explicit TrojanOutboundEditor(QWidget *parent = nullptr);

  protected:
    void changeEvent(QEvent *e) override;
    virtual void Load() override
    {
        trojan.loadJson(settings);
    }
    void Store() override
    {
        settings = IOProtocolSettings{ trojan.toJson() };
    }

  private:
    Qv2ray::Models::TrojanclientObject trojan;
};
