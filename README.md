# awesome_like_workspaces
Emulates per monitor workspaces on Hyprland. Like i am used to from awesome wm.

## Installation
with hyprpm ([hyprland wiki](https://wiki.hyprland.org/Plugins/Using-Plugins/)):

```
hyprpm add https://github.com/gomfol12/awesome_like_workspaces
hyprpm enable awesome_like_workspaces
```

## Usage
Modify this config snippet to your liking and add it to your hyprland config.

```
plugin {
    awesome_like_workspaces {
        HDMI-A-1 {
            count = 9
            priority = 1
        }
        DVI-D-1 {
            count = 5
            priority = 2
        }
    }
}

# Switch workspaces
bind = $mainMod, 1, alw-workspace, 1
bind = $mainMod, 2, alw-workspace, 2
bind = $mainMod, 3, alw-workspace, 3
bind = $mainMod, 4, alw-workspace, 4
bind = $mainMod, 5, alw-workspace, 5
bind = $mainMod, 6, alw-workspace, 6
bind = $mainMod, 7, alw-workspace, 7
bind = $mainMod, 8, alw-workspace, 8
bind = $mainMod, 9, alw-workspace, 9

# Move active window to a workspace
bind = $mainMod SHIFT, 1, alw-movetoworkspacesilent, 1
bind = $mainMod SHIFT, 2, alw-movetoworkspacesilent, 2
bind = $mainMod SHIFT, 3, alw-movetoworkspacesilent, 3
bind = $mainMod SHIFT, 4, alw-movetoworkspacesilent, 4
bind = $mainMod SHIFT, 5, alw-movetoworkspacesilent, 5
bind = $mainMod SHIFT, 6, alw-movetoworkspacesilent, 6
bind = $mainMod SHIFT, 7, alw-movetoworkspacesilent, 7
bind = $mainMod SHIFT, 8, alw-movetoworkspacesilent, 8
bind = $mainMod SHIFT, 9, alw-movetoworkspacesilent, 9
```

Config variables:

- `count` - Number of workspaces per monitor
- `priority` - The monitor with the lowest priority will have workspace 1 and counting upwards to count workspaces. Aka the monitor where the counting starts.

This plugin implements three new keybind dispatchers:

- `alw-workspace` - Switch to a workspace
- `alw-movetoworkspace` - Move active window to a workspace
- `alw-movetoworkspacesilent` - Move active window to a workspace without changing the active workspace

All of them are local to the current focused monitor.

## Inspiration and alternatives
- [hyprsome](https://github.com/sopa0/hyprsome)
- [split-monitor-workspaces](https://github.com/Duckonaut/split-monitor-workspaces)
