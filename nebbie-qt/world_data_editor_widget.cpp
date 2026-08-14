#include "world_data_editor_widget.hpp"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QString textFromEdit(const QTextEdit* field) {
    return field->toPlainText();
}

void setTextEdit(QTextEdit* field, const std::string& value) {
    field->setPlainText(QString::fromStdString(value));
}

QWidget* wrapScroll(QWidget* content) {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    return scroll;
}

QHBoxLayout* makeListEditorRow(QListWidget* list, QWidget* editor, const int list_width = 220) {
    list->setMinimumWidth(list_width);
    auto* row = new QHBoxLayout;
    row->addWidget(list);
    row->addWidget(editor, 1);
    return row;
}

} // namespace

WorldDataEditorWidget::WorldDataEditorWidget(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->addWidget(new QLabel(
        "Tabelle monolitiche myst.shp / myst.spe / myst.dam / myst.act / myst.pos / myst.gui. "
        "Le modifiche si applicano in memoria; usa Salva per scrivere i file."));
    tabs_ = new QTabWidget;
    buildShopTab(tabs_);
    buildSpecialTab(tabs_);
    buildDamageTab(tabs_);
    buildSocialTab(tabs_);
    buildPoseTab(tabs_);
    buildGuildTab(tabs_);
    root->addWidget(tabs_, 1);
}

void WorldDataEditorWidget::setWorld(nebbie::World* world) {
    world_ = world;
    refresh();
}

