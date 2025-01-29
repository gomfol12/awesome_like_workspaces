#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <map>
#include <string>
#include <vector>

#include "project_vars.h"

inline HANDLE PHANDLE = nullptr;

static CHyprColor notification_color{1.0, 0.2, 0.2, 1.0};
static SP<HOOK_CALLBACK_FN> configReloaded_callback_ptr;
static SP<HOOK_CALLBACK_FN> monitorAdded_callback_ptr;
static SP<HOOK_CALLBACK_FN> monitorRemoved_callback_ptr;
static std::map<uint64_t, std::vector<std::string>> monitor_workspaces;
static std::map<uint64_t, int> config_monitor_workspace_count;
static std::map<uint64_t, int> config_monitor_priority;
static uint64_t currnet_workspace_id = 1;

static int get_config_value(const std::string &name);
static PHLMONITOR get_current_monitor();
static const std::string &get_workspace_from_monitor(PHLMONITOR monitor, const std::string &workspace);
static SDispatchResult alw_workspace(const std::string &workspace);
static SDispatchResult alw_movetoworkspace(const std::string &workspace);
static SDispatchResult alw_movetoworkspacesilent(const std::string &workspace);
static void get_next_monitor(PHLMONITOR monitor, PHLMONITOR *next_monitor);
static SDispatchResult alw_focusnextmonitor(const std::string &);
static SDispatchResult alw_movetonextmonitor(const std::string &);
static void create_workspaces(PHLMONITOR monitor);
static void remove_workspaces(PHLMONITOR monitor);
static void create_all_workspaces();
static void remove_all_workspaces();
static void load_all_config_values();
static void configReloaded_callback(void *, SCallbackInfo &, std::any);
static void monitorAdded_callback(void *, SCallbackInfo &, std::any);
static void monitorRemoved_callback(void *, SCallbackInfo &, std::any);

