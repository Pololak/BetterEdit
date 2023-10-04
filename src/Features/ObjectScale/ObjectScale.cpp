#include <Geode/Geode.hpp>
#include <Geode/modify/GJScaleControl.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "InputScaleDelegate.hpp"
#include "LockButton.hpp"

using namespace geode::prelude;

class $modify(BetterScaleControl, GJScaleControl) {
    CCTextInputNode* m_textInput = nullptr;
    CCScale9Sprite* m_textInputSprite = nullptr;
    Patch* m_absolutePositionPatch = nullptr;
    InputScaleDelegate* m_inputScaleDelegate = nullptr;

    bool m_scalingUILoaded = false;

    bool init() {
        if (!GJScaleControl::init()) {
            return false;
        }

        if (!Mod::get()->getSettingValue<bool>("better-scaling")) {
            return true;
        }

        this->setupScalingUI();

        return true;
    }

    void setupScalingUI() {
        m_label->setVisible(false);

        auto sprite = CCScale9Sprite::create(
            "square02b_001.png", { 0.0f, 0.0f, 80.0f, 80.0f }
        );

        sprite->setScale(0.3f);
        sprite->setColor({ 0, 0, 0 });
        sprite->setOpacity(100);
        sprite->setContentSize({ 115.0f, 75.0f });
        sprite->setPosition(m_label->getPosition());
        sprite->setID("scale-input-sprite"_spr);
        m_fields->m_textInputSprite = sprite;

        auto input = CCTextInputNode::create(40.0f, 30.0f, "1.00", "bigFont.fnt");
        input->setPosition(m_label->getPosition());
        input->ignoreAnchorPointForPosition(false);
        input->setLabelPlaceholderColor({ 120, 120, 120 });
        input->setAllowedChars(".0123456789");
        input->m_textField->setAnchorPoint({ 0.5f, 0.5f });
        input->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
        input->setScale(0.7f);
        input->setID("scale-input"_spr);
        input->setMouseEnabled(true);
        input->setTouchEnabled(true);
        m_fields->m_textInput = input;

        bool scaleSnap = Mod::get()->getSavedValue<bool>("scale-snap-enabled", false);
        bool absolutePosition = Mod::get()->getSavedValue<bool>("absolute-position-enabled", false);

        Mod::get()->setSavedValue<bool>("scale-snap-enabled", scaleSnap);
        Mod::get()->setSavedValue<bool>("absolute-position-enabled", absolutePosition);

        if (absolutePosition && m_fields->m_absolutePositionPatch == nullptr) {
            m_fields->m_absolutePositionPatch = Mod::get()->patch(reinterpret_cast<void*>(base::get() + 0x8f2f9), { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }).unwrap();
            log::info("patched successfully!");
        } else if (!absolutePosition && m_fields->m_absolutePositionPatch != nullptr) {
            Mod::get()->unpatch(m_fields->m_absolutePositionPatch).unwrap();
            m_fields->m_absolutePositionPatch = nullptr;
            log::info("unpatched successfully!");
        }

        auto menu = CCMenu::create();
        menu->setID("lock-menu"_spr);

        auto absoluteLockButton = LockButton::create(
            CCSprite::create("GJ_button_01.png"),
            this,
            menu_selector(BetterScaleControl::onAbsolutePositionSwap),
            SliderLockType::LockAbsolute
        );
        absoluteLockButton->setLockedStatus(absolutePosition);
        absoluteLockButton->setID("absolute-lock"_spr);
        absoluteLockButton->setPosition({ -30.0f, 0.f });
        menu->addChild(absoluteLockButton);

        auto sliderSnapButton = LockButton::create(
            CCSprite::create("GJ_button_01.png"),
            this,
            menu_selector(BetterScaleControl::onSliderSnapSwap),
            SliderLockType::LockSlider
        );

        sliderSnapButton->setLockedStatus(scaleSnap);
        sliderSnapButton->setID("snap-lock"_spr);
        sliderSnapButton->setPosition({ 30.0f, 0.f });
        menu->addChild(sliderSnapButton);

        menu->setPosition(input->getPosition());

        auto delegate = InputScaleDelegate::create(this, absoluteLockButton, sliderSnapButton);
        delegate->setID("input-delegate"_spr);
        m_fields->m_inputScaleDelegate = delegate;
        input->setDelegate(delegate);

        this->addChild(delegate);
        this->addChild(sprite);
        this->addChild(input);
        this->addChild(menu);

        m_fields->m_scalingUILoaded = true;
    }

