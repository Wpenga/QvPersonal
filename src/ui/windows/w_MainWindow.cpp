#include "w_MainWindow.hpp"

#include "GuiPluginHost/GuiPluginHost.hpp"
#include "Qv2rayApplication.hpp"
#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Profile/KernelManager.hpp"
#include "Qv2rayBase/Profile/ProfileManager.hpp"
#include "components/UpdateChecker/UpdateChecker.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui/widgets/ConnectionInfoWidget.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"
#include "ui/windows/editors/w_OutboundEditor.hpp"

#ifdef QV2RAY_COMPONENT_RouteEditor
#include "ui/windows/editors/w_RoutesEditor.hpp"
#endif

#include "ui/windows/w_AboutWindow.hpp"
#include "ui/windows/w_GroupManager.hpp"
#include "ui/windows/w_ImportConfig.hpp"
#include "ui/windows/w_PluginManager.hpp"
#include "ui/windows/w_PreferencesWindow.hpp"

#include <QClipboard>
#include <QInputDialog>
#include <QScrollBar>
#include <QToolTip>

#define QV_MODULE_NAME "MainWindow"

#define CheckCurrentWidget                                                                                                                                               \
    auto widget = GetIndexWidget(connectionTreeView->currentIndex());                                                                                                    \
    if (widget == nullptr)                                                                                                                                               \
        return;

#define GetIndexWidget(item) (qobject_cast<ConnectionItemWidget *>(connectionTreeView->indexWidget(item)))
#define NumericString(i) (QString("%1").arg(i, 30, 10, QLatin1Char('0')))

constexpr auto BUTTON_PROP_PLUGIN_MAINWIDGETITEM_INDEX = "plugin_list_index";

QvMessageBusSlotImpl(MainWindow)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
        case MessageBus::RETRANSLATE:
        {
            retranslateUi(this);
            updateActionTranslations();
            break;
        }
    }
}

