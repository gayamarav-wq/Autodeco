#include "ToolsPopup.hpp"

#include <Geode/ui/MDPopup.hpp>
#include <map>
#include <set>

#include "../Catalog.hpp"
#include "../DecoManager.hpp"
#include "../PresetStore.hpp"
#include "TempoPopup.hpp"

using namespace geode::prelude;

namespace adeco::ui {

namespace { constexpr float W = 300.f, H = 262.f; }

ToolsPopup* ToolsPopup::create(LevelEditorLayer* lel, std::function<void()> onChanged) {
    auto ret = new ToolsPopup();
    if (ret->init(lel, std::move(onChanged))) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ToolsPopup::init(LevelEditorLayer* lel, std::function<void()> onChanged) {
    m_u = fitScale(W, H);
    if (!Popup::init(W * m_u, H * m_u)) return false;
    styleBackground(m_bgSprite);
    if (m_closeBtn) { m_closeBtn->setVisible(false); m_closeBtn->setEnabled(false); }
    m_lel = lel;
    m_onChanged = std::move(onChanged);

    place(label("Tools", "bigFont.fnt", 0.55f * m_u, kText, {0.f, 0.5f}), 20, H - 24);
    place(pill("x", U(22), U(22), kPanelLight, this, menu_selector(ToolsPopup::onX), 0.45f), W - 24, H - 24);

    auto& dm = DecoManager::get();
    std::string undoText = dm.undoDepth() ? "Undo: " + dm.lastRunLabel() : std::string("Undo last decoration");
    float bw = W - 40.f, y = H - 62.f;
    auto row = [&](std::string const& text, SEL_MenuHandler sel, Rgb bg = kPanelLight) {
        place(pill(text, U(bw), U(30), bg, this, sel, 0.38f), W / 2, y);
        y -= 38.f;
    };
    row(undoText, menu_selector(ToolsPopup::onUndo), dm.undoDepth() ? kPrimary : kPanelLight);
    row("Remove structure pieces", menu_selector(ToolsPopup::onRemoveStructure));
    row("Tempo sync...", menu_selector(ToolsPopup::onTempo));
    row("Verify presets", menu_selector(ToolsPopup::onVerify));
    row("Reload presets", menu_selector(ToolsPopup::onReload));
    row("Mod settings", menu_selector(ToolsPopup::onSettings));
    return true;
}

void ToolsPopup::onUndo(CCObject*) {
    std::string msg;
    bool ok = DecoManager::get().undoLast(m_lel, msg);
    Notification::create(msg, ok ? NotificationIcon::Success : NotificationIcon::Warning, 4.f)->show();
    if (ok && m_onChanged) m_onChanged();
    if (ok) this->onClose(nullptr);
}

void ToolsPopup::onRemoveStructure(CCObject*) {
    int n = 0;
    bool ok = DecoManager::get().removeStructure(m_lel, n);
    Notification::create(ok ? fmt::format("Removed {} structure pieces.", n) : std::string("No structure pieces to remove."),
                         ok ? NotificationIcon::Success : NotificationIcon::Warning, 3.f)->show();
}

void ToolsPopup::onTempo(CCObject*) {
    Ref<ToolsPopup> self = this;
    TempoPopup::create([self]() { if (self->m_onChanged) self->m_onChanged(); })->show();
}

void ToolsPopup::onVerify(CCObject*) {
    auto& store = PresetStore::get();
    GeodeCatalog cat(m_lel);
    std::vector<int> all;
    for (auto const& p : store.presets()) {
        auto ids = presetObjectIds(p);
        all.insert(all.end(), ids.begin(), ids.end());
    }
    cat.prime(all);  // asks the editor to create + delete one probe object per unseen ID

    std::string md = fmt::format("**{} presets loaded.**\n\n", store.presets().size());
    if (cat.probeRequested() > 0) {
        md += fmt::format("Probed {} object IDs; the editor created {} of them.\n\n", cat.probeRequested(), cat.probeCreated());
        if (cat.probeCreated() == 0)
            md += "**The editor created none of the probe objects**, so every ID will look unusable. "
                  "Please report this together with your Geode and game version.\n\n";
    }
    if (!store.errors().empty()) {
        md += "### Problems while loading\n";
        for (auto const& e : store.errors()) md += "- " + e + "\n";
        md += "\n";
    }

    std::map<std::string, std::set<int>> byReason;
    int problems = 0;
    float w, h;
    for (auto const& p : store.presets()) {
        std::set<int> bad;
        for (int id : presetObjectIds(p))
            if (!cat.usable(id, w, h)) bad.insert(id);
        if (bad.empty()) continue;
        ++problems;
        md += "- **" + p.name + "**: unusable object IDs (skipped when decorating):";
        for (int id : bad) {
            md += " " + std::to_string(id);
            byReason[cat.reason(id)].insert(id);
        }
        md += "\n";
    }
    if (!problems) md += "All object IDs used by the presets exist in this game version and are non-solid decorations.";
    else {
        md += "\n### Why\n";
        for (auto const& [why, ids] : byReason) {
            md += "- " + why + ":";
            int n = 0;
            for (int id : ids) { if (++n > 12) { md += " ..."; break; } md += " " + std::to_string(id); }
            md += "\n";
        }
    }
    md += "\n\nYour own presets go in:\n`" + store.userPresetDir() + "`";
    MDPopup::create("Preset check", md, "OK")->show();
}

void ToolsPopup::onReload(CCObject*) {
    PresetStore::get().reload();
    Notification::create(fmt::format("Reloaded: {} presets, {} problem(s).", PresetStore::get().presets().size(),
                                     PresetStore::get().errors().size()),
                         NotificationIcon::Success, 3.f)->show();
    if (m_onChanged) m_onChanged();
}

void ToolsPopup::onSettings(CCObject*) {
    openSettingsPopup(Mod::get(), false);
}

}  // namespace adeco::ui