    void revertScalingUI() {
        m_label->setVisible(true);

        if (m_fields->m_textInput) {
            m_fields->m_textInput->removeFromParentAndCleanup(true);
            m_fields->m_inputScaleDelegate = nullptr;
        }
        if (m_fields->m_textInputSprite) {
            m_fields->m_textInputSprite->removeFromParentAndCleanup(true);
            m_fields->m_inputScaleDelegate = nullptr;
        }
        if (m_fields->m_inputScaleDelegate) {
            m_fields->m_inputScaleDelegate->removeFromParentAndCleanup(true);
            m_fields->m_inputScaleDelegate = nullptr;
        }

        if (m_fields->m_absolutePositionPatch) {
            Mod::get()->unpatch(m_fields->m_absolutePositionPatch).unwrap();
            m_fields->m_absolutePositionPatch = nullptr;
        }

        auto menu = this->getChildByID("lock-menu"_spr);
        if (menu) {
            menu->removeFromParentAndCleanup(true);
        }

        m_fields->m_scalingUILoaded = false;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_scalingUILoaded) {
            GJScaleControl::ccTouchMoved(touch, event);
            return;
        }

        auto shouldSnap = Mod::get()->getSavedValue<bool>("scale-snap-enabled");
        if (!shouldSnap) {
            GJScaleControl::ccTouchMoved(touch, event);
            return;
        }
        if (!m_touchID == touch->getID()) {
            return;
        }

        m_slider->ccTouchMoved(touch, event);
        float val = roundf((m_slider->m_touchLogic->m_thumb->getValue() * 1.5f + 0.5f) * 100) / 100;
        log::info("slider value: {}, val: {}", m_slider->m_touchLogic->m_thumb->getValue(), val);

        float snap = static_cast<float>(Mod::get()->getSettingValue<double>("scale-snap"));
        val = roundf(val / snap) * snap;
        log::info("new val {}", val);
        if (val < 0.5f) {
            val = 0.5f;
        }

        m_slider->setValue((val - 0.5f) / 1.5f);

        if (m_delegate) {
            m_delegate->scaleChanged(val);
        }

        m_value = val;

