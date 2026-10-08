
// ge - ReXGlue Recompiled Project
//
// This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/ppc/func.h>
#include <rex/rex_app.h>
#include <rex/system/gpu_plugin.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/user_profile.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>

#include <string>
#include <algorithm>

#include "ge_menu.h"
#include "ge_postfx.h"

// Relaunch the current executable as a fresh process (implemented in
// ge_hooks.cpp). Used by the ONLINE menu's
// "Save & Restart" so username/server/enable changes take effect on a clean
// boot -- they are read at startup (UserProfile ctor, online client start).
namespace ge {
void LaunchSelfDetached();
// Initialize platform mouse input and cursor capture at startup. Implemented in
// ge_hooks.cpp.
void InitMouseLook(rex::ui::Window* window);
void ShutdownMouseLook();
// Suppress mouse-look while the pause menu is open (cursor is needed for the
// menu, and motion shouldn't turn into look). Implemented in ge_hooks.cpp.
void SetMouselookSuppressed(bool suppressed);
}

class GeApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<GeApp>(new GeApp(ctx, "ge",
        GetImageInfo()));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (!config.graphics && config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
#if defined(__linux__)
    // Load the plugin before choosing its defaults so its cvars have registered
    // and pending config/environment/CLI values have been applied. GPU setup
    // still happens afterwards through ReXApp.
    if (!config.graphics && config.gpu_plugin == "xenos") {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin);
      if (config.graphics &&
          rex::cvar::GetFlagSource("render_target_path_vulkan") == rex::cvar::Source::kDefault) {
        // GoldenEye aliases color/depth EDRAM during weapon transitions. The
        // host-framebuffer path can resolve the wrong owner and present black
        // frames. FSI keeps the guest EDRAM representation accurate.
        rex::cvar::SetFlagByName("render_target_path_vulkan", "fsi");
      }
      if (config.graphics) {
        REXGPU_INFO("GE Vulkan render target path: {}",
                    rex::cvar::GetFlagByName("render_target_path_vulkan"));
      }
    }
