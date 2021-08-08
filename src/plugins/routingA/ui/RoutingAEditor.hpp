#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_RoutingAEditor.h"

#include <QRegularExpression>
#include <QSyntaxHighlighter>

class LogHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

  public:
    explicit LogHighlighter(QTextDocument *parent = nullptr);

  protected:
    void highlightBlock(const QString &text) override;

  private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;

    QTextCharFormat keywordsFormat;
    QTextCharFormat builtinTagFormat;
    QTextCharFormat geoIPSiteFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat literalFormat;
    QTextCharFormat quotedFormat;
};

class RoutingAEditor
    : public Qv2rayPlugin::Gui::PluginMainWindowWidget
    , private Ui::RoutingAEditor
{
    Q_OBJECT

  public:
    explicit RoutingAEditor(QWidget *parent = nullptr);

  protected:
    void changeEvent(QEvent *e);

  private slots:
    void on_routingAText_textChanged();
    void on_showLogBtn_clicked();
    void on_checkBtn_clicked();
};