void MainWindow::SortConnectionList(ConnectionInfoRole byCol, bool asending)
{
    modelHelper->Sort(byCol, asending ? Qt::AscendingOrder : Qt::DescendingOrder);
    on_locateBtn_clicked();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    constexpr auto sizeRatioA = 0.382;
    constexpr auto sizeRatioB = 1 - sizeRatioA;
    splitter->setSizes({ (int) (width() * sizeRatioA), (int) (width() * sizeRatioB) });

    QvMessageBusConnect();

    infoWidget = new ConnectionInfoWidget(this);
    connectionInfoLayout->addWidget(infoWidget);
    vCoreLogHighlighter = new LogHighlighter::LogHighlighter(masterLogBrowser->document());

    // For charts
    speedChartWidget = new SpeedWidget(this);
    speedChart->addWidget(speedChartWidget);

    modelHelper = new ConnectionListHelper(connectionTreeView);

    this->setWindowIcon(QvApp->Qv2rayLogo);
    updateColorScheme();
    updateActionTranslations();
    //
    //
    connect(QvKernelManager, &Qv2rayBase::Profile::KernelManager::OnCrashed,
            [this](const ProfileId &, const QString &reason)
            {
                MWShowWindow();
                QvBaselib->Warn(tr("Kernel terminated."), tr("The kernel terminated unexpectedly:") + NEWLINE + reason + NEWLINE + NEWLINE +
                                                              tr("To solve the problem, read the kernel log in the log text browser."));
            });

    connect(QvKernelManager, &Qv2rayBase::Profile::KernelManager::OnConnected, this, &MainWindow::OnConnected);
    connect(QvKernelManager, &Qv2rayBase::Profile::KernelManager::OnDisconnected, this, &MainWindow::OnDisconnected);
    connect(QvKernelManager, &Qv2rayBase::Profile::KernelManager::OnStatsDataAvailable, this, &MainWindow::OnStatsAvailable);
    connect(QvKernelManager, &Qv2rayBase::Profile::KernelManager::OnKernelLogAvailable, this, &MainWindow::OnKernelLogAvailable);
    connect(QvProfileManager, &Qv2rayBase::Profile::ProfileManager::OnSubscriptionUpdateFinished,
            [](const GroupId &gid) { QvApp->GetTrayManager()->ShowTrayMessage(tr("Subscription \"%1\" has been updated").arg(GetDisplayName(gid))); });

    connect(infoWidget, &ConnectionInfoWidget::OnTagSearchRequested, this,
            [this](QString tag)
            {
                if (tag.isEmpty())
                    return;

                if (tag[0].isDigit() || tag.contains(' ') || tag.contains('"'))
                {
                    tag.replace('"', R"(\")");
                    tag = '"' + tag + '"';
                }
                connectionFilterTxt->setText("> tags=" + tag);
                on_connectionFilterTxt_textEdited("> tags=" + tag);
            });
    connect(infoWidget, &ConnectionInfoWidget::OnEditRequested, this, &MainWindow::OnEditRequested);
    connect(infoWidget, &ConnectionInfoWidget::OnJsonEditRequested, this, &MainWindow::OnEditJsonRequested);

    connect(masterLogBrowser->verticalScrollBar(), &QSlider::valueChanged, this, &MainWindow::OnLogScrollbarValueChanged);
    //
    // Actions for right click the log text browser
    //
    logRCM_Menu->addAction(action_RCM_CopySelected);
    logRCM_Menu->addAction(action_RCM_CopyRecentLogs);
    connect(masterLogBrowser, &QTextBrowser::customContextMenuRequested, [this](QPoint) { logRCM_Menu->popup(QCursor::pos()); });
    connect(action_RCM_CopyRecentLogs, &QAction::triggered, this, &MainWindow::Action_CopyRecentLogs);
    connect(action_RCM_CopySelected, &QAction::triggered, masterLogBrowser, &QTextBrowser::copy);
    //
    speedChartWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(speedChartWidget, &QWidget::customContextMenuRequested, [this](QPoint) { graphWidgetMenu->popup(QCursor::pos()); });
    //
    masterLogBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    {
        auto font = masterLogBrowser->font();
        font.setPointSize(9);
        masterLogBrowser->document()->setDefaultFont(font);
    }
    //
    // Globally invokable signals.
    //
    connect(this, &MainWindow::StartConnection, QvProfileManager, &Qv2rayBase::Profile::ProfileManager::RestartConnection);
    connect(this, &MainWindow::StopConnection, QvProfileManager, &Qv2rayBase::Profile::ProfileManager::StopConnection);
    connect(this, &MainWindow::RestartConnection, QvProfileManager, &Qv2rayBase::Profile::ProfileManager::RestartConnection);
    //
    // Actions for right click the connection list
    //
    connectionListRCM_Menu->addAction(action_RCM_Start);
    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_Edit);
    connectionListRCM_Menu->addAction(action_RCM_EditJson);
    connectionListRCM_Menu->addAction(action_RCM_EditComplex);
    connectionListRCM_Menu->addSeparator();

    connectionListRCM_Menu->addAction(action_RCM_TestLatency);

    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_SetAutoConnection);
    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_RenameConnection);
    connectionListRCM_Menu->addAction(action_RCM_DuplicateConnection);
    connectionListRCM_Menu->addAction(action_RCM_ResetStats);
    connectionListRCM_Menu->addAction(action_RCM_UpdateSubscription);
    connectionListRCM_Menu->addSeparator();

    connectionListRCM_Menu->addAction(action_RCM_DeleteConnection);

    connect(action_RCM_Start, &QAction::triggered, this, &MainWindow::Action_Start);
    connect(action_RCM_SetAutoConnection, &QAction::triggered, this, &MainWindow::Action_SetAutoConnection);
    connect(action_RCM_Edit, &QAction::triggered, this, &MainWindow::Action_Edit);
    connect(action_RCM_EditJson, &QAction::triggered, this, &MainWindow::Action_EditJson);
    connect(action_RCM_EditComplex, &QAction::triggered, this, &MainWindow::Action_EditComplex);
    connect(action_RCM_TestLatency, &QAction::triggered, this, &MainWindow::Action_TestLatency);
    connect(action_RCM_RenameConnection, &QAction::triggered, this, &MainWindow::Action_RenameConnection);
    connect(action_RCM_DuplicateConnection, &QAction::triggered, this, &MainWindow::Action_DuplicateConnection);
    connect(action_RCM_ResetStats, &QAction::triggered, this, &MainWindow::Action_ResetStats);
    connect(action_RCM_UpdateSubscription, &QAction::triggered, this, &MainWindow::Action_UpdateSubscription);
    connect(action_RCM_DeleteConnection, &QAction::triggered, this, &MainWindow::Action_DeleteConnections);
    //
    // Sort Menu
    //
    sortMenu->addAction(sortAction_SortByName_Asc);
    sortMenu->addAction(sortAction_SortByName_Dsc);
    sortMenu->addSeparator();
    sortMenu->addAction(sortAction_SortByData_Asc);
    sortMenu->addAction(sortAction_SortByData_Dsc);
    sortMenu->addSeparator();
    sortMenu->addAction(sortAction_SortByPing_Asc);
    sortMenu->addAction(sortAction_SortByPing_Dsc);
    //
    connect(sortAction_SortByName_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_DISPLAYNAME, true); });
    connect(sortAction_SortByName_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_DISPLAYNAME, false); });
    connect(sortAction_SortByData_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_DATA_USAGE, true); });
    connect(sortAction_SortByData_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_DATA_USAGE, false); });
    connect(sortAction_SortByPing_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_LATENCY, true); });
    connect(sortAction_SortByPing_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_LATENCY, false); });
    //
    sortBtn->setMenu(sortMenu);
    //
    graphWidgetMenu->addAction(action_RCM_CopyGraph);
    connect(action_RCM_CopyGraph, &QAction::triggered, this, &MainWindow::Action_CopyGraphAsImage);
    //
    // Find and start if there is an auto-connection
    const auto connectionStarted = StartAutoConnectionEntry();

    if (!connectionStarted && !QvProfileManager->GetConnections().isEmpty())
    {
        // Select the first connection.
        const auto groups = QvProfileManager->GetGroups();
        if (!groups.isEmpty())
        {
            const auto connections = QvProfileManager->GetConnections(groups.first());
            if (!connections.empty())
            {
                const auto index = modelHelper->GetConnectionPairIndex({ connections.first(), groups.first() });
                connectionTreeView->setCurrentIndex(index);
                on_connectionTreeView_clicked(index);
            }
        }
    }

    CheckForSubscriptionsUpdate();

    for (const auto &[pluginInterface, guiInterface] : GUIPluginHost->GUI_QueryByComponent(Qv2rayPlugin::GUI_COMPONENT_MAIN_WINDOW_ACTIONS))
    {
        auto mainWindowWidgetPtr = guiInterface->GetMainWindowWidget();
        if (!mainWindowWidgetPtr)
            continue;
        const auto index = pluginWidgets.count();
        {
            // Let Qt manage the ownership.
            auto widget = mainWindowWidgetPtr.release();
            pluginWidgets.append(widget);
        }
        auto btn = new QPushButton(pluginInterface->GetMetadata().Name, this);
        connect(btn, &QPushButton::clicked, this, &MainWindow::OnPluginButtonClicked);
        btn->setProperty(BUTTON_PROP_PLUGIN_MAINWIDGETITEM_INDEX, index);
        topButtonsLayout->addWidget(btn);
    }

    if (connectionStarted || GlobalConfig->behaviorConfig->QuietMode)
        MWHideWindow();
    else
        MWShowWindow();

    UpdateChecker::CheckUpdate();
}

