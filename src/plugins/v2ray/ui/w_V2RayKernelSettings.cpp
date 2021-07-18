#include "w_V2RayKernelSettings.hpp"

#include "BuiltinV2RayCorePlugin.hpp"
#include "common/CommonHelpers.hpp"

#include <QFileDialog>
#include <QProcessEnvironment>
#include <QStandardPaths>

V2RayKernelSettings::V2RayKernelSettings(QWidget *parent) : Qv2rayPlugin::Gui::PluginSettingsWidget(parent)
{
    setupUi(this);
    settingsObject.APIEnabled.ReadWriteBind(enableAPI, "checked", &QCheckBox::toggled);
    settingsObject.APIPort.ReadWriteBind(statsPortBox, "value", &QSpinBox::valueChanged);
    settingsObject.AssetsPath.ReadWriteBind(vCoreAssetsPathTxt, "text", &QLineEdit::textEdited);
    settingsObject.CorePath.ReadWriteBind(vCorePathTxt, "text", &QLineEdit::textEdited);
    settingsObject.LogLevel.ReadWriteBind(logLevelComboBox, "currentIndex", &QComboBox::currentIndexChanged);
    settingsObject.OutboundMark.ReadWriteBind(somarkSB, "value", &QSpinBox::valueChanged);
}

void V2RayKernelSettings::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void V2RayKernelSettings::Load()
{
    settingsObject.loadJson(settings);
}

void V2RayKernelSettings::Store()
{
    settings = settingsObject.toJson();
}

void V2RayKernelSettings::on_selectVCoreBtn_clicked()
{
    const auto core = QFileDialog::getOpenFileName(this, tr("Open V2Ray core file"), QDir::currentPath());
    if (!core.isEmpty())
        settingsObject.CorePath = core;
}

void V2RayKernelSettings::on_selectVAssetBtn_clicked()
{
    const auto dir = QFileDialog::getExistingDirectory(this, tr("Open V2Ray assets folder"), QDir::currentPath());
    if (!dir.isEmpty())
        settingsObject.AssetsPath = dir;
}

void V2RayKernelSettings::on_checkVCoreSettings_clicked()
{
    if (const auto &[result, msg] = ValidateKernel(settingsObject.CorePath, settingsObject.AssetsPath); !result)
    {
        BuiltinV2RayCorePlugin::ShowMessageBox(tr("V2Ray Core Settings"), *msg);
    }
    else
    {
        const auto content = tr("V2Ray path configuration check passed.") + //
                             QStringLiteral("\n\n") +                       //
                             tr("Kernel Output: ") +                        //
                             QStringLiteral("\n") +                         //
                             *msg;
        BuiltinV2RayCorePlugin::ShowMessageBox(tr("V2Ray Core Settings"), content);
    }
}

void V2RayKernelSettings::on_detectCoreBtn_clicked()
{
    QStringList searchPaths;

    // A cursed v2ray core searcher.
#ifdef Q_OS_WINDOWS
#define PATH_SPLITTER ';'
#else
#define PATH_SPLITTER ':'
#endif

    searchPaths << QProcessEnvironment::systemEnvironment().value(QStringLiteral("PATH")).split(QChar::fromLatin1(PATH_SPLITTER));

    searchPaths << QDir::homePath();
    for (const auto &sp : {
             QStandardPaths::AppDataLocation,      //
             QStandardPaths::AppConfigLocation,    //
             QStandardPaths::AppLocalDataLocation, //
             QStandardPaths::ApplicationsLocation, //
             QStandardPaths::HomeLocation,         //
             QStandardPaths::DesktopLocation,      //
             QStandardPaths::DocumentsLocation,    //
             QStandardPaths::DownloadLocation,     //
         })
        for (const auto &dn : {
                 QStringLiteral("v2ray"),            //
                 QStringLiteral("v2ray-core"),       //
                 QStringLiteral("v2ray-windows-64"), //
             })
            searchPaths << QStandardPaths::locateAll(sp, dn, QStandardPaths::LocateDirectory);

#ifdef Q_OS_WINDOWS
    // Scoop shim causes problems: https://github.com/lukesampson/scoop/issues/3294
    searchPaths.removeIf([](const QString &s) { return s.contains(QStringLiteral("shims")); });
    searchPaths << QDir::homePath() + QStringLiteral("/scoop/apps/v2ray/current/");
    searchPaths << QDir::homePath() + QStringLiteral("/source/repos/v2ray-core/");
    searchPaths << QDir::homePath() + QStringLiteral("/source/repos/v2ray/");

    for (const auto &dl : QDir::drives())
        for (const auto &dn : {
                 QStringLiteral("v2ray"),            //
                 QStringLiteral("v2ray-core"),       //
                 QStringLiteral("v2ray-windows-64"), //
             })
            searchPaths << dl.absolutePath() + dn;
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    searchPaths << QStringLiteral("/bin");
    searchPaths << QStringLiteral("/usr/bin");
    searchPaths << QStringLiteral("/usr/local/bin");
    searchPaths << QStringLiteral("/usr/share/v2ray");
    searchPaths << QStringLiteral("/usr/local/share/v2ray");
    searchPaths << QStringLiteral("/usr/lib/v2ray");
    searchPaths << QStringLiteral("/usr/local/lib/v2ray");
    searchPaths << QStringLiteral("/opt/bin");
    searchPaths << QStringLiteral("/opt/v2ray");
    searchPaths << QStringLiteral("/usr/local/opt/bin");
    searchPaths << QStringLiteral("/usr/local/opt/v2ray");
#endif

    searchPaths << settingsObject.AssetsPath;
    searchPaths.removeDuplicates();

    QString corePath = settingsObject.CorePath;
    QString assetsPath = settingsObject.AssetsPath;

    bool assetsFound = false;
    bool coreFound = false;

    const auto result = QStandardPaths::findExecutable(QStringLiteral("v2ray"), searchPaths);
    if (!result.isEmpty())
    {
        corePath = result;
        coreFound = true;
    }

    for (const auto &d : searchPaths)
    {
        const QDir assetsdir{ d };
        if (assetsdir.entryList().contains(QStringLiteral("geosite.dat")) && assetsdir.entryList().contains(QStringLiteral("geoip.dat")))
        {
            assetsFound = true;
            assetsPath = assetsdir.path();
            break;
        }
    }

    QStringList messages;
    messages << (coreFound ? QStringLiteral("Found v2ray core at: ") + corePath : QStringLiteral("Cannot find v2ray core."));
    messages << (assetsFound ? QStringLiteral("Found v2ray assets at: ") + assetsPath : QStringLiteral("Cannot find v2ray assets."));

    BuiltinV2RayCorePlugin::ShowMessageBox(QStringLiteral("V2Ray Core Detection"), messages.join(QChar::fromLatin1('\n')));

    settingsObject.CorePath = corePath;
    settingsObject.AssetsPath = assetsPath;
}

void V2RayKernelSettings::on_resetVCoreBtn_clicked()
{
    settingsObject.CorePath = QStringLiteral(QV2RAY_DEFAULT_VCORE_PATH);
}

void V2RayKernelSettings::on_resetVAssetBtn_clicked()
{
    settingsObject.AssetsPath = QStringLiteral(QV2RAY_DEFAULT_VASSETS_PATH);
}