        this->updateLabel(val);
    }

    void onAbsolutePositionSwap(CCObject* sender) {
        log::info("swapping absolute position swap");
        bool enabled = !Mod::get()->getSavedValue<bool>("absolute-position-enabled");
        Mod::get()->setSavedValue("absolute-position-enabled", enabled);

        if (enabled && m_fields->m_absolutePositionPatch == nullptr) {
            m_fields->m_absolutePositionPatch = Mod::get()->patch(reinterpret_cast<void*>(base::get() + 0x8f2f9), { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }).unwrap();
            log::info("patched successfully!");
        } else if (!enabled && m_fields->m_absolutePositionPatch != nullptr) {
            log::info("unpatched successfully!");
            Mod::get()->unpatch(m_fields->m_absolutePositionPatch).unwrap();
            m_fields->m_absolutePositionPatch = nullptr;
        }

        std::string message = "";
        if (enabled) {
            message = "Absolute positioning enabled";
        } else {
            message = "Absolute positioning disabled";
        }

        Notification::create(message, CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();

        static_cast<LockButton*>(sender)->setLockedStatus(enabled);
    }

    void onSliderSnapSwap(CCObject* sender) {
        log::info("swapping slider snap swap");
        bool enabled = !Mod::get()->getSavedValue<bool>("scale-snap-enabled");
        Mod::get()->setSavedValue("scale-snap-enabled", enabled);

        std::string message = "";
        if (enabled) {
            message = "Scale snap enabled";
        } else {
            message = "Scale snap disabled";
        }

        Notification::create(message, CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png"))->show();

        static_cast<LockButton*>(sender)->setLockedStatus(enabled);
    }

    void loadValues(GameObject* object, CCArray* objects) {
        if (!Mod::get()->getSettingValue<bool>("better-scaling")) {
            if (m_fields->m_scalingUILoaded) {
                this->revertScalingUI();
            }

            GJScaleControl::loadValues(object, objects);
            return;
        }
        if (!m_fields->m_scalingUILoaded) {
            this->setupScalingUI();
        }

        GJScaleControl::loadValues(object, objects);

        float value = m_slider->m_touchLogic->m_thumb->getValue();
        auto scale = 1.5f * value + 0.5f;
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << scale;

        std::string s = ss.str();

        if (m_fields->m_textInput != nullptr) {
            m_fields->m_textInput->setString(s);
        }
    }

    void updateLabel(float value) {
        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_scalingUILoaded) {
            GJScaleControl::updateLabel(value);
            return;
        }
        GJScaleControl::updateLabel(value);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << m_value;

        std::string s = ss.str();

        if (m_fields->m_textInput != nullptr) {
            m_fields->m_textInput->setString(s);
        }
    }
};

class $modify(ScaleEditorUI, EditorUI) {
    bool m_isTouchingScaleSnap = false;
    bool m_isTouchingAbsoluteLock = false;

    bool m_betterScalingActivated = false;