void MainWindow::OnPluginButtonClicked()
{
    const auto senderWidget = qobject_cast<QPushButton *>(sender());
    if (!senderWidget)
        return;

    bool ok = false;
    const auto index = senderWidget->property(BUTTON_PROP_PLUGIN_MAINWIDGETITEM_INDEX).toInt(&ok);
    if (!ok)
        return;

    const auto widget = pluginWidgets.at(index);
    if (!widget)
        return;

    widget->setVisible(!widget->isVisible());
}

void MainWindow::ProcessCommand(const QString &command, QStringList commands, QMap<QString, QString> args)
{
    if (commands.isEmpty())
        return;
    if (command == u"open"_qs)
    {
        const auto subcommand = commands.takeFirst();
        QvDialog *w;
        if (subcommand == u"preference"_qs)
            w = new PreferencesWindow();
        else if (subcommand == u"plugin"_qs)
            w = new PluginManageWindow();
        else if (subcommand == u"group"_qs)
            w = new GroupManager();
        else if (subcommand == u"import"_qs)
            w = new ImportConfigWindow();
        else
            return;
        w->processCommands(command, commands, args);
        w->exec();
        delete w;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (focusWidget() == connectionTreeView)
    {
        CheckCurrentWidget;
        if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
        {
            // If pressed enter or return on connectionListWidget. Try to connect to the selected connection.
            if (widget->IsConnection())
            {
                widget->BeginConnection();
            }
            else
            {
                connectionTreeView->expand(connectionTreeView->currentIndex());
            }
        }
        else if (e->key() == Qt::Key_F2)
        {
            widget->BeginRename();
        }
        else if (e->key() == Qt::Key_Delete)
        {
            Action_DeleteConnections();
        }
    }

    if (e->key() == Qt::Key_Escape)
    {
        auto widget = GetIndexWidget(connectionTreeView->currentIndex());
        // Check if this key was accpted by the ConnectionItemWidget
        if (widget && widget->IsRenaming())
        {
            widget->CancelRename();
            return;
        }
        else if (this->isActiveWindow())
        {
            this->close();
        }
    }
    // Ctrl + Q = Exit
    else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_Q)
    {
        if (QvBaselib->Ask(tr("Quit Qv2ray"), tr("Are you sure to exit Qv2ray?")) == Qv2rayBase::MessageOpt::Yes)
            Action_Exit();
    }
    // Control + W = Close Window
    else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_W)
    {
        if (this->isActiveWindow())
            this->close();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    // Workaround of QtWidget not grabbing KeyDown and KeyUp in keyPressEvent
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
    {
        if (focusWidget() == connectionTreeView)
        {
            CheckCurrentWidget;
            on_connectionTreeView_clicked(connectionTreeView->currentIndex());
        }
    }
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
        QvApp->GetTrayManager()->SetMainWindowCurrentState((isHidden() || isMinimized()) ? MainWindowState::State_Hidden : MainWindowState::State_Shown);
}

