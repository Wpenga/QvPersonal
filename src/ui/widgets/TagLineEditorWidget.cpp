#include "TagLineEditorWidget.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>

const static inline auto ClearStyle = u"#_tagframe { border: 1px solid #00ffc898; }"_qs;
const static inline auto SelectedStyle = u"#_tagframe { border: 1px solid #ffffc898; background: #40cbba00; }"_qs;

using namespace Qv2ray::components;
using namespace std::chrono_literals;

TagEntryWidget::TagEntryWidget(const QString &name, QWidget *parent) : QFrame(parent), label(new QLabel{ name }), text(name)
{
    setStyleSheet(ClearStyle);
    setObjectName(u"_tagframe"_qs);
    setMouseTracking(true);

    label->installEventFilter(this);
    label->setMargin(3);

    setLayout(new QHBoxLayout);
    layout()->addWidget(label);
    layout()->setContentsMargins(0, 0, 0, 0);
}

QString TagEntryWidget::GetTag() const
{
    return text;
}

bool TagEntryWidget::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Enter:
        {
            setStyleSheet(SelectedStyle);
            return true;
        }
        case QEvent::Leave:
        {
            const auto clearFunc = [this]()
            {
                label->setText(text);
                setStyleSheet(ClearStyle);
                update();
            };

            if (isDeleteMode)
                QTimer::singleShot(3s, this, [=]() { clearFunc(), isDeleteMode = false; });
            else
                clearFunc();
            return true;
        }
        case QEvent::MouseButtonRelease:
        {
            if (isDeleteMode)
            {
                emit OnTagDeleted(text);
            }
            else
            {
                label->setPixmap(QIcon::fromTheme(u"delete"_qs).pixmap(label->width(), label->height()));
                isDeleteMode = true;
            }
            return true;
        }
        default: break;
    }
    return QWidget::event(event);
}

TagsLineEdit::TagsLineEdit(QWidget *parent) : QWidget(parent), lineEdit(new QLineEdit), _layout(new FlowLayout)
{
    setLayout(_layout);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->addWidget(lineEdit);
    lineEdit->installEventFilter(this);
}

void TagsLineEdit::SetTags(const QSet<QString> &_tags)
{
    blockSignals(true);
    tagsSet.clear();
    // count() changes while we remove things, so copy here
    const auto count = _layout->count();
    for (auto i = 0; i < count - 1; i++)
    {
        auto item = _layout->takeAt(0);
        delete item->widget();
        delete item;
    }
    qDebug() << _layout->count();
    assert(_layout->count() == 1);
    blockSignals(false);

    tagsSet.reserve(_tags.size());
    for (const auto &str : _tags)
        AddTag(str);
}

QStringList TagsLineEdit::GetTags()
{
    return { tagsSet.begin(), tagsSet.end() };
}

bool TagsLineEdit::eventFilter(QObject *target, QEvent *event)
{
    if (target == lineEdit)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            qDebug() << keyEvent->keyCombination();
            if (keyEvent->key() == Qt::Key_Backspace)
            {
                const auto text = lineEdit->text();

                if (!text.isEmpty() || _layout->count() <= 1)
                    return QWidget::eventFilter(target, event);
                else
                {
                    auto w = qobject_cast<TagEntryWidget *>(_layout->itemAt(_layout->count() - 2)->widget());
                    const auto tag = w->GetTag();
                    tagsSet.remove(tag);
                    lineEdit->setText(tag);
                    emit OnTagsChanged(GetTags());
                    delete w;
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return || //
                     keyEvent->key() == Qt::Key_Comma || keyEvent->key() == Qt::Key_Semicolon)
            {
                const auto text = lineEdit->text().trimmed();
                if (text.isEmpty())
                    return QWidget::eventFilter(target, event);
                AddTag(text);
                lineEdit->clear();
                return true;
            }
        }
    }
    return QWidget::eventFilter(target, event);
}

void TagsLineEdit::DeleteTagSlot(const QString &s)
{
    qDebug() << "Delete" << s;
    tagsSet.remove(s);
    emit OnTagsChanged(GetTags());
    delete sender();
}

void TagsLineEdit::AddTag(const QString &str)
{
    if (tagsSet.contains(str))
        return;
    tagsSet << str;
    emit OnTagsChanged(GetTags());
    auto w = new TagEntryWidget{ str };
    connect(w, &TagEntryWidget::OnTagDeleted, this, &TagsLineEdit::DeleteTagSlot);
    _layout->addWidgetAt(w, _layout->count() - 1);
}
