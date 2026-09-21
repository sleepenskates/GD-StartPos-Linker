#include <Geode/Geode.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>

#include <Geode/binding/EditLevelLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/LevelManagerDelegate.hpp>

#include <string>
#include <vector>
#include <cfloat>

using namespace geode::prelude;

struct LinkPair {
    int online = 0;         // online level id (positive)
    std::string localName;  // created level, identified by its name
};

namespace matjson {
    template <>
    struct Serialize<LinkPair> {
        static Result<LinkPair> fromJson(Value const& value) {
            auto res = LinkPair{};
            GEODE_UNWRAP_INTO(res.online, value["online"].asInt());
            GEODE_UNWRAP_INTO(res.localName, value["localName"].asString());
            return Ok(res);
        }
        static Value toJson(LinkPair const& value) {
            auto obj = Value::object();
            obj["online"] = value.online;
            obj["localName"] = value.localName;
            return obj;
        }
    };
}

class LevelOpenHelper : public cocos2d::CCObject, public LevelManagerDelegate {
protected:
    int m_target = 0;
    bool m_done = false;

public:
    static LevelOpenHelper* create(int target) {
        auto ret = new LevelOpenHelper();
        ret->m_target = target;
        ret->autorelease();
        return ret;
    }

    static void open(int target) {
        create(target)->start();
    }

    void start() {
        this->retain(); // keep alive until the download resolves
        auto gml = GameLevelManager::get();
        gml->m_levelManagerDelegate = this;
        gml->getOnlineLevels(GJSearchObject::create(SearchType::Type19, std::to_string(m_target)));
    }

    void finish(CCArray* levels) {
        if (m_done) return;
        m_done = true;

        auto gml = GameLevelManager::get();
        if (gml->m_levelManagerDelegate == this) {
            gml->m_levelManagerDelegate = nullptr;
        }

        GJGameLevel* found = nullptr;
        if (levels) {
            for (auto* level : CCArrayExt<GJGameLevel*>(levels)) {
                if (level->m_levelID == m_target) {
                    found = level;
                    break;
                }
            }
            if (!found && levels->count() > 0) {
                found = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
            }
        }

        if (found) {
            auto layer = LevelInfoLayer::create(found, false);
            auto scene = CCScene::create();
            scene->addChild(layer);
            CCDirector::get()->replaceScene(scene);
        }
        else {
            FLAlertLayer::create("Level Linker", "Could not find that online level. It may have been deleted.", "OK")->show();
        }
        this->release();
    }

    void fail() {
        if (m_done) return;
        m_done = true;

        auto gml = GameLevelManager::get();
        if (gml->m_levelManagerDelegate == this) {
            gml->m_levelManagerDelegate = nullptr;
        }
        FLAlertLayer::create("Level Linker", "Failed to load the online level. Check your connection.", "OK")->show();
        this->release();
    }

    void loadLevelsFinished(CCArray* levels, char const* key) override {
        finish(levels);
    }
    void loadLevelsFinished(CCArray* levels, char const* key, int type) override {
        finish(levels);
    }
    void loadLevelsFailed(char const* key) override {
        fail();
    }
    void loadLevelsFailed(char const* key, int type) override {
        fail();
    }
    void setupPageInfo(std::string info, char const* key) override {}
};

class Linker {
public:
    static std::vector<LinkPair> getLinks() {
        return Mod::get()->getSavedValue<std::vector<LinkPair>>("links", {});
    }

    static void setLinks(std::vector<LinkPair> const& links) {
        Mod::get()->setSavedValue("links", links);
    }

    static std::string getPartnerLocalName(int onlineId) {
        for (auto const& pair : getLinks()) {
            if (pair.online == onlineId) return pair.localName;
        }
        return "";
    }

    static int getPartnerOnlineId(std::string const& localName) {
        for (auto const& pair : getLinks()) {
            if (pair.localName == localName) return pair.online;
        }
        return 0;
    }

    static bool hasLinkOnline(int onlineId) {
        return !getPartnerLocalName(onlineId).empty();
    }