void MainWindow::Action_Start()
{
    CheckCurrentWidget;
    if (widget->IsConnection())
        widget->BeginConnection();
}

MainWindow::~MainWindow()
{
    delete modelHelper;
    for (auto &widget : pluginWidgets)
        widget->accept();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (GlobalConfig->appearanceConfig->ShowTrayIcon)
    {
        MWHideWindow();
        event->ignore();
    }
    else if (QvBaselib->Ask(tr("Quit Qv2ray"), tr("Are you sure to exit Qv2ray?")) == Qv2rayBase::MessageOpt::Yes)
        Action_Exit();
    else
        event->ignore();
}

void MainWindow::OnTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
#ifdef Q_OS_MACOS
    // special arrangement needed for macOS
    const auto toggleTriggerEvent = QSystemTrayIcon::DoubleClick;
#else
    const auto toggleTriggerEvent = QSystemTrayIcon::Trigger;
#endif
    if (reason == toggleTriggerEvent)
        MWToggleVisibility();
}

void MainWindow::Action_Exit()
{
    QvApp->quit();
}

void MainWindow::on_clearlogButton_clicked()
{
    masterLogBrowser->document()->clear();
}

void MainWindow::on_connectionTreeView_customContextMenuRequested(QPoint pos)
{
    Q_UNUSED(pos)

    auto _pos = QCursor::pos();
    const auto item = connectionTreeView->indexAt(connectionTreeView->mapFromGlobal(_pos));
    if (item.isValid())
    {
        bool isConnection = GetIndexWidget(item)->IsConnection();
        // Disable connection-specific settings.
        action_RCM_Start->setEnabled(isConnection);
        action_RCM_SetAutoConnection->setEnabled(isConnection);
        action_RCM_Edit->setEnabled(isConnection);
        action_RCM_EditJson->setEnabled(isConnection);
        action_RCM_EditComplex->setEnabled(isConnection);
        action_RCM_RenameConnection->setEnabled(isConnection);
        action_RCM_DuplicateConnection->setEnabled(isConnection);
        action_RCM_UpdateSubscription->setEnabled(!isConnection);
        connectionListRCM_Menu->popup(_pos);
    }
}

