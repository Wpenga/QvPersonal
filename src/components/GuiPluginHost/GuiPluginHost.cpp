#include "GuiPluginHost.hpp"

#include "Qv2rayBase/Plugin/PluginManagerCore.hpp"
#include "Qv2rayBase/Qv2rayBaseLibrary.hpp"

namespace Qv2ray::ui::common
{
    GuiPluginAPIHost::GuiPluginAPIHost()
    {
        apiHost = QvPluginAPIHost;
    }

    QList<std::pair<Qv2rayPlugin::Qv2rayInterfaceImpl *, Qv2rayPlugin::Gui::Qv2rayGUIInterface *>> GuiPluginAPIHost::GUI_QueryByComponent(
        Qv2rayPlugin::PLUGIN_GUI_COMPONENT_TYPE c) const
    {
        QList<std::pair<Qv2rayPlugin::Qv2rayInterfaceImpl *, Qv2rayPlugin::Gui::Qv2rayGUIInterface *>> guiInterfaces;
        for (const auto &plugin : QvPluginManagerCore->GetPlugins(Qv2rayPlugin::COMPONENT_GUI))
        {
            const auto guiInterface = plugin->pinterface->GetGUIInterface();
            if (guiInterface->GetComponents().contains(c))
                guiInterfaces << std::make_pair(plugin->pinterface, guiInterface);
        }
        return guiInterfaces;
    }
} // namespace Qv2ray::ui::common
