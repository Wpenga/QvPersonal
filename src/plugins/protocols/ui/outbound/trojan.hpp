#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "src/plugins/PluginsCommon/V2RayModels.hpp"
#include "ui_trojan.h"

class TrojanOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::TrojanOutEditor
{
    Q_OBJECT

  public:
    explicit TrojanOutboundEditor(QWidget *parent = nullptr);

  protected:
    void changeEvent(QEvent *e) override;
    virtual void SetContent(const IOProtocolSettings &j) override
    {
        trojan.loadJson(j);
    }
    virtual const IOProtocolSettings GetContent() const override
    {
        return IOProtocolSettings{ trojan.toJson() };
    }

  private:
    Qv2ray::Models::TrojanclientObject trojan;
};