void WorldDataEditorWidget::refresh() {
    shop_list_->clear();
    special_list_->clear();
    damage_list_->clear();
    social_list_->clear();
    pose_list_->clear();
    guild_list_->clear();
    if (!world_) {
        return;
    }

    for (const auto& shop : world_->shops) {
        shop_list_->addItem(QString("#%1 keeper=%2 room=%3")
                                .arg(shop.vnum)
                                .arg(shop.keeper)
                                .arg(shop.in_room));
        shop_list_->item(shop_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(shop.vnum));
    }
    for (std::size_t i = 0; i < world_->special_procs.size(); ++i) {
        const auto& spe = world_->special_procs[i];
        special_list_->addItem(QString("%1 %2 %3")
                                   .arg(QChar(spe.type))
                                   .arg(spe.vnum)
                                   .arg(QString::fromStdString(spe.procedure)));
        special_list_->item(special_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
    for (std::size_t i = 0; i < world_->damage_messages.size(); ++i) {
        const auto& msg = world_->damage_messages[i];
        damage_list_->addItem(QString("attack_type %1").arg(msg.attack_type));
        damage_list_->item(damage_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
    for (std::size_t i = 0; i < world_->social_messages.size(); ++i) {
        const auto& msg = world_->social_messages[i];
        social_list_->addItem(QString("act_nr %1").arg(msg.act_nr));
        social_list_->item(social_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
    for (std::size_t i = 0; i < world_->pose_entries.size(); ++i) {
        const auto& pose = world_->pose_entries[i];
        pose_list_->addItem(QString("level %1").arg(pose.level));
        pose_list_->item(pose_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
    for (std::size_t i = 0; i < world_->guilds.size(); ++i) {
        const auto& guild = world_->guilds[i];
        guild_list_->addItem(QString::fromStdString(guild.base_filename));
        guild_list_->item(guild_list_->count() - 1)->setData(Qt::UserRole, static_cast<qlonglong>(i));
    }
}

void WorldDataEditorWidget::selectShop(const long vnum) {
    for (int row = 0; row < shop_list_->count(); ++row) {
        if (shop_list_->item(row)->data(Qt::UserRole).toLongLong() == vnum) {
            tabs_->setCurrentIndex(0);
            shop_list_->setCurrentRow(row);
            return;
        }
    }
}

void WorldDataEditorWidget::buildShopTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    shop_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    shop_vnum_ = new QSpinBox;
    shop_vnum_->setRange(0, 999999);
    shop_keeper_ = new QSpinBox;
    shop_keeper_->setRange(0, 999999);
    shop_room_ = new QSpinBox;
    shop_room_->setRange(0, 999999);
    shop_with_who_ = new QSpinBox;
    shop_with_who_->setRange(0, 999999);
    shop_profit_buy_ = new QDoubleSpinBox;
    shop_profit_buy_->setRange(0.0, 100.0);
    shop_profit_buy_->setDecimals(2);
    shop_profit_sell_ = new QDoubleSpinBox;
    shop_profit_sell_->setRange(0.0, 100.0);
    shop_profit_sell_->setDecimals(2);
    for (int i = 0; i < 5; ++i) {
        shop_producing_[i] = new QSpinBox;
        shop_producing_[i]->setRange(0, 999999);
        shop_trade_[i] = new QSpinBox;
        shop_trade_[i]->setRange(0, 999999);
    }
    shop_temper1_ = new QSpinBox;
    shop_temper1_->setRange(-1, 999);
    shop_temper2_ = new QSpinBox;
    shop_temper2_->setRange(-1, 999);
    shop_open1_ = new QSpinBox;
    shop_open1_->setRange(0, 24);
    shop_close1_ = new QSpinBox;
    shop_close1_->setRange(0, 24);
    shop_open2_ = new QSpinBox;
    shop_open2_->setRange(0, 24);
    shop_close2_ = new QSpinBox;
    shop_close2_->setRange(0, 24);
    shop_msg_buy_ = new QLineEdit;
    shop_msg_sell_ = new QLineEdit;
    shop_no_item1_ = new QLineEdit;
    shop_no_item2_ = new QLineEdit;
    shop_no_buy_ = new QLineEdit;
    shop_no_cash1_ = new QLineEdit;
    shop_no_cash2_ = new QLineEdit;
    form->addRow("Vnum:", shop_vnum_);
    form->addRow("Keeper (mob):", shop_keeper_);
    form->addRow("Room:", shop_room_);
    form->addRow("With who:", shop_with_who_);
    form->addRow("Profit buy:", shop_profit_buy_);
    form->addRow("Profit sell:", shop_profit_sell_);
    for (int i = 0; i < 5; ++i) {
        form->addRow(QString("Producing %1:").arg(i + 1), shop_producing_[i]);
        form->addRow(QString("Trade type %1:").arg(i + 1), shop_trade_[i]);
    }
    form->addRow("Temper 1/2:", shop_temper1_);
    form->addRow("", shop_temper2_);
    form->addRow("Open/Close 1:", shop_open1_);
    form->addRow("", shop_close1_);
    form->addRow("Open/Close 2:", shop_open2_);
    form->addRow("", shop_close2_);
    form->addRow("Message buy:", shop_msg_buy_);
    form->addRow("Message sell:", shop_msg_sell_);
    form->addRow("No such item 1:", shop_no_item1_);
    form->addRow("No such item 2:", shop_no_item2_);
    form->addRow("Do not buy:", shop_no_buy_);
    form->addRow("Missing cash 1:", shop_no_cash1_);
    form->addRow("Missing cash 2:", shop_no_cash2_);
    auto* apply = new QPushButton("Applica negozio");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyShop);
    layout->addLayout(makeListEditorRow(shop_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(shop_list_, &QListWidget::currentRowChanged, this, [this](int) { onShopSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Negozi");
}

void WorldDataEditorWidget::buildSpecialTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    special_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    special_type_ = new QComboBox;
    special_type_->addItem("m (mobile)", QVariant(QChar('m')));
    special_type_->addItem("o (oggetto)", QVariant(QChar('o')));
    special_type_->addItem("r (stanza)", QVariant(QChar('r')));
    special_vnum_ = new QSpinBox;
    special_vnum_->setRange(0, 999999);
    special_procedure_ = new QLineEdit;
    special_params_ = new QLineEdit;
    form->addRow("Tipo:", special_type_);
    form->addRow("Vnum:", special_vnum_);
    form->addRow("Procedure:", special_procedure_);
    form->addRow("Params:", special_params_);
    auto* apply = new QPushButton("Applica special proc");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applySpecial);
    layout->addLayout(makeListEditorRow(special_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(special_list_, &QListWidget::currentRowChanged, this, [this](int) { onSpecialSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Special");
}

void WorldDataEditorWidget::buildDamageTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    damage_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    damage_attack_type_ = new QSpinBox;
    damage_attack_type_->setRange(0, 999);
    damage_die_attacker_ = new QTextEdit;
    damage_die_victim_ = new QTextEdit;
    damage_die_room_ = new QTextEdit;
    damage_miss_attacker_ = new QTextEdit;
    damage_miss_victim_ = new QTextEdit;
    damage_miss_room_ = new QTextEdit;
    damage_hit_attacker_ = new QTextEdit;
    damage_hit_victim_ = new QTextEdit;
    damage_hit_room_ = new QTextEdit;
    damage_god_attacker_ = new QTextEdit;
    damage_god_victim_ = new QTextEdit;
    damage_god_room_ = new QTextEdit;
    for (QTextEdit* field : {damage_die_attacker_, damage_die_victim_, damage_die_room_, damage_miss_attacker_,
                            damage_miss_victim_, damage_miss_room_, damage_hit_attacker_, damage_hit_victim_,
                            damage_hit_room_, damage_god_attacker_, damage_god_victim_, damage_god_room_}) {
        field->setMaximumHeight(72);
    }
    form->addRow("Attack type:", damage_attack_type_);
    form->addRow("Die attacker:", damage_die_attacker_);
    form->addRow("Die victim:", damage_die_victim_);
    form->addRow("Die room:", damage_die_room_);
    form->addRow("Miss attacker:", damage_miss_attacker_);
    form->addRow("Miss victim:", damage_miss_victim_);
    form->addRow("Miss room:", damage_miss_room_);
    form->addRow("Hit attacker:", damage_hit_attacker_);
    form->addRow("Hit victim:", damage_hit_victim_);
    form->addRow("Hit room:", damage_hit_room_);
    form->addRow("God attacker:", damage_god_attacker_);
    form->addRow("God victim:", damage_god_victim_);
    form->addRow("God room:", damage_god_room_);
    auto* apply = new QPushButton("Applica messaggi danno");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyDamage);
    layout->addLayout(makeListEditorRow(damage_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(damage_list_, &QListWidget::currentRowChanged, this, [this](int) { onDamageSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Danni");
}

void WorldDataEditorWidget::buildSocialTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    social_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    social_act_nr_ = new QSpinBox;
    social_act_nr_->setRange(0, 9999);
    social_hide_ = new QSpinBox;
    social_hide_->setRange(0, 1);
    social_min_pos_ = new QSpinBox;
    social_min_pos_->setRange(0, 15);
    social_char_no_arg_ = new QTextEdit;
    social_others_no_arg_ = new QTextEdit;
    social_char_found_ = new QTextEdit;
    social_others_found_ = new QTextEdit;
    social_vict_found_ = new QTextEdit;
    social_not_found_ = new QTextEdit;
    social_char_auto_ = new QTextEdit;
    social_others_auto_ = new QTextEdit;
    for (QTextEdit* field :
         {social_char_no_arg_, social_others_no_arg_, social_char_found_, social_others_found_, social_vict_found_,
          social_not_found_, social_char_auto_, social_others_auto_}) {
        field->setMaximumHeight(64);
    }
    form->addRow("act_nr:", social_act_nr_);
    form->addRow("hide:", social_hide_);
    form->addRow("min_victim_position:", social_min_pos_);
    form->addRow("char_no_arg:", social_char_no_arg_);
    form->addRow("others_no_arg:", social_others_no_arg_);
    form->addRow("char_found:", social_char_found_);
    form->addRow("others_found:", social_others_found_);
    form->addRow("vict_found:", social_vict_found_);
    form->addRow("not_found:", social_not_found_);
    form->addRow("char_auto:", social_char_auto_);
    form->addRow("others_auto:", social_others_auto_);
    auto* apply = new QPushButton("Applica social");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applySocial);
    layout->addLayout(makeListEditorRow(social_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(social_list_, &QListWidget::currentRowChanged, this, [this](int) { onSocialSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Social");
}

void WorldDataEditorWidget::buildPoseTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    pose_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    pose_level_ = new QSpinBox;
    pose_level_->setRange(0, 60);
    for (int i = 0; i < 4; ++i) {
        pose_poser_[i] = new QTextEdit;
        pose_room_[i] = new QTextEdit;
        pose_poser_[i]->setMaximumHeight(56);
        pose_room_[i]->setMaximumHeight(56);
        form->addRow(QString("Poser class %1:").arg(i), pose_poser_[i]);
        form->addRow(QString("Room class %1:").arg(i), pose_room_[i]);
    }
    form->addRow("Level:", pose_level_);
    auto* apply = new QPushButton("Applica pose");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyPose);
    layout->addLayout(makeListEditorRow(pose_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(pose_list_, &QListWidget::currentRowChanged, this, [this](int) { onPoseSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Pose");
}

void WorldDataEditorWidget::buildGuildTab(QWidget* parent) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    guild_list_ = new QListWidget;
    auto* form_host = new QWidget;
    auto* form = new QFormLayout(form_host);
    guild_name_ = new QLineEdit;
    guild_guard_mob_ = new QSpinBox;
    guild_guard_mob_->setRange(0, 999999);
    guild_guard_room_ = new QSpinBox;
    guild_guard_room_->setRange(0, 999999);
    guild_guard_dir_ = new QSpinBox;
    guild_guard_dir_->setRange(0, 5);
    guild_banker_mob_ = new QSpinBox;
    guild_banker_mob_->setRange(0, 999999);
    guild_bank_room_ = new QSpinBox;
    guild_bank_room_->setRange(0, 999999);
    guild_banker_xp_mob_ = new QSpinBox;
    guild_banker_xp_mob_->setRange(0, 999999);
    guild_bank_xp_room_ = new QSpinBox;
    guild_bank_xp_room_->setRange(0, 999999);
    guild_member_book_ = new QSpinBox;
    guild_member_book_->setRange(0, 999999);
    form->addRow("Nome file:", guild_name_);
    form->addRow("Guard mob:", guild_guard_mob_);
    form->addRow("Guard room:", guild_guard_room_);
    form->addRow("Guard dir:", guild_guard_dir_);
    form->addRow("Banker mob:", guild_banker_mob_);
    form->addRow("Bank room:", guild_bank_room_);
    form->addRow("XP banker mob:", guild_banker_xp_mob_);
    form->addRow("XP bank room:", guild_bank_xp_room_);
    form->addRow("Member book obj:", guild_member_book_);
    auto* apply = new QPushButton("Applica gilda");
    connect(apply, &QPushButton::clicked, this, &WorldDataEditorWidget::applyGuild);
    layout->addLayout(makeListEditorRow(guild_list_, wrapScroll(form_host)));
    layout->addWidget(apply);
    connect(guild_list_, &QListWidget::currentRowChanged, this, [this](int) { onGuildSelected(); });
    static_cast<QTabWidget*>(parent)->addTab(page, "Gilde");
}

nebbie::Shop* WorldDataEditorWidget::findShop(const long vnum) {
    if (!world_) {
        return nullptr;
    }
    for (auto& shop : world_->shops) {
        if (shop.vnum == vnum) {
            return &shop;
        }
    }
    return nullptr;
}

void WorldDataEditorWidget::onShopSelected() {
    if (!world_) {
        return;
    }
    auto* item = shop_list_->currentItem();
    if (!item) {
        return;
    }
    const nebbie::Shop* shop = findShop(item->data(Qt::UserRole).toLongLong());
    if (!shop) {
        return;
    }
    shop_vnum_->setValue(static_cast<int>(shop->vnum));
    shop_keeper_->setValue(shop->keeper);
    shop_room_->setValue(shop->in_room);
    shop_with_who_->setValue(shop->with_who);
    shop_profit_buy_->setValue(shop->profit_buy);
    shop_profit_sell_->setValue(shop->profit_sell);
    for (int i = 0; i < 5; ++i) {
        shop_producing_[i]->setValue(shop->producing[i]);
        shop_trade_[i]->setValue(shop->trade_types[i]);
    }
    shop_temper1_->setValue(shop->temper1);
    shop_temper2_->setValue(shop->temper2);
    shop_open1_->setValue(shop->open1);
    shop_close1_->setValue(shop->close1);
    shop_open2_->setValue(shop->open2);
    shop_close2_->setValue(shop->close2);
    shop_msg_buy_->setText(QString::fromStdString(shop->message_buy));
    shop_msg_sell_->setText(QString::fromStdString(shop->message_sell));
    shop_no_item1_->setText(QString::fromStdString(shop->no_such_item1));
    shop_no_item2_->setText(QString::fromStdString(shop->no_such_item2));
    shop_no_buy_->setText(QString::fromStdString(shop->do_not_buy));
    shop_no_cash1_->setText(QString::fromStdString(shop->missing_cash1));
    shop_no_cash2_->setText(QString::fromStdString(shop->missing_cash2));
}

void WorldDataEditorWidget::applyShop() {
    if (!world_) {
        return;
    }
    auto* item = shop_list_->currentItem();
    if (!item) {
        return;
    }
    nebbie::Shop* shop = findShop(item->data(Qt::UserRole).toLongLong());
    if (!shop) {
        return;
    }
    shop->vnum = shop_vnum_->value();
    shop->keeper = shop_keeper_->value();
    shop->in_room = shop_room_->value();
    shop->with_who = shop_with_who_->value();
    shop->profit_buy = static_cast<float>(shop_profit_buy_->value());
    shop->profit_sell = static_cast<float>(shop_profit_sell_->value());
    for (int i = 0; i < 5; ++i) {
        shop->producing[i] = shop_producing_[i]->value();
        shop->trade_types[i] = shop_trade_[i]->value();
    }
    shop->temper1 = shop_temper1_->value();
    shop->temper2 = shop_temper2_->value();
    shop->open1 = shop_open1_->value();
    shop->close1 = shop_close1_->value();
    shop->open2 = shop_open2_->value();
    shop->close2 = shop_close2_->value();
    shop->message_buy = shop_msg_buy_->text().toStdString();
    shop->message_sell = shop_msg_sell_->text().toStdString();
    shop->no_such_item1 = shop_no_item1_->text().toStdString();
    shop->no_such_item2 = shop_no_item2_->text().toStdString();
    shop->do_not_buy = shop_no_buy_->text().toStdString();
    shop->missing_cash1 = shop_no_cash1_->text().toStdString();
    shop->missing_cash2 = shop_no_cash2_->text().toStdString();
    item->setData(Qt::UserRole, static_cast<qlonglong>(shop->vnum));
    item->setText(QString("#%1 keeper=%2 room=%3").arg(shop->vnum).arg(shop->keeper).arg(shop->in_room));
    emit modified();
}

void WorldDataEditorWidget::onSpecialSelected() {
    if (!world_) {
        return;
    }
    auto* item = special_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->special_procs.size()) {
        return;
    }
    const auto& spe = world_->special_procs[index];
    const int type_index = special_type_->findData(QVariant(QChar(spe.type)));
    special_type_->setCurrentIndex(type_index >= 0 ? type_index : 0);
    special_vnum_->setValue(static_cast<int>(spe.vnum));
    special_procedure_->setText(QString::fromStdString(spe.procedure));
    special_params_->setText(QString::fromStdString(spe.params));
}

void WorldDataEditorWidget::applySpecial() {
    if (!world_) {
        return;
    }
    auto* item = special_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->special_procs.size()) {
        return;
    }
    auto& spe = world_->special_procs[index];
    spe.type = static_cast<char>(special_type_->currentData().toChar().unicode());
    spe.vnum = special_vnum_->value();
    spe.procedure = special_procedure_->text().toStdString();
    spe.params = special_params_->text().toStdString();
    item->setText(QString("%1 %2 %3")
                      .arg(QChar(spe.type))
                      .arg(spe.vnum)
                      .arg(QString::fromStdString(spe.procedure)));
    emit modified();
}

void WorldDataEditorWidget::onDamageSelected() {
    if (!world_) {
        return;
    }
    auto* item = damage_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->damage_messages.size()) {
        return;
    }
    const auto& msg = world_->damage_messages[index];
    damage_attack_type_->setValue(msg.attack_type);
    setTextEdit(damage_die_attacker_, msg.die_attacker);
    setTextEdit(damage_die_victim_, msg.die_victim);
    setTextEdit(damage_die_room_, msg.die_room);
    setTextEdit(damage_miss_attacker_, msg.miss_attacker);
    setTextEdit(damage_miss_victim_, msg.miss_victim);
    setTextEdit(damage_miss_room_, msg.miss_room);
    setTextEdit(damage_hit_attacker_, msg.hit_attacker);
    setTextEdit(damage_hit_victim_, msg.hit_victim);
    setTextEdit(damage_hit_room_, msg.hit_room);
    setTextEdit(damage_god_attacker_, msg.god_attacker);
    setTextEdit(damage_god_victim_, msg.god_victim);
    setTextEdit(damage_god_room_, msg.god_room);
}

void WorldDataEditorWidget::applyDamage() {
    if (!world_) {
        return;
    }
    auto* item = damage_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->damage_messages.size()) {
        return;
    }
    auto& msg = world_->damage_messages[index];
    msg.attack_type = damage_attack_type_->value();
    msg.die_attacker = textFromEdit(damage_die_attacker_).toStdString();
    msg.die_victim = textFromEdit(damage_die_victim_).toStdString();
    msg.die_room = textFromEdit(damage_die_room_).toStdString();
    msg.miss_attacker = textFromEdit(damage_miss_attacker_).toStdString();
    msg.miss_victim = textFromEdit(damage_miss_victim_).toStdString();
    msg.miss_room = textFromEdit(damage_miss_room_).toStdString();
    msg.hit_attacker = textFromEdit(damage_hit_attacker_).toStdString();
    msg.hit_victim = textFromEdit(damage_hit_victim_).toStdString();
    msg.hit_room = textFromEdit(damage_hit_room_).toStdString();
    msg.god_attacker = textFromEdit(damage_god_attacker_).toStdString();
    msg.god_victim = textFromEdit(damage_god_victim_).toStdString();
    msg.god_room = textFromEdit(damage_god_room_).toStdString();
    item->setText(QString("attack_type %1").arg(msg.attack_type));
    emit modified();
}

void WorldDataEditorWidget::onSocialSelected() {
    if (!world_) {
        return;
    }
    auto* item = social_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->social_messages.size()) {
        return;
    }
    const auto& msg = world_->social_messages[index];
    social_act_nr_->setValue(msg.act_nr);
    social_hide_->setValue(msg.hide);
    social_min_pos_->setValue(msg.min_victim_position);
    setTextEdit(social_char_no_arg_, msg.char_no_arg);
    setTextEdit(social_others_no_arg_, msg.others_no_arg);
    setTextEdit(social_char_found_, msg.char_found);
    setTextEdit(social_others_found_, msg.others_found);
    setTextEdit(social_vict_found_, msg.vict_found);
    setTextEdit(social_not_found_, msg.not_found);
    setTextEdit(social_char_auto_, msg.char_auto);
    setTextEdit(social_others_auto_, msg.others_auto);
}

void WorldDataEditorWidget::applySocial() {
    if (!world_) {
        return;
    }
    auto* item = social_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->social_messages.size()) {
        return;
    }
    auto& msg = world_->social_messages[index];
    msg.act_nr = social_act_nr_->value();
    msg.hide = social_hide_->value();
    msg.min_victim_position = social_min_pos_->value();
    msg.char_no_arg = textFromEdit(social_char_no_arg_).toStdString();
    msg.others_no_arg = textFromEdit(social_others_no_arg_).toStdString();
    msg.char_found = textFromEdit(social_char_found_).toStdString();
    msg.others_found = textFromEdit(social_others_found_).toStdString();
    msg.vict_found = textFromEdit(social_vict_found_).toStdString();
    msg.not_found = textFromEdit(social_not_found_).toStdString();
    msg.char_auto = textFromEdit(social_char_auto_).toStdString();
    msg.others_auto = textFromEdit(social_others_auto_).toStdString();
    item->setText(QString("act_nr %1").arg(msg.act_nr));
    emit modified();
}

void WorldDataEditorWidget::onPoseSelected() {
    if (!world_) {
        return;
    }
    auto* item = pose_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->pose_entries.size()) {
        return;
    }
    const auto& pose = world_->pose_entries[index];
    pose_level_->setValue(pose.level);
    for (int i = 0; i < 4; ++i) {
        setTextEdit(pose_poser_[i], pose.poser_msg[i]);
        setTextEdit(pose_room_[i], pose.room_msg[i]);
    }
}

void WorldDataEditorWidget::applyPose() {
    if (!world_) {
        return;
    }
    auto* item = pose_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->pose_entries.size()) {
        return;
    }
    auto& pose = world_->pose_entries[index];
    pose.level = pose_level_->value();
    for (int i = 0; i < 4; ++i) {
        pose.poser_msg[i] = textFromEdit(pose_poser_[i]).toStdString();
        pose.room_msg[i] = textFromEdit(pose_room_[i]).toStdString();
    }
    item->setText(QString("level %1").arg(pose.level));
    emit modified();
}

void WorldDataEditorWidget::onGuildSelected() {
    if (!world_) {
        return;
    }
    auto* item = guild_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->guilds.size()) {
        return;
    }
    const auto& guild = world_->guilds[index];
    guild_name_->setText(QString::fromStdString(guild.base_filename));
    guild_guard_mob_->setValue(guild.guard_mob);
    guild_guard_room_->setValue(guild.guard_room);
    guild_guard_dir_->setValue(guild.guard_dir);
    guild_banker_mob_->setValue(guild.banker_mob);
    guild_bank_room_->setValue(guild.bank_room);
    guild_banker_xp_mob_->setValue(guild.banker_xp_mob);
    guild_bank_xp_room_->setValue(guild.bank_xp_room);
    guild_member_book_->setValue(guild.member_book_obj);
}

void WorldDataEditorWidget::applyGuild() {
    if (!world_) {
        return;
    }
    auto* item = guild_list_->currentItem();
    if (!item) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(item->data(Qt::UserRole).toLongLong());
    if (index >= world_->guilds.size()) {
        return;
    }
    auto& guild = world_->guilds[index];
    guild.base_filename = guild_name_->text().toStdString();
    guild.guard_mob = guild_guard_mob_->value();
    guild.guard_room = guild_guard_room_->value();
    guild.guard_dir = guild_guard_dir_->value();
    guild.banker_mob = guild_banker_mob_->value();
    guild.bank_room = guild_bank_room_->value();
    guild.banker_xp_mob = guild_banker_xp_mob_->value();
    guild.bank_xp_room = guild_bank_xp_room_->value();
    guild.member_book_obj = guild_member_book_->value();
    item->setText(QString::fromStdString(guild.base_filename));
    emit modified();
}
