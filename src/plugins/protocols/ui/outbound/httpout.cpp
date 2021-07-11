#include "httpout.hpp"

HttpOutboundEditor::HttpOutboundEditor(QWidget *parent) : Qv2rayPlugin::Gui::QvPluginEditor(parent)
{
    setupUi(this);
    setProperty("QV2RAY_INTERNAL_HAS_STREAMSETTINGS", true);
}

void HttpOutboundEditor::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}