    static bool hasLinkLocal(std::string const& localName) {
        return getPartnerOnlineId(localName) != 0;
    }

    static void addLink(int onlineId, std::string const& localName) {
        auto links = getLinks();
        links.push_back(LinkPair{ onlineId, localName });
        setLinks(links);
    }

    static bool removeLinkOnline(int onlineId) {
        auto links = getLinks();
        for (auto it = links.begin(); it != links.end(); ++it) {
            if (it->online == onlineId) {
                links.erase(it);
                setLinks(links);
                return true;
            }
        }
        return false;
    }

    static bool removeLinkLocal(std::string const& localName) {
        auto links = getLinks();
        for (auto it = links.begin(); it != links.end(); ++it) {
            if (it->localName == localName) {
                links.erase(it);
                setLinks(links);
                return true;
            }
        }
        return false;
    }

    // --- pending link (one side picked, waiting for the other) ---
    static void setPendingOnline(int id) {
        Mod::get()->setSavedValue<int>("pendingOnline", id);
        Mod::get()->setSavedValue<std::string>("pendingLocalName", "");
    }

    static void setPendingLocalName(std::string const& name) {
        Mod::get()->setSavedValue<int>("pendingOnline", 0);
        Mod::get()->setSavedValue<std::string>("pendingLocalName", name);
    }

    static int getPendingOnline() {
        return Mod::get()->getSavedValue<int>("pendingOnline", 0);
    }

    static std::string getPendingLocalName() {
        return Mod::get()->getSavedValue<std::string>("pendingLocalName", "");
    }

    static void clearPending() {
        Mod::get()->setSavedValue<int>("pendingOnline", 0);
        Mod::get()->setSavedValue<std::string>("pendingLocalName", "");
    }

    static GJGameLevel* getLocalLevel(std::string const& name) {
        for (auto* level : CCArrayExt<GJGameLevel*>(LocalLevelManager::get()->m_localLevels)) {
            if (level->m_levelName == name) return level;
        }
        return nullptr;
    }

    static void onLinkPressedOnline(int id) {
        if (hasLinkOnline(id)) {
            geode::createQuickPopup(
                "Level Linker", "Unlink this level?",
                "Cancel", "Unlink",
                [id](FLAlertLayer*, bool confirm) {
                    if (confirm) removeLinkOnline(id);
                }
            );
            return;
        }

        std::string pendingLocal = getPendingLocalName();
        if (!pendingLocal.empty()) {
            removeLinkLocal(pendingLocal);
            addLink(id, pendingLocal);
            clearPending();
            FLAlertLayer::create("Level Linker", "Levels linked! Use <cy>Switch</c> to jump between them.", "OK")->show();
            return;
        }

        int pendingOnline = getPendingOnline();
        if (pendingOnline == id) {
            clearPending();
            FLAlertLayer::create("Level Linker", "Link cancelled.", "OK")->show();
            return;
        }
        if (pendingOnline != 0) {
            FLAlertLayer::create(
                "Level Linker",
                "Link this level with a <co>created level</c> (press <cg>Link</c> on it), not another online level.",
                "OK"
            )->show();
            return;
        }

        setPendingOnline(id);
        FLAlertLayer::create(
            "Level Linker",
            "Link pending! Now open the <co>created level</c> and press <cg>Link</c> again.",
            "OK"
        )->show();
    }

