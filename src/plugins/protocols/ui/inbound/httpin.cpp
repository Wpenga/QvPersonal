#include "httpin.hpp"

#include "BuiltinProtocolPlugin.hpp"

HTTPInboundEditor::HTTPInboundEditor(QWidget *parent) : Qv2rayPlugin::Gui::PluginProtocolEditor(parent)
{
    setupUi(this);
    setProperty("QV2RAY_INTERNAL_HAS_STREAMSETTINGS", true);
}

void HTTPInboundEditor::Store()
{
    // Remove useless, misleading 'accounts' array.
    if (settings["accounts"].toArray().count() == 0)
        settings.remove("accounts");
}

void HTTPInboundEditor::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void HTTPInboundEditor::Load()
{
    isLoading = true;
    httpTimeoutSpinBox->setValue(settings[QStringLiteral("timeout")].toInt());
    httpTransparentCB->setChecked(settings[QStringLiteral("allowTransparent")].toBool());
    httpAccountListBox->clear();

    for (const auto &user : settings[QStringLiteral("accounts")].toArray())
    {
        httpAccountListBox->addItem(user.toObject()[QStringLiteral("user")].toString() + ":" + user.toObject()[QStringLiteral("pass")].toString());
    }
    isLoading = false;
}

void HTTPInboundEditor::on_httpTimeoutSpinBox_valueChanged(int arg1)
{
    if (isLoading)
        return;
    settings[QStringLiteral("timeout")] = arg1;
}

void HTTPInboundEditor::on_httpTransparentCB_stateChanged(int arg1)
{
    if (isLoading)
        return;
    settings[QStringLiteral("allowTransparent")] = arg1 == Qt::Checked;
}

void HTTPInboundEditor::on_httpRemoveUserBtn_clicked()
{
    if (isLoading)
        return;
    if (httpAccountListBox->currentRow() < 0)
    {
        InternalProtocolSupportPlugin::ShowMessageBox(tr("Removing a user"), tr("You haven't selected a user yet."));
        return;
    }
    const auto item = httpAccountListBox->currentItem();
    auto list = settings[QStringLiteral("accounts")].toArray();

    for (int i = 0; i < list.count(); i++)
    {
        const auto user = list[i].toObject();
        const auto entry = user[QStringLiteral("user")].toString() + ":" + user[QStringLiteral("pass")].toString();
        if (entry == item->text().trimmed())
        {
            list.removeAt(i);
            settings[QStringLiteral("accounts")] = list;
            httpAccountListBox->takeItem(httpAccountListBox->currentRow());
            return;
        }
    }
}

void HTTPInboundEditor::on_httpAddUserBtn_clicked()
{
    if (isLoading)
        return;
    const auto user = httpAddUserTxt->text();
    const auto pass = httpAddPasswordTxt->text();
    //
    auto list = settings[QStringLiteral("accounts")].toArray();

    for (int i = 0; i < list.count(); i++)
    {
        const auto _user = list[i].toObject();
        if (_user[QStringLiteral("user")].toString() == user)
        {
            InternalProtocolSupportPlugin::ShowMessageBox(tr("Add a user"), tr("This user exists already."));
            return;
        }
    }

    httpAddUserTxt->clear();
    httpAddPasswordTxt->clear();
    list.append(QJsonObject{ { "user", user }, { "pass", pass } });
    httpAccountListBox->addItem(user + ":" + pass);
    settings[QStringLiteral("accounts")] = list;
}
