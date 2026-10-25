// Editor-module entry: registers the Kenney Assets dock panel when the
// kenney-assets editor library loads (static init, the single-TU pattern the
// eztree and unity-import packages use). The library links the EditorSDK, so
// the registration lands in the host editor's panel registry, which replays it
// for packages that load after editor startup.

#include "KenneyPanel.h"

#include "Editor/Registries/EditorPanelRegistry.h"

#include <memory>

namespace
{

struct KenneyEditorModuleRegistrar
{
    KenneyEditorModuleRegistrar()
    {
        using namespace GameEngine;

        Editor::EditorPanelDescriptor panel;
        panel.PanelId = "KenneyAssets";
        panel.Title = "Kenney Assets";
        panel.AssetSourceAlias = "kenney-assets";
        panel.LayoutAssetPath = "Editor/UI/panels/KenneyPanel.uxml";
        panel.StyleAssetPath = "Editor/UI/panels/KenneyPanel.css";
        panel.Factory = []() -> std::unique_ptr<UIElement> { return std::make_unique<KenneyAssets::KenneyPanel>(); };
        Editor::EditorPanelRegistry::Get().RegisterPanel(std::move(panel));
    }
};

KenneyEditorModuleRegistrar s_Registrar;

} // namespace
