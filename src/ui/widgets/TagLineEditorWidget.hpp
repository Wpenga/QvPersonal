#pragma once

#include "components/FlowLayout/FlowLayout.hpp"

#include <QFrame>
#include <QSet>

class QLabel;
class QLineEdit;

class TagEntryWidget : public QFrame
{
    Q_OBJECT

  public:
    explicit TagEntryWidget(const QString &name, QWidget *parent = nullptr);
    ~TagEntryWidget() = default;

    QString GetTag() const;

  protected:
    virtual bool event(QEvent *event) override;

  signals:
    void OnTagDeleted(const QString &);

  private:
    QLabel *label;
    QString text;
    bool isDeleteMode = false;
};

class TagsLineEdit : public QWidget
{
    Q_OBJECT

  public:
    explicit TagsLineEdit(QWidget *parent = nullptr);
    ~TagsLineEdit() = default;

    void SetTags(const QSet<QString> &);
    QStringList GetTags();

  signals:
    void OnTagsChanged(const QStringList &);

  protected:
    virtual bool eventFilter(QObject *target, QEvent *event) override;

  private slots:
    void DeleteTagSlot(const QString &s);
    void AddTag(const QString &str);

  private:
    QLineEdit *lineEdit;
    Qv2ray::components::FlowLayout *_layout;
    QSet<QString> tagsSet;
};