void MainWindow::Action_DeleteConnections()
{
    QList<ProfileId> connlist;
    QList<GroupId> groupsList;

    for (const auto &item : connectionTreeView->selectionModel()->selectedIndexes())
    {
        const auto widget = GetIndexWidget(item);
        if (!widget)
            continue;

        const auto identifier = widget->Identifier();
        if (widget->IsConnection())
        {
            // Simply add the connection id
            connlist.append(identifier);
            continue;
        }

        for (const auto &conns : QvProfileManager->GetConnections(identifier.groupId))
            connlist.append(ProfileId{ conns, identifier.groupId });

        const auto message = tr("Do you want to remove groups as well?") + NEWLINE + tr("Group: ") + GetDisplayName(identifier.groupId);
        if (QvBaselib->Ask(tr("Removing Connection(s)"), message) == Qv2rayBase::MessageOpt::Yes)
            groupsList << identifier.groupId;
    }

    const auto strRemoveConnTitle = tr("Removing Connection(s)");
    const auto strRemoveConnContent = tr("Are you sure to remove selected connection(s)?");

    if (QvBaselib->Ask(strRemoveConnTitle, strRemoveConnContent) != Qv2rayBase::MessageOpt::Yes)
        return;

    for (const auto &conn : connlist)
        QvProfileManager->RemoveFromGroup(conn.connectionId, conn.groupId);

    for (const auto &group : groupsList)
        QvProfileManager->DeleteGroup(group, false);
}

void MainWindow::on_importConfigButton_clicked()
{
    ImportConfigWindow w(this);
    const auto &[group, connections] = w.DoImportConnections();
    for (auto it = connections.keyValueBegin(); it != connections.constKeyValueEnd(); it++)
        QvProfileManager->CreateConnection(it->second, it->first, group);
}

void MainWindow::Action_EditComplex()
{
    CheckCurrentWidget;
    if (widget->IsConnection())
    {
        const auto id = widget->Identifier();
        ProfileContent root = QvProfileManager->GetConnection(id.connectionId);

#ifdef QV2RAY_COMPONENT_RouteEditor
        QvLog() << "Opening route editor.";
        RouteEditor editor(root, this);
        root = editor.OpenEditor();
        QvProfileManager->UpdateConnection(id.connectionId, root);
#else
        JsonEditor editor(root.toJson(), this);
        root = ProfileContent::fromJson(editor.OpenEditor());
#endif
        if (editor.result() == QDialog::Accepted)
            QvProfileManager->UpdateConnection(id.connectionId, root);
    }
}

void MainWindow::on_subsButton_clicked()
{
    GroupManager().exec();
}

void MainWindow::OnDisconnected(const ProfileId &id)
{
    Q_UNUSED(id)
    locateBtn->setEnabled(false);
    netspeedLabel->setText("0.00 B/s" NEWLINE "0.00 B/s");
    dataamountLabel->setText("0.00 B" NEWLINE "0.00 B");
    connetionStatusLabel->setText(tr("Not Connected"));
}

void MainWindow::OnConnected(const ProfileId &id)
{
    Q_UNUSED(id)
    locateBtn->setEnabled(true);
    on_clearlogButton_clicked();
    auto name = GetDisplayName(id.connectionId);
    if (!GlobalConfig->behaviorConfig->QuietMode)
    {
        QvApp->GetTrayManager()->ShowTrayMessage(tr("Connected: ") + name);
    }
    QvApp->GetTrayManager()->SetTrayTooltip("Qv2ray " QV2RAY_VERSION_STRING NEWLINE + tr("Connected: ") + name);
    connetionStatusLabel->setText(tr("Connected: ") + name);
    //
    GlobalConfig->appearanceConfig->RecentConnections->removeAll(id);
    GlobalConfig->appearanceConfig->RecentConnections->push_front(id);
}

