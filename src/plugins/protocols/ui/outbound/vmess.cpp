#include "vmess.hpp"

#include "BuiltinProtocolPlugin.hpp"

VmessOutboundEditor::VmessOutboundEditor(QWidget *parent) : Qv2rayPlugin::Gui::QvPluginEditor(parent)
{
    setupUi(this);
    setProperty("QV2RAY_INTERNAL_HAS_STREAMSETTINGS", true);
    setProperty("QV2RAY_INTERNAL_HAS_FORWARD_PROXY", true);
}

void VmessOutboundEditor::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void VmessOutboundEditor::SetContent(const IOProtocolSettings &content)
{
    this->content = content;
    vmess.loadJson(content);

    vmess.security.ReadWriteBind(securityCombo, "currentText", &QComboBox::currentIndexChanged);
    vmess.id.ReadWriteBind(idLineEdit, "text", &QLineEdit::textEdited);
}
