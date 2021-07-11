#include "socksout.hpp"

SocksOutboundEditor::SocksOutboundEditor(QWidget *parent) : Qv2rayPlugin::Gui::QvPluginEditor(parent)
{
    setupUi(this);
    setProperty("QV2RAY_INTERNAL_HAS_STREAMSETTINGS", true);
}

void SocksOutboundEditor::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}