void MainWindow::on_connectionFilterTxt_textEdited(const QString &arg1)
{
    if (arg1.startsWith(">"))
    {
        try
        {
            auto command = arg1;
            command = command.remove(0, 1);
            const auto prog = QueryParser::ParseProgram(command);
            modelHelper->Filter(prog);
            BLACK(connectionFilterTxt);
        }
        catch (std::runtime_error e)
        {
            RED(connectionFilterTxt);
            QToolTip::showText(connectionFilterTxt->mapToGlobal(connectionFilterTxt->pos()), e.what());
        }
    }
    else
    {
        modelHelper->Filter(arg1);
    }
}

void MainWindow::OnStatsAvailable(const ProfileId &id, const StatisticsObject &data)
{
    if (!QvProfileManager->IsConnected(id))
        return;

    // This may not be, or may not precisely be, speed per second if the backend
    // has "any" latency. (Hope not...)
    QMap<SpeedWidget::GraphType, long> pointData;
    pointData[SpeedWidget::OUTBOUND_PROXY_UP] = data.proxyUp;
    pointData[SpeedWidget::OUTBOUND_PROXY_DOWN] = data.proxyDown;
    pointData[SpeedWidget::OUTBOUND_DIRECT_UP] = data.directUp;
    pointData[SpeedWidget::OUTBOUND_DIRECT_DOWN] = data.directDown;

    speedChartWidget->AddPointData(pointData);

    const auto &[totalUp, totalDown] = GetConnectionUsageAmount(id.connectionId, StatisticsObject::PROXY);
    auto totalDataUp = FormatBytes(totalUp);
    auto totalDataDown = FormatBytes(totalDown);
    auto totalSpeedUp = FormatBytes(data.proxyUp) + "/s";
    auto totalSpeedDown = FormatBytes(data.proxyDown) + "/s";

    netspeedLabel->setText(totalSpeedUp + NEWLINE + totalSpeedDown);
    dataamountLabel->setText(totalDataUp + NEWLINE + totalDataDown);

    const static auto format = u"Qv2ray %1\nConnected: %2\nUp: %3 Down: %4"_qs;
    QvApp->GetTrayManager()->SetTrayTooltip(format.arg(QV2RAY_VERSION_STRING, GetDisplayName(id.connectionId), totalSpeedUp, totalSpeedDown));
}

void MainWindow::OnKernelLogAvailable(const ProfileId &id, const QString &log)
{
    Q_UNUSED(id);
    FastAppendTextDocument(log.trimmed(), masterLogBrowser->document());

    // From https://gist.github.com/jemyzhang/7130092
    const auto maxLines = GlobalConfig->appearanceConfig->MaximizeLogLines;
    auto block = masterLogBrowser->document()->begin();

    while (block.isValid())
    {
        if (masterLogBrowser->document()->blockCount() > maxLines)
        {
            QTextCursor cursor(block);
            block = block.next();
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            continue;
        }

        break;
    }
}

void MainWindow::OnEditRequested(const ConnectionId &id)
{
    const auto original = QvProfileManager->GetConnection(id);
    if (IsComplexConfig(id))
    {
#ifdef QV2RAY_COMPONENT_RouteEditor
        QvLog() << "INFO: Opening route editor.";
        RouteEditor editor(original, this);
        ProfileContent root = editor.OpenEditor();
#else
        JsonEditor editor(original.toJson(), this);
        const auto root = ProfileContent::fromJson(editor.OpenEditor());
#endif
        if (editor.result() == QDialog::Accepted)
            QvProfileManager->UpdateConnection(id, root);
    }
    else
    {
        QvLog() << "INFO: Opening single connection edit window.";
        OutboundObject out;
        if (!original.outbounds.isEmpty())
            out = original.outbounds.constFirst();
        OutboundEditor editor(out, this);
        ProfileContent root{ editor.OpenEditor() };
        if (editor.result() == QDialog::Accepted)
            QvProfileManager->UpdateConnection(id, root);
    }
}
void MainWindow::OnEditJsonRequested(const ConnectionId &id)
{
    JsonEditor w(QvProfileManager->GetConnection(id).toJson(), this);
    auto root = w.OpenEditor();

    ProfileContent newRoot;
    newRoot.loadJson(root);

    if (w.result() == QDialog::Accepted)
    {
        QvProfileManager->UpdateConnection(id, newRoot);
    }
}