    static void onLinkPressedLocal(std::string const& name) {
        if (name.empty()) {
            FLAlertLayer::create(
                "Level Linker",
                "Give this level a <cy>name</c> and save it first, then press <cg>Link</c>.",
                "OK"
            )->show();
            return;
        }

        if (hasLinkLocal(name)) {
            geode::createQuickPopup(
                "Level Linker", "Unlink this level?",
                "Cancel", "Unlink",
                [name](FLAlertLayer*, bool confirm) {
                    if (confirm) removeLinkLocal(name);
                }
            );
            return;
        }

        int pendingOnline = getPendingOnline();
        if (pendingOnline != 0) {
            removeLinkOnline(pendingOnline);
            addLink(pendingOnline, name);
            clearPending();
            FLAlertLayer::create("Level Linker", "Levels linked! Use <cy>Switch</c> to jump between them.", "OK")->show();
            return;
        }

        std::string pendingLocal = getPendingLocalName();
        if (pendingLocal == name) {
            clearPending();
            FLAlertLayer::create("Level Linker", "Link cancelled.", "OK")->show();
            return;
        }
        if (!pendingLocal.empty()) {
            FLAlertLayer::create(
                "Level Linker",
                "Link this level with an <cg>online level</c> (press <cg>Link</c> on it), not another created level.",
                "OK"
            )->show();
            return;
        }

        setPendingLocalName(name);
        FLAlertLayer::create(
            "Level Linker",
            "Link pending! Now open the <cg>online level</c> and press <cg>Link</c> again.",
            "OK"
        )->show();
    }

    static void onSwitchPressedOnline(int id) {
        auto partner = getPartnerLocalName(id);
        if (partner.empty()) return;

        auto* level = getLocalLevel(partner);
        if (!level) {
            FLAlertLayer::create("Level Linker", "That created level is no longer in your saved levels.", "OK")->show();
            removeLinkOnline(id);
            return;
        }
        auto layer = EditLevelLayer::create(level);
        auto scene = CCScene::create();
        scene->addChild(layer);
        CCDirector::get()->replaceScene(scene);
    }

    static void onSwitchPressedLocal(std::string const& name) {
        int partner = getPartnerOnlineId(name);
        if (partner == 0) return;
        LevelOpenHelper::open(partner);
    }

    static bool nodeHasIcon(cocos2d::CCNode* node, char const* frame) {
        if (auto* spr = dynamic_cast<cocos2d::CCSprite*>(node)) {
            if (auto* f = spr->displayFrame()) {
                if (f->getFrameName() == gd::string(frame)) return true;
            }
        }
        if (auto* arr = node->getChildren()) {
            for (auto* child : CCArrayExt<cocos2d::CCNode*>(arr)) {
                if (nodeHasIcon(child, frame)) return true;
            }
        }
        return false;
    }

    // Finds the CCMenu that holds the button with the given icon (copy,
    // settings, trash, ...). Searches every menu anywhere in the layer.
    static cocos2d::CCMenu* getIconMenu(cocos2d::CCLayer* layer, char const* frame) {
        std::vector<cocos2d::CCNode*> stack;
        for (auto* c : CCArrayExt<cocos2d::CCNode*>(layer->getChildren())) {
            stack.push_back(c);
        }
        while (!stack.empty()) {
            auto* n = stack.back();
            stack.pop_back();
            if (auto* menu = dynamic_cast<cocos2d::CCMenu*>(n)) {
                for (auto* item : CCArrayExt<cocos2d::CCNode*>(menu->getChildren())) {
                    if (nodeHasIcon(item, frame)) return menu;
                }
            }
            if (auto* arr = n->getChildren()) {
                for (auto* c : CCArrayExt<cocos2d::CCNode*>(arr)) {
                    stack.push_back(c);
                }
            }
        }
        return nullptr;
    }

    // Returns the world position one stack-step above the topmost button in
    // the copy/settings column (the left column of the right-hand panel).
    static bool getColumnTop(cocos2d::CCMenu* menu, float& outX, float& outY) {
        float leftX = 0.f, topY = -FLT_MAX;
        bool found = false;
        for (auto* child : CCArrayExt<cocos2d::CCNode*>(menu->getChildren())) {
            if (child->getPositionX() < 0.f) {
                if (!found || child->getPositionY() > topY) {
                    leftX = child->getPositionX();
                    topY = child->getPositionY();
                    found = true;
                }
            }
        }
        if (!found) return false;
        auto base = menu->convertToWorldSpace({ leftX, topY });
        outX = base.x;
        outY = base.y + 57.f;
        return true;
    }