#endif
  }

  // GoldenEye boot defaults. Runs before the config file is loaded, so these
  // are just defaults -- ge.toml (written by the in-game menu) overrides them.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {
      paths.game_data_root = rex::filesystem::GetExecutableFolder() / "assets";
    }
    config_path_ = paths.config_path;
    // NOTE: vsync is NOT forced here. Its SDK default is false (off), so the
    // in-menu toggle persists: turning it ON differs from default -> written to
    // ge.toml; OFF == default -> not written but still boots off. Forcing it here
    // would re-assert off every boot and the "on" choice would never survive a
    // restart (SaveConfig only writes cvars that differ from their default).
    rex::cvar::SetFlagByName("max_fps", "60");  // default 60 (clamped to native refresh)
    // Let the SDK resolve window/video dimensions from resolution,
    // video_mode_width/height and window_width/height, including CLI overrides.
    // NOTE: fullscreen is NOT forced here. Its default is set to true at the
    // framework level (window.cpp) instead. That makes "windowed" the
    // non-default value, so toggling to windowed actually saves to ge.toml --
    // SaveConfig only writes cvars that differ from their default. Forcing
    // fullscreen=true here would re-assert it every boot and the windowed
    // choice would never persist. The throttle is the same story: its default
    // lives in its REXCVAR_DEFINE and it is tuned live from the pause menu, so
    // it is never written here (writing default==default is a no-op anyway).
  }

  // Register the ESC pause-menu keybind and create the always-on Post-FX
  // filter overlay once the ImGui drawer exists.
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    // Window/taskbar title shown while running. Overrides the SDK default
    // ("ge <build stamp>"); the internal app name stays "ge" so ge.toml and the
    // user data dir are unchanged.
    if (window()) window()->SetTitle("GoldenEye");
    rex::ui::RegisterBind("bind_pause_menu", "Escape", "Pause menu",
                          [this] { TogglePauseMenu(); });
    ge::InitMouseLook(window());
    postfx_ = std::make_unique<ge::PostFxOverlay>(drawer);
    // Username/server are set in the ONLINE pause-menu tab now -- no first-boot
    // prompt. They apply on the Save & Restart the ONLINE tab triggers.
  }

  // Tear down the menu, overlay and keybind before the drawer is destroyed.
  void OnShutdown() override {
    ge::ShutdownMouseLook();
    rex::ui::UnregisterBind("bind_pause_menu");
    if (menu_) {
      // Direct delete (not Close()) so we don't re-enter pause bookkeeping
      // during shutdown; removes itself from the drawer in its destructor.
      delete menu_;
      menu_ = nullptr;
    }
    postfx_.reset();
  }

 private:
  static rex::PPCImageInfo GetImageInfo() {
    auto info = PPCImageConfig;
    // The title has generated fragment entries at 0x830E0xxx, beyond the XEX's
    // static code section but still inside its image. ReXGlue 0.10 validates
    // every mapping against the dispatch range, so include those entries too.
    // This only expands host function dispatch metadata, not guest memory.
    for (auto* mapping = info.func_mappings; mapping && mapping->host; ++mapping) {
      if (mapping->guest >= info.code_base &&
          mapping->guest + 4 <= uint64_t(info.image_base) + info.image_size) {
        info.code_size = std::max(info.code_size,
            static_cast<uint32_t>(mapping->guest + 4 - info.code_base));
      }
    }
    return info;
  }

  void PersistConfig() { rex::cvar::SaveConfig(config_path_); }

  // ESC handler: open or close the menu. The game keeps running underneath.
  void TogglePauseMenu() {
    if (menu_) {
      menu_->RequestClose();  // on_closed clears menu_
      return;
    }
    GeMenuDialog::Callbacks cb;
    cb.on_closed = [this] {
      menu_ = nullptr;
      ge::SetMouselookSuppressed(false);  // re-enable mouse-look on menu close
    };
    cb.on_quit = [this] {
      if (runtime() && runtime()->kernel_state()) {
        runtime()->kernel_state()->TerminateTitle();
      }
      app_context().QuitFromUIThread();
    };
    cb.get_fullscreen = [this] { return window() && window()->IsFullscreen(); };
    cb.request_fullscreen = [this](bool v) {
      // Persist the choice: update the cvar (so SaveConfig writes it) and flush
      // ge.toml now. Without this the window changes but reverts next boot.
      rex::cvar::SetFlagByName("fullscreen", v ? "true" : "false");
      PersistConfig();
      // Defer off the paint thread: applying a window/surface change from inside
      // the ImGui draw (which runs during the presenter's paint) tears down the
      // surface being painted and crashes. Running it from the UI loop between
      // frames is the same safe path as a normal window resize.
      app_context().CallInUIThreadDeferred([this, v] {
        if (window()) window()->SetFullscreen(v);
      });
    };
    cb.persist_config = [this] { PersistConfig(); };
    cb.request_restart = [this] {
      // ONLINE tab "Save & Restart": the menu has already persisted the cvars;
      // launch a fresh process (which reads the new ge.toml at boot) then tear
      // this one down. Deferred to the UI thread -- never quit/relaunch from
      // inside the paint (same reason as request_fullscreen).
      app_context().CallInUIThreadDeferred([this] {
        ge::LaunchSelfDetached();
        if (runtime() && runtime()->kernel_state()) {
          runtime()->kernel_state()->TerminateTitle();
        }
        app_context().QuitFromUIThread();
      });
    };
    ge::SetMouselookSuppressed(true);  // freeze mouse-look while the menu is up
    menu_ = new GeMenuDialog(imgui_drawer(), std::move(cb));
  }

  GeMenuDialog* menu_ = nullptr;  // non-owning; self-deletes via the drawer
  std::filesystem::path config_path_;
  std::unique_ptr<ge::PostFxOverlay> postfx_;       // always-on filter layer
};