void MainWindow::OnLogScrollbarValueChanged(int value)
{
    if (masterLogBrowser->verticalScrollBar()->maximum() == value)
        qvLogAutoScoll = true;
    else
        qvLogAutoScoll = false;
}

void MainWindow::on_locateBtn_clicked()
{
    auto id = QvKernelManager->CurrentConnection();
    if (!id.isNull())
    {
        const auto index = modelHelper->GetConnectionPairIndex(id);
        connectionTreeView->setCurrentIndex(index);
        connectionTreeView->scrollTo(index);
        on_connectionTreeView_clicked(index);
    }
}

void MainWindow::Action_RenameConnection()
{
    CheckCurrentWidget;
    widget->BeginRename();
}

void MainWindow::Action_DuplicateConnection()
{
    QList<ProfileId> connlist;
    connlist.reserve(connectionTreeView->selectionModel()->selectedIndexes().size());
    for (const auto &item : connectionTreeView->selectionModel()->selectedIndexes())
    {
        auto widget = GetIndexWidget(item);
        if (widget->IsConnection())
            connlist.append(widget->Identifier());
    }

    QvLog() << "Selected" << connlist.count() << "items.";

    const auto strDupConnTitle = tr("Duplicating Connection(s)", "", connlist.count());
    const auto strDupConnContent = tr("Are you sure to duplicate these connection(s)?", "", connlist.count());

    if (connlist.count() > 1 && QvBaselib->Ask(strDupConnTitle, strDupConnContent) != Qv2rayBase::MessageOpt::Yes)
        return;

    for (const auto &conn : connlist)
    {
        const auto profile = QvProfileManager->GetConnection(conn.connectionId);
        QvProfileManager->CreateConnection(profile, GetDisplayName(conn.connectionId) + tr(" (Copy)"), conn.groupId);
    }
}

void MainWindow::Action_Edit()
{
    CheckCurrentWidget;
    OnEditRequested(widget->Identifier().connectionId);
}

void MainWindow::Action_EditJson()
{
    CheckCurrentWidget;
    OnEditJsonRequested(widget->Identifier().connectionId);
}

void MainWindow::on_chartVisibilityBtn_clicked()
{
    speedChartHolderWidget->setVisible(!speedChartWidget->isVisible());
}

void MainWindow::on_logVisibilityBtn_clicked()
{
    masterLogBrowser->setVisible(!masterLogBrowser->isVisible());
}

void MainWindow::on_clearChartBtn_clicked()
{
    speedChartWidget->Clear();
}

void MainWindow::on_masterLogBrowser_textChanged()
{
    if (!qvLogAutoScoll)
        return;
    auto bar = masterLogBrowser->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::Action_SetAutoConnection()
{
    const auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        const auto widget = GetIndexWidget(current);
        const auto identifier = widget->Identifier();
        GlobalConfig->behaviorConfig->AutoConnectProfileId = identifier;
        GlobalConfig->behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_FIXED;
        if (!GlobalConfig->behaviorConfig->QuietMode)
        {
            QvApp->GetTrayManager()->ShowTrayMessage(tr("%1 has been set to be auto connected.").arg(GetDisplayName(identifier.connectionId)));
        }
        QvApp->SaveQv2raySettings();
    }
}

void MainWindow::Action_ResetStats()
{
    auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        auto widget = GetIndexWidget(current);
        if (widget)
        {
            if (widget->IsConnection())
                QvProfileManager->ClearConnectionUsage(widget->Identifier());
            else
                QvProfileManager->ClearGroupUsage(widget->Identifier().groupId);
        }
    }
}

void MainWindow::Action_UpdateSubscription()
{
    auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        auto widget = GetIndexWidget(current);
        if (widget)
        {
            if (widget->IsConnection())
                return;
            const auto gid = widget->Identifier().groupId;
            if (QvProfileManager->GetGroupObject(gid).subscription_config.isSubscription)
                QvProfileManager->UpdateSubscription(gid, true);
            else
                QvBaselib->Info(tr("Update Subscription"), tr("Selected group is not a subscription"));
        }
    }
}