static int get_config_value(const std::string &name)
{
    const auto *const value = (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(PHANDLE, name)->getDataStaticPtr();

    if (value == nullptr)
        throw std::runtime_error("[" NAME "] Config value not found!");

    return **value;
}

static PHLMONITOR get_current_monitor()
{
    PHLMONITOR monitor = g_pCompositor->m_pLastMonitor.lock();

    if (monitor == nullptr)
    {
        monitor = g_pCompositor->getMonitorFromCursor();
    }

    return monitor;
}

static const std::string &get_workspace_from_monitor(PHLMONITOR monitor, const std::string &workspace)
{
    int workspace_index = std::stoi(workspace) - 1;

    if (workspace_index < 0 || workspace_index >= config_monitor_workspace_count[monitor->ID])
    {
        return monitor_workspaces[monitor->ID].back();
    }

    return monitor_workspaces[monitor->ID][workspace_index]; // 0 indexed
}

static SDispatchResult alw_workspace(const std::string &workspace)
{
    std::string out = HyprlandAPI::invokeHyprctlCommand(
        "dispatch", "workspace " + get_workspace_from_monitor(get_current_monitor(), workspace));

    if (out.find("ok") != std::string::npos)
        return {false, true, out};

    return {false, false, "alw_workspace: " + out};
}

static SDispatchResult alw_movetoworkspace(const std::string &workspace)
{
    std::string out = HyprlandAPI::invokeHyprctlCommand(
        "dispatch", "movetoworkspace " + get_workspace_from_monitor(get_current_monitor(), workspace));

    if (out.find("ok") != std::string::npos)
        return {false, true, out};

    return {false, false, "alw_movetoworkspace: " + out};
}

static SDispatchResult alw_movetoworkspacesilent(const std::string &workspace)
{
    std::string out = HyprlandAPI::invokeHyprctlCommand(
        "dispatch", "movetoworkspacesilent " + get_workspace_from_monitor(get_current_monitor(), workspace));

    if (out.find("ok") != std::string::npos)
        return {false, true, out};

    return {false, false, "alw_movetoworkspacesilent: " + out};
}

static void get_next_monitor(PHLMONITOR monitor, PHLMONITOR *next_monitor)
{
    for (int i = 0; i < g_pCompositor->m_vMonitors.size(); i++)
    {
        if (g_pCompositor->m_vMonitors[i] == monitor)
        {
            *next_monitor = g_pCompositor->m_vMonitors[(i + 1) % g_pCompositor->m_vMonitors.size()];
            return;
        }
    }

    *next_monitor = monitor;
    return;
}

static SDispatchResult alw_focusnextmonitor(const std::string &)
{
    PHLMONITOR monitor = get_current_monitor();
    PHLMONITOR next_monitor;
    get_next_monitor(monitor, &next_monitor);

    std::string out = HyprlandAPI::invokeHyprctlCommand("dispatch", "focusmonitor " + std::to_string(next_monitor->ID));

    if (out.find("ok") != std::string::npos)
        return {false, true, out};

    return {false, false, "alw_focusnextmonitor: " + out};
}

static SDispatchResult alw_movetonextmonitor(const std::string &)
{
    PHLMONITOR monitor = get_current_monitor();
    PHLMONITOR next_monitor;
    get_next_monitor(monitor, &next_monitor);

    std::string out =
        HyprlandAPI::invokeHyprctlCommand("dispatch", "movetoworkspace " + next_monitor->activeWorkspace->m_szName);

    if (out.find("ok") != std::string::npos)
        return {false, true, out};

    return {false, false, "alw_movetonextmonitor: " + out};
}

static void create_workspaces(PHLMONITOR monitor)
{
    // skip
    if (monitor->activeMonitorRule.disabled || monitor->isMirror())
        return;

    for (int i = 0; i < config_monitor_workspace_count[monitor->ID]; i++)
    {
        int64_t workspace_id = currnet_workspace_id++;

        monitor_workspaces[monitor->ID].push_back(std::to_string(workspace_id));
        PHLWORKSPACE workspace = g_pCompositor->getWorkspaceByName(std::to_string(workspace_id));

        if (workspace.get() == nullptr)
        {
            workspace = g_pCompositor->createNewWorkspace(workspace_id, monitor->ID);
        }
        g_pCompositor->moveWorkspaceToMonitor(workspace, monitor);
        workspace->m_bPersistent = true;
    }
}

static void remove_workspaces(PHLMONITOR monitor)
{
    if (monitor_workspaces.contains(monitor->ID))
    {
        for (const auto &workspace : monitor_workspaces[monitor->ID])
        {
            PHLWORKSPACE workspace_ptr = g_pCompositor->getWorkspaceByName(workspace);
            if (workspace_ptr.get() != nullptr)
            {
                workspace_ptr->m_bPersistent = false;
            }
        }
        monitor_workspaces.erase(monitor->ID);
    }
}

static void create_all_workspaces()
{
    // copy monitors
    std::vector<PHLMONITOR> monitors;
    std::transform(g_pCompositor->m_vMonitors.begin(), g_pCompositor->m_vMonitors.end(), std::back_inserter(monitors),
                   [](const auto &monitor) { return monitor; });

    // reorder monitors with priority
    std::sort(monitors.begin(), monitors.end(), [](const PHLMONITOR a, const PHLMONITOR b) {
        return config_monitor_priority[a->ID] < config_monitor_priority[b->ID];
    });

    for (auto &monitor : monitors)
    {
        create_workspaces(monitor);
    }
}

static void remove_all_workspaces()
{
    while (!monitor_workspaces.empty())
    {
        uint64_t monitor_id = monitor_workspaces.begin()->first;
        PHLMONITOR monitor = g_pCompositor->getMonitorFromID(monitor_id);
        if (monitor != nullptr)
        {
            remove_workspaces(monitor);
        }
        else
        {
            monitor_workspaces.erase(monitor_id);
        }
    }
    monitor_workspaces.clear();
    currnet_workspace_id = 1;
}

static void load_all_config_values()
{
    for (auto &monitor : g_pCompositor->m_vMonitors)
    {
        config_monitor_workspace_count[monitor->ID] = get_config_value("plugin:" NAME ":" + monitor->szName + ":count");
        config_monitor_priority[monitor->ID] = get_config_value("plugin:" NAME ":" + monitor->szName + ":priority");
    }
}

static void configReloaded_callback(void *, SCallbackInfo &, std::any)
{
    load_all_config_values();

    remove_all_workspaces();
    create_all_workspaces();
}

static void monitorAdded_callback(void *, SCallbackInfo &, std::any data)
{
    create_workspaces(std::any_cast<PHLMONITOR>(data));
}

static void monitorRemoved_callback(void *, SCallbackInfo &, std::any data)
{
    remove_workspaces(std::any_cast<PHLMONITOR>(data));
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    // ALWAYS add this to your plugins. It will prevent random crashes coming from
    // mismatched header versions.
    if (HASH != GIT_COMMIT_HASH)
    {
        HyprlandAPI::addNotification(PHANDLE, "[" NAME "] Mismatched headers! Can't proceed.", notification_color,
                                     5000);
        throw std::runtime_error("[" NAME "] Version mismatch");
    }

    for (auto &monitor : g_pCompositor->m_vMonitors)
    {
        HyprlandAPI::addConfigValue(PHANDLE, "plugin:" NAME ":" + monitor->szName + ":count", Hyprlang::INT{10});
        HyprlandAPI::addConfigValue(PHANDLE, "plugin:" NAME ":" + monitor->szName + ":priority",
                                    Hyprlang::INT{std::numeric_limits<int>::max()});
    }
    HyprlandAPI::addDispatcherV2(PHANDLE, "alw-workspace", alw_workspace);
    HyprlandAPI::addDispatcherV2(PHANDLE, "alw-movetoworkspace", alw_movetoworkspace);
    HyprlandAPI::addDispatcherV2(PHANDLE, "alw-movetoworkspacesilent", alw_movetoworkspacesilent);
    HyprlandAPI::addDispatcherV2(PHANDLE, "alw-focusnextmonitor", alw_focusnextmonitor);
    HyprlandAPI::addDispatcherV2(PHANDLE, "alw-movetonextmonitor", alw_movetonextmonitor);

    HyprlandAPI::reloadConfig();
    load_all_config_values();

    configReloaded_callback_ptr =
        HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", configReloaded_callback);
    monitorAdded_callback_ptr = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorAdded", monitorAdded_callback);
    monitorRemoved_callback_ptr =
        HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorRemoved", monitorRemoved_callback);

    return {NAME, DESCRIPTION, AUTHOR, VERSION};
}

APICALL EXPORT void PLUGIN_EXIT()
{
    remove_all_workspaces();
}