    void activateScaleControl(CCObject* sender) {
        auto setting = Mod::get()->getSetting("better-scaling");
        EditorUI::activateScaleControl(sender);

        if (m_scaleControl->getChildByID("lock-menu"_spr) == nullptr) {
            m_fields->m_betterScalingActivated = false;
        } else {
            m_fields->m_betterScalingActivated = true;
        }

        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_betterScalingActivated) {
            return;
        }
        auto pos = CCDirector::sharedDirector()->getWinSize() / 2;
        pos.height += 50.0f;
        pos = m_editorLayer->m_objectLayer->convertToNodeSpace(pos);
        m_scaleControl->setPosition(pos);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_betterScalingActivated) {
            return EditorUI::ccTouchBegan(touch, event);
        }

        if (this->touchedScaleInput(touch, event)) {
            return true;
        }

        if (this->touchedLockButton(touch, event)) {
            return true;
        }
        return EditorUI::ccTouchBegan(touch, event);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) {
        EditorUI::ccTouchEnded(touch, event);
        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_betterScalingActivated) {
            return;
        }
        log::info("touch ended");

        if (!m_fields->m_isTouchingAbsoluteLock && !m_fields->m_isTouchingScaleSnap) {
            return;
        }

        auto delegate = static_cast<InputScaleDelegate*>(this->m_scaleControl->getChildByID("input-delegate"_spr));
        if (!delegate) {
            m_fields->m_isTouchingAbsoluteLock = false;
            m_fields->m_isTouchingScaleSnap = false;
            return;
        }

        if (this->mouseIsHoveringNode(delegate->m_snapLock, touch->getLocation())) {
            log::info("deselect snap lock");
            delegate->m_snapLock->unselected();

            if (m_fields->m_isTouchingScaleSnap) {
                delegate->m_snapLock->activate();
            }
        }

        if (this->mouseIsHoveringNode(delegate->m_absoluteLock, touch->getLocation())) {
            log::info("deselect absolute lock");
            delegate->m_absoluteLock->unselected();
            if (m_fields->m_isTouchingAbsoluteLock) {
                delegate->m_absoluteLock->activate();
            }
        }

        m_fields->m_isTouchingAbsoluteLock = false;
        m_fields->m_isTouchingScaleSnap = false;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        EditorUI::ccTouchMoved(touch, event);
        if (!Mod::get()->getSettingValue<bool>("better-scaling") && !m_fields->m_betterScalingActivated) {
            return;
        }

        if (!m_fields->m_isTouchingAbsoluteLock && !m_fields->m_isTouchingScaleSnap) {
            return;
        }

        auto delegate = static_cast<InputScaleDelegate*>(this->m_scaleControl->getChildByID("input-delegate"_spr));
        if (!delegate) {
            return;
        }

        if (this->mouseIsHoveringNode(delegate->m_snapLock, touch->getLocation())) {
            delegate->m_snapLock->selected();
        } else if (delegate->m_snapLock->isSelected()) {
            delegate->m_snapLock->unselected();
        }

        if (this->mouseIsHoveringNode(delegate->m_absoluteLock, touch->getLocation())) {
            delegate->m_absoluteLock->selected();
        } else if (delegate->m_absoluteLock->isSelected()) {
            delegate->m_absoluteLock->unselected();
        }
    }

    bool mouseIsHoveringNode(CCNode* node, CCPoint const& mousePosition) {
        auto pos = node->getParent()->convertToWorldSpace(node->getPosition());
        auto size = node->getScaledContentSize();

        auto rect = CCRect {
            pos.x - size.width / 2,
            pos.y - size.height / 2,
            size.width,
            size.height
        };

        return rect.containsPoint(mousePosition);
    }

    bool touchedLockButton(CCTouch* touch, CCEvent* event) {
        auto delegate = static_cast<InputScaleDelegate*>(this->m_scaleControl->getChildByID("input-delegate"_spr));
        auto input = static_cast<CCTextInputNode*>(this->m_scaleControl->getChildByID("scale-input"_spr));
        if (!delegate) {
            log::info("no delegate");
            return false;
        }
        if (m_scaleControl == nullptr || !m_scaleControl->isVisible()) {
            log::info("scale control is hidden");
            return false;
        }

        if (this->mouseIsHoveringNode(delegate->m_absoluteLock, touch->getLocation())) {
            log::info("clicked absolute lock");
            delegate->m_absoluteLock->selected();
            m_fields->m_isTouchingAbsoluteLock = true;
            return true;
        }

        if (this->mouseIsHoveringNode(delegate->m_snapLock, touch->getLocation())) {
            log::info("clicked snap lock");
            delegate->m_snapLock->selected();
            m_fields->m_isTouchingScaleSnap = true;
            return true;
        }

        return false;
    }

    bool touchedScaleInput(CCTouch* touch, CCEvent* event) {
        auto input = static_cast<CCTextInputNode*>(this->m_scaleControl->getChildByID("scale-input"_spr));
        if (!m_scaleControl->isVisible()) {
            if (input) {
                input->getTextField()->detachWithIME();
            }
            return false;
        }

        if (!input) {
            return false;
        }

        auto nodeSize = input->getScaledContentSize();
        nodeSize.width *= 2.0f;
        nodeSize.height *= 1.5f;

        auto inputRect = CCRect {
            m_scaleControl->getPositionX() + input->getPositionX() - nodeSize.width / 2,
            (m_scaleControl->getPositionY() + input->getPositionY() - nodeSize.height / 2) + 12.5f,
            nodeSize.width,
            nodeSize.height
        };

        auto pos = GameManager::sharedState()
            ->getEditorLayer()
            ->getObjectLayer()
            ->convertTouchToNodeSpace(touch);
        
        if (inputRect.containsPoint(pos)) {
            input->getTextField()->attachWithIME();
            return true;
        } 

        input->getTextField()->detachWithIME();
        return false;
    }
};