void MainWindow::Action_TestLatency()
{
    for (const auto &current : connectionTreeView->selectionModel()->selectedIndexes())
    {
        if (!current.isValid())
            continue;
        const auto widget = GetIndexWidget(current);
        if (!widget)
            continue;
        if (widget->IsConnection())
            QvProfileManager->StartLatencyTest(widget->Identifier().connectionId, GlobalConfig->behaviorConfig->DefaultLatencyTestEngine);
        else
            QvProfileManager->StartLatencyTest(widget->Identifier().groupId, GlobalConfig->behaviorConfig->DefaultLatencyTestEngine);
    }
}

void MainWindow::Action_CopyGraphAsImage()
{
    const auto image = speedChartWidget->grab();
    qApp->clipboard()->setImage(image.toImage());
}

void MainWindow::on_pluginsBtn_clicked()
{
    PluginManageWindow(this).exec();
}

void MainWindow::on_newConnectionBtn_clicked()
{
    OutboundEditor w(this);
    const ProfileContent root{ w.OpenEditor() };
    bool isChanged = w.result() == QDialog::Accepted;
    if (isChanged)
    {
        const auto alias = w.GetFriendlyName();
        const auto item = connectionTreeView->currentIndex();
        const auto id = item.isValid() ? GetIndexWidget(item)->Identifier().groupId : DefaultGroupId;
        QvProfileManager->CreateConnection(root, alias, id);
    }
}

void MainWindow::on_newComplexConnectionBtn_clicked()
{
#ifdef QV2RAY_COMPONENT_RouteEditor
    RouteEditor w({}, this);
    const auto root = w.OpenEditor();
    if (w.result() == QDialog::Accepted)
    {
        const auto item = connectionTreeView->currentIndex();
        const auto id = item.isValid() ? GetIndexWidget(item)->Identifier().groupId : DefaultGroupId;
        QvProfileManager->CreateConnection(root, u"New Connection"_qs, id);
    }
#else
    JsonEditor w({}, this);
    const auto root = ProfileContent::fromJson(w.OpenEditor());
    if (w.result() == QDialog::Accepted)
    {
        const auto item = connectionTreeView->currentIndex();
        const auto id = item.isValid() ? GetIndexWidget(item)->Identifier().groupId : DefaultGroupId;
        QvProfileManager->CreateConnection(root, u"New Connection"_qs, id);
    }
#endif
}

void MainWindow::on_collapseGroupsBtn_clicked()
{
    connectionTreeView->collapseAll();
}

void MainWindow::Action_CopyRecentLogs()
{
    const auto lines = SplitLines(masterLogBrowser->document()->toPlainText());
    bool accepted = false;
    const auto line = QInputDialog::getInt(this, tr("Copy latest logs"), tr("Number of lines of logs to copy"), 20, 0, 2500, 1, &accepted);
    if (!accepted)
        return;
    const auto totalLinesCount = lines.count();
    const auto linesToCopy = std::min((int) totalLinesCount, line);
    QStringList result;
    for (auto i = totalLinesCount - linesToCopy; i < totalLinesCount; i++)
    {
        result.append(lines[i]);
    }
    qApp->clipboard()->setText(result.join('\n'));
}

void MainWindow::on_connectionTreeView_doubleClicked(const QModelIndex &index)
{
    const auto widget = GetIndexWidget(index);
    if (widget == nullptr)
        return;
    if (widget->IsConnection())
        widget->BeginConnection();
}

void MainWindow::on_connectionTreeView_clicked(const QModelIndex &index)
{
    const auto widget = GetIndexWidget(index);
    if (widget == nullptr)
        return;
    infoWidget->ShowDetails(widget->Identifier());
}

void MainWindow::on_preferencesBtn_clicked()
{
    PreferencesWindow(this).exec();
}

void MainWindow::on_aboutBtn_clicked()
{
    AboutWindow(this).exec();
}
