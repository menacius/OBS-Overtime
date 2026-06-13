#include <QAction>
#include <QMainWindow>
#include <QPointer>
#include <QWidget>

#include "plugin-config.hpp"
#include "overlay-controller.hpp"
#include "settings-dialog.hpp"
#include "media-dock.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("projector-time-overlay", "en-US")

static const char *kDockId = "projector_time_overlay_media_dock";

static QPointer<SettingsDialog> g_settingsDialog;

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Configurable time overlay rendered on top of OBS projector "
           "windows (streaming/recording/media time).";
}

MODULE_EXPORT const char *obs_module_name(void)
{
    return obs_module_text("PluginName");
}

static void show_settings_dialog()
{
    void *mainWindowPtr = obs_frontend_get_main_window();
    QWidget *mainWindow = static_cast<QWidget *>(mainWindowPtr);

    if (!g_settingsDialog)
        g_settingsDialog = new SettingsDialog(mainWindow);

    g_settingsDialog->show();
    g_settingsDialog->raise();
    g_settingsDialog->activateWindow();
}

bool obs_module_load(void)
{
    PluginConfig::instance().load();

    MediaDock *dock = new MediaDock();
    dock->setObjectName("projectorTimeOverlayMediaDock");
    obs_frontend_add_dock_by_id(kDockId, obs_module_text("Dock.Title"), dock);

    QAction *action = static_cast<QAction *>(
        obs_frontend_add_tools_menu_qaction(obs_module_text("Menu.Settings")));
    if (action) {
        QObject::connect(action, &QAction::triggered,
                         []() { show_settings_dialog(); });
    }

    obs_frontend_add_event_callback(overlay_frontend_event, nullptr);

    OverlayController::instance()->start();

    blog(LOG_INFO, "[projector-time-overlay] loaded (v%s)", PLUGIN_VERSION);
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(overlay_frontend_event, nullptr);
    OverlayController::instance()->stop();
    PluginConfig::instance().save();
    blog(LOG_INFO, "[projector-time-overlay] unloaded");
}