    // Anchor to the copy/like/star-rate buttons on LevelInfoLayer directly.
    static bool getInfoButtonSpot(cocos2d::CCLayer* layer, float& outX, float& outY) {
        if (auto* info = dynamic_cast<LevelInfoLayer*>(layer)) {
            CCMenuItemSpriteExtra* anchor = info->m_cloneBtn;
            if (!anchor) anchor = info->m_likeBtn;
            if (!anchor) anchor = info->m_starRateBtn;
            if (!anchor) anchor = info->m_demonRateBtn;
            if (anchor) {
                auto p = anchor->convertToWorldSpace(cocos2d::CCPointZero);
                outX = p.x;
                outY = p.y + 57.f;
                return true;
            }
        }
        return false;
    }

    static CCMenuItemSpriteExtra* makeCircleButton(
        char const* frame, CircleBaseColor color, CircleBaseSize size,
        cocos2d::CCObject* target, SEL_MenuHandler sel
    ) {
        return CCMenuItemSpriteExtra::create(
            CircleButtonSprite::createWithSpriteFrameName(frame, 1.f, color, size),
            target, sel
        );
    }

    static void attachButtons(
        cocos2d::CCLayer* layer, int levelId, std::string const& levelName,
        cocos2d::CCObject* target,
        SEL_MenuHandler onLink, SEL_MenuHandler onSwitch, SEL_MenuHandler onUnlink,
        bool onInfoLayer
    ) {
        if (auto old = layer->getChildByID("linker-menu"_spr)) {
            old->removeFromParent();
        }

        bool linked = onInfoLayer ? hasLinkOnline(levelId) : hasLinkLocal(levelName);

        if (onInfoLayer) {
            // Dock to the copy/like buttons' column: one stack-step above
            // them, same horizontal line. Fallbacks: icon search, then a
            // plain left-side spot (never center).
            auto win = CCDirector::get()->getWinSize();
            auto worldPos = cocos2d::CCPoint{ 40.f, win.height * 0.5f + 40.f };
            if (getInfoButtonSpot(layer, worldPos.x, worldPos.y)) {
                // Slide halfway back toward the center (never on it).
                worldPos.x += (win.width * 0.5f - worldPos.x) * 0.5f;
                geode::log::info(
                    "Linker attach info: anchored to column button at ({},{})",
                    worldPos.x, worldPos.y
                );
            }
            else {
                auto menu = getIconMenu(layer, "GJ_copyBtn_001.png");
                if (!menu) menu = getIconMenu(layer, "GJ_optionsBtn_001.png");
                if (!menu) menu = getIconMenu(layer, "GJ_trashBtn_001.png");
                if (menu && getColumnTop(menu, worldPos.x, worldPos.y)) {
                    worldPos.x += (win.width * 0.5f - worldPos.x) * 0.5f;
                    geode::log::info(
                        "Linker attach info: anchored to icon column at ({},{})",
                        worldPos.x, worldPos.y
                    );
                }
                else {
                    worldPos = cocos2d::CCPoint{ 40.f, win.height * 0.5f + 40.f };
                    geode::log::info("Linker attach info: fallback left position");
                }
            }
            if (worldPos.y > win.height - 30.f) worldPos.y = win.height - 30.f;

            auto menu2 = CCMenu::create();
            menu2->setPosition({ 0.f, 0.f });
            menu2->setID("linker-menu"_spr);
            layer->addChild(menu2, 100);

            if (linked) {
                auto sw = makeCircleButton("GJ_arrow_02_001.png", CircleBaseColor::Blue, CircleBaseSize::Small, target, onSwitch);
                sw->setPosition(worldPos);
                sw->setID("switch-button"_spr);
                menu2->addChild(sw);

                auto ul = makeCircleButton("GJ_closeBtn_001.png", CircleBaseColor::Red, CircleBaseSize::Tiny, target, onUnlink);
                ul->setPosition({ worldPos.x, worldPos.y - 57.f });
                ul->setID("unlink-button"_spr);
                menu2->addChild(ul);
            }
            else {
                auto linkBtn = makeCircleButton(
                    "edit_eStartPosBtn_001.png", CircleBaseColor::Green, CircleBaseSize::Small,
                    target, onLink
                );
                linkBtn->setPosition(worldPos);
                linkBtn->setID("link-button"_spr);
                menu2->addChild(linkBtn);
            }
        }
        else {
            // Created level: fixed stack under the back button (top-left).
            auto win = CCDirector::get()->getWinSize();
            auto menu = CCMenu::create();
            menu->setPosition({ 0.f, 0.f });
            menu->setID("linker-menu"_spr);
            layer->addChild(menu, 100);

            auto center = cocos2d::CCPoint{ 35.f, win.height - 95.f };
            if (linked) {
                auto sw = makeCircleButton("GJ_arrow_02_001.png", CircleBaseColor::Blue, CircleBaseSize::Small, target, onSwitch);
                sw->setPosition(center);
                sw->setID("switch-button"_spr);
                menu->addChild(sw, 100);

                auto ul = makeCircleButton("GJ_closeBtn_001.png", CircleBaseColor::Red, CircleBaseSize::Tiny, target, onUnlink);
                ul->setPosition({ 35.f, win.height - 156.f });
                ul->setID("unlink-button"_spr);
                menu->addChild(ul, 100);
            }
            else {
                auto linkBtn = makeCircleButton(
                    "GJ_hammerIcon_001.png", CircleBaseColor::Green, CircleBaseSize::Small,
                    target, onLink
                );
                linkBtn->setPosition(center);
                linkBtn->setID("link-button"_spr);
                menu->addChild(linkBtn, 100);
            }
        }
    }
};

