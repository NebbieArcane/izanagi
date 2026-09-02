#pragma once

#include "mud_editor_fields.hpp"
#include "nebbie/world.hpp"

#include <QWidget>

class QListWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QTabWidget;

namespace nebbie::qt {
class MudColorTextEdit;
}

class WorldDataEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit WorldDataEditorWidget(QWidget* parent = nullptr);

    void setWorld(nebbie::World* world);
    void refresh();

    void selectShop(long vnum);

    void setMaxLineLength(int max_length);
    void setShowColorCodes(bool show);
    nebbie::qt::MudColorTextEdit* focusedMudField() const;

signals:
    void modified();

private slots:
    void onShopSelected();
    void onSpecialSelected();
    void onDamageSelected();
    void onSocialSelected();
    void onPoseSelected();
    void onGuildSelected();
    void applyShop();
    void applySpecial();
    void applyDamage();
    void applySocial();
    void applyPose();
    void applyGuild();

private:
    void buildShopTab(QWidget* parent);
    void buildSpecialTab(QWidget* parent);
    void buildDamageTab(QWidget* parent);
    void buildSocialTab(QWidget* parent);
    void buildPoseTab(QWidget* parent);
    void buildGuildTab(QWidget* parent);
    void refreshSpecialProcedureChoices();
    void applyMudFieldSettings();
    nebbie::qt::MudFieldList mudFields() const;

    nebbie::Shop* findShop(long vnum);

    nebbie::World* world_ = nullptr;
    int max_line_length_ = 0;
    bool show_color_codes_ = false;

    QTabWidget* tabs_ = nullptr;

    QListWidget* shop_list_ = nullptr;
    QSpinBox* shop_vnum_ = nullptr;
    QSpinBox* shop_keeper_ = nullptr;
    QSpinBox* shop_room_ = nullptr;
    QSpinBox* shop_with_who_ = nullptr;
    QDoubleSpinBox* shop_profit_buy_ = nullptr;
    QDoubleSpinBox* shop_profit_sell_ = nullptr;
    QSpinBox* shop_producing_[5] = {};
    QSpinBox* shop_trade_[5] = {};
    QSpinBox* shop_temper1_ = nullptr;
    QSpinBox* shop_temper2_ = nullptr;
    QSpinBox* shop_open1_ = nullptr;
    QSpinBox* shop_close1_ = nullptr;
    QSpinBox* shop_open2_ = nullptr;
    QSpinBox* shop_close2_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_msg_buy_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_msg_sell_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_no_item1_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_no_item2_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_no_buy_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_no_cash1_ = nullptr;
    nebbie::qt::MudColorTextEdit* shop_no_cash2_ = nullptr;

    QListWidget* special_list_ = nullptr;
    QComboBox* special_type_ = nullptr;
    QSpinBox* special_vnum_ = nullptr;
    QComboBox* special_procedure_ = nullptr;
    QLineEdit* special_params_ = nullptr;

    QListWidget* damage_list_ = nullptr;
    QSpinBox* damage_attack_type_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_die_attacker_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_die_victim_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_die_room_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_miss_attacker_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_miss_victim_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_miss_room_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_hit_attacker_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_hit_victim_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_hit_room_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_god_attacker_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_god_victim_ = nullptr;
    nebbie::qt::MudColorTextEdit* damage_god_room_ = nullptr;

    QListWidget* social_list_ = nullptr;
    QSpinBox* social_act_nr_ = nullptr;
    QSpinBox* social_hide_ = nullptr;
    QSpinBox* social_min_pos_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_char_no_arg_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_others_no_arg_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_char_found_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_others_found_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_vict_found_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_not_found_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_char_auto_ = nullptr;
    nebbie::qt::MudColorTextEdit* social_others_auto_ = nullptr;

    QListWidget* pose_list_ = nullptr;
    QSpinBox* pose_level_ = nullptr;
    nebbie::qt::MudColorTextEdit* pose_poser_[4] = {};
    nebbie::qt::MudColorTextEdit* pose_room_[4] = {};

    QListWidget* guild_list_ = nullptr;
    nebbie::qt::MudColorTextEdit* guild_name_ = nullptr;
    QSpinBox* guild_guard_mob_ = nullptr;
    QSpinBox* guild_guard_room_ = nullptr;
    QSpinBox* guild_guard_dir_ = nullptr;
    QSpinBox* guild_banker_mob_ = nullptr;
    QSpinBox* guild_bank_room_ = nullptr;
    QSpinBox* guild_banker_xp_mob_ = nullptr;
    QSpinBox* guild_bank_xp_room_ = nullptr;
    QSpinBox* guild_member_book_ = nullptr;
};