class $modify(LevelHopInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        Linker::attachButtons(
            this, level->m_levelID, "", this,
            menu_selector(LevelHopInfoLayer::onLink),
            menu_selector(LevelHopInfoLayer::onSwitch),
            menu_selector(LevelHopInfoLayer::onUnlink),
            true
        );
        return true;
    }

    void onLink(CCObject*) {
        Linker::onLinkPressedOnline(this->m_level->m_levelID);
        Linker::attachButtons(
            this, this->m_level->m_levelID, "", this,
            menu_selector(LevelHopInfoLayer::onLink),
            menu_selector(LevelHopInfoLayer::onSwitch),
            menu_selector(LevelHopInfoLayer::onUnlink),
            true
        );
    }

    void onSwitch(CCObject*) {
        Linker::onSwitchPressedOnline(this->m_level->m_levelID);
    }

    void onUnlink(CCObject*) {
        Linker::removeLinkOnline(this->m_level->m_levelID);
        Linker::attachButtons(
            this, this->m_level->m_levelID, "", this,
            menu_selector(LevelHopInfoLayer::onLink),
            menu_selector(LevelHopInfoLayer::onSwitch),
            menu_selector(LevelHopInfoLayer::onUnlink),
            true
        );
    }
};

class $modify(LevelHopEditLayer, EditLevelLayer) {
    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        Linker::attachButtons(
            this, 0, level->m_levelName, this,
            menu_selector(LevelHopEditLayer::onLink),
            menu_selector(LevelHopEditLayer::onSwitch),
            menu_selector(LevelHopEditLayer::onUnlink),
            false
        );
        return true;
    }

    void onLink(CCObject*) {
        Linker::onLinkPressedLocal(this->m_level->m_levelName);
        Linker::attachButtons(
            this, 0, this->m_level->m_levelName, this,
            menu_selector(LevelHopEditLayer::onLink),
            menu_selector(LevelHopEditLayer::onSwitch),
            menu_selector(LevelHopEditLayer::onUnlink),
            false
        );
    }

    void onSwitch(CCObject*) {
        Linker::onSwitchPressedLocal(this->m_level->m_levelName);
    }

    void onUnlink(CCObject*) {
        Linker::removeLinkLocal(this->m_level->m_levelName);
        Linker::attachButtons(
            this, 0, this->m_level->m_levelName, this,
            menu_selector(LevelHopEditLayer::onLink),
            menu_selector(LevelHopEditLayer::onSwitch),
            menu_selector(LevelHopEditLayer::onUnlink),
            false
        );
    }
};