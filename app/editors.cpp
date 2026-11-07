#include "editors.hpp"

#include "flag_editor.hpp"
#include "line_limited_edit.hpp"

#include "eleutheria/world.hpp"

#include <algorithm>
#include <cstdio>
#include <map>

#include <QAbstractItemView>
#include <QComboBox>
#include <QStringList>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QSpinBox* make_spin(QWidget* parent, int minv, int maxv) {
  auto* s = new QSpinBox(parent);
  s->setRange(minv, maxv);
  return s;
}

void fill_named(QComboBox* box, const std::vector<eleutheria::NamedValue>& values) {
  box->clear();
  for(const auto& v : values) {
    box->addItem(QString::fromStdString(v.label), v.value);
  }
}

int combo_value(QComboBox* box) {
  return box->currentData().toInt();
}

void set_combo_value(QComboBox* box, int value) {
  const int idx = box->findData(value);
  if(idx >= 0) {
    box->setCurrentIndex(idx);
  } else {
    box->addItem(QString::number(value), value);
    box->setCurrentIndex(box->count() - 1);
  }
}

}  // namespace

namespace {

const char* kEquipPositions[] = {
    "Special",        "Right finger",   "Left finger",    "Neck 1",
    "Neck 2",         "Body",           "Head",           "Legs",
    "Feet",           "Hands",          "Arms",           "Shield",
    "About",          "Waist",          "Right wrist",    "Left wrist",
    "Wield",          "Hold",           "Back",           "Right ear",
    "Left ear",       "Eyes",           "Loaded weapon",  "Clan symbol",
};

std::optional<std::int64_t> pick_entity_vnum(QWidget* parent, const QString& title,
                                            const std::map<std::int64_t, QString>& choices) {
  if(choices.empty()) {
    QMessageBox::information(parent, title, QStringLiteral("Nessuna entita' disponibile."));
    return std::nullopt;
  }
  QStringList labels;
  QList<std::int64_t> vnums;
  for(const auto& [vnum, label] : choices) {
    labels << QStringLiteral("%1  %2").arg(vnum, 6).arg(label);
    vnums << vnum;
  }
  bool ok = false;
  const QString picked = QInputDialog::getItem(parent, title, QStringLiteral("Scegli:"), labels, 0,
                                               false, &ok);
  if(!ok || picked.isEmpty()) {
    return std::nullopt;
  }
  const int idx = labels.indexOf(picked);
  if(idx < 0 || idx >= vnums.size()) {
    return std::nullopt;
  }
  return vnums[idx];
}

}  // namespace

namespace {

const char* kDirFrom[] = {"North from", "East from", "South from",
                          "West from", "Up from", "Down from"};

}  // namespace

RoomEditor::RoomEditor(const eleutheria::Catalog& catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  tabs_ = new QTabWidget(this);
  root->addWidget(tabs_);

  // --- Summary (stile SMAUG Area Editor) ---
  auto* summary_page = new QWidget(tabs_);
  auto* summary_layout = new QVBoxLayout(summary_page);

  auto* head = new QFormLayout();
  vnum_ = make_spin(summary_page, 0, 999999);
  name_ = new QLineEdit(summary_page);
  flags_summary_ = new QLineEdit(summary_page);
  flags_summary_->setReadOnly(true);
  head->addRow(QStringLiteral("Vnum"), vnum_);
  head->addRow(QStringLiteral("Type"), new QLabel(QStringLiteral("In a room"), summary_page));
  head->addRow(QStringLiteral("Name"), name_);
  head->addRow(QStringLiteral("Flags"), flags_summary_);
  summary_layout->addLayout(head);

  summary_layout->addWidget(new QLabel(QStringLiteral("Entered from"), summary_page));
  auto* entered_row = new QHBoxLayout();
  entered_from_list_ = new QListWidget(summary_page);
  entered_from_list_->setFont(QFont(QStringLiteral("Menlo"), 11));
  entered_from_list_->setMaximumHeight(110);
  entered_row->addWidget(entered_from_list_, 1);
  goto_entered_btn_ = new QPushButton(QStringLiteral("Go to"), summary_page);
  entered_row->addWidget(goto_entered_btn_, 0, Qt::AlignTop);
  summary_layout->addLayout(entered_row);

  summary_layout->addWidget(new QLabel(QStringLiteral("References"), summary_page));
  auto* refs_row = new QHBoxLayout();
  contents_list_ = new QListWidget(summary_page);
  contents_list_->setFont(QFont(QStringLiteral("Menlo"), 11));
  contents_list_->setAlternatingRowColors(true);
  refs_row->addWidget(contents_list_, 1);

  auto* btns = new QVBoxLayout;
  goto_ref_btn_ = new QPushButton(QStringLiteral("Go to"), summary_page);
  add_mob_btn_ = new QPushButton(QStringLiteral("Add mob..."), summary_page);
  add_obj_btn_ = new QPushButton(QStringLiteral("Add object..."), summary_page);
  remove_btn_ = new QPushButton(QStringLiteral("Remove..."), summary_page);
  equip_btn_ = new QPushButton(QStringLiteral("Equip..."), summary_page);
  give_btn_ = new QPushButton(QStringLiteral("Give..."), summary_page);
  for(auto* b : {goto_ref_btn_, add_mob_btn_, add_obj_btn_, remove_btn_, equip_btn_, give_btn_}) {
    btns->addWidget(b);
  }
  btns->addStretch();
  refs_row->addLayout(btns);
  summary_layout->addLayout(refs_row, 1);
  tabs_->addTab(summary_page, QStringLiteral("Summary"));

  // --- Dettaglio stanza ---
  auto* room_page = new QWidget(tabs_);
  auto* room_layout = new QVBoxLayout(room_page);
  auto* form = new QFormLayout();
  zone_hint_ = make_spin(room_page, -1, 999999);
  sector_ = new QComboBox(room_page);
  fill_named(sector_, catalog_.sectors());
  form->addRow(QStringLiteral("Zone hint"), zone_hint_);
  form->addRow(QStringLiteral("Settore"), sector_);
  room_layout->addLayout(form);

  description_ = new LineLimitedPlainTextEdit(room_page);
  configure_mud_text_field(description_);
  description_->setPlaceholderText(QStringLiteral("Descrizione stanza"));
  room_layout->addWidget(new QLabel(QStringLiteral("Descrizione"), room_page));
  room_layout->addWidget(description_, 2);

  flags_ = new FlagEditor(room_page);
  flags_->setFlags(catalog_.room_flags());
  room_layout->addWidget(new QLabel(QStringLiteral("Room flags"), room_page));
  room_layout->addWidget(flags_);

  exits_ = new QTableWidget(6, 5, room_page);
  exits_->setHorizontalHeaderLabels(
      {QStringLiteral("Dir"), QStringLiteral("To"), QStringLiteral("Flags"), QStringLiteral("Key"),
       QStringLiteral("Keywords")});
  exits_->verticalHeader()->setVisible(false);
  exits_->horizontalHeader()->setStretchLastSection(true);
  const char* dirs[] = {"N", "E", "S", "W", "U", "D"};
  for(int i = 0; i < 6; ++i) {
    exits_->setItem(i, 0, new QTableWidgetItem(QString::fromUtf8(dirs[i])));
    exits_->item(i, 0)->setFlags(Qt::ItemIsEnabled);
    for(int c = 1; c < 5; ++c) {
      exits_->setItem(i, c, new QTableWidgetItem);
    }
  }
  room_layout->addWidget(new QLabel(QStringLiteral("Uscite"), room_page));
  room_layout->addWidget(exits_);

  extras_ = new LineLimitedPlainTextEdit(room_page);
  configure_mud_text_field(extras_);
  extras_->setPlaceholderText(QStringLiteral("Extra desc: keywords|description  (una per riga)"));
  room_layout->addWidget(new QLabel(QStringLiteral("Extra descriptions"), room_page));
  room_layout->addWidget(extras_);
  tabs_->addTab(room_page, QStringLiteral("Dettaglio"));

  connect(add_mob_btn_, &QPushButton::clicked, this, &RoomEditor::addMobReset);
  connect(add_obj_btn_, &QPushButton::clicked, this, &RoomEditor::addObjectReset);
  connect(equip_btn_, &QPushButton::clicked, this, &RoomEditor::equipSelectedMob);
  connect(give_btn_, &QPushButton::clicked, this, &RoomEditor::giveSelectedMob);
  connect(remove_btn_, &QPushButton::clicked, this, &RoomEditor::removeSelectedReset);
  connect(goto_entered_btn_, &QPushButton::clicked, this, &RoomEditor::goToEnteredFrom);
  connect(goto_ref_btn_, &QPushButton::clicked, this, &RoomEditor::goToReference);
  connect(entered_from_list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem*) { goToEnteredFrom(); });
  connect(contents_list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem*) { goToReference(); });

  auto notify = [this]() { emit edited(); };
  connect(vnum_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(description_, &QPlainTextEdit::textChanged, this, notify);
  connect(flags_, &FlagEditor::valueChanged, this, [this, notify]() {
    flags_summary_->setText(flagNames(flags_->value()));
    notify();
  });
}

void RoomEditor::setMaxLineLength(int max_length) {
  if(description_ != nullptr) {
    description_->setMaxLineLength(max_length);
  }
  if(extras_ != nullptr) {
    extras_->setMaxLineLength(max_length);
  }
}

void RoomEditor::setArea(eleutheria::Area* area) { area_ = area; }

void RoomEditor::clear() {
  eleutheria::Room empty;
  load(empty);
}

QString RoomEditor::flagNames(std::uint64_t bits) const {
  QStringList names;
  for(const auto& flag : catalog_.room_flags()) {
    const auto mask = eleutheria::Catalog::bit(flag.bit_index);
    if((bits & mask) != 0) {
      names << QString::fromStdString(flag.key.empty() ? flag.label : flag.key);
    }
  }
  return names.join(QLatin1Char(' '));
}

QString RoomEditor::entityLabel(char kind, std::int64_t vnum) const {
  if(area_ == nullptr) {
    return QStringLiteral("?");
  }
  if(kind == 'M') {
    const auto it = area_->mobiles.find(vnum);
    if(it == area_->mobiles.end()) {
      return QStringLiteral("?");
    }
    return QString::fromStdString(eleutheria::strip_mud_colors(
        it->second.short_descr.empty() ? it->second.name : it->second.short_descr));
  }
  if(kind == 'O') {
    const auto it = area_->objects.find(vnum);
    if(it == area_->objects.end()) {
      return QStringLiteral("?");
    }
    return QString::fromStdString(eleutheria::strip_mud_colors(
        it->second.short_description.empty() ? it->second.name : it->second.short_description));
  }
  if(kind == 'R') {
    const auto it = area_->rooms.find(vnum);
    if(it == area_->rooms.end()) {
      return QStringLiteral("?");
    }
    return QString::fromStdString(eleutheria::strip_mud_colors(it->second.name));
  }
  return QStringLiteral("?");
}

std::size_t RoomEditor::insertAfterMobChain(eleutheria::Zone& zone, std::size_t mob_index) const {
  std::size_t at = mob_index + 1;
  while(at < zone.commands.size()) {
    const char c = zone.commands[at].command;
    if(c == 'E' || c == 'G' || c == 'P') {
      ++at;
      continue;
    }
    break;
  }
  return at;
}

void RoomEditor::load(const eleutheria::Room& room) {
  loaded_vnum_ = room.vnum;
  vnum_->blockSignals(true);
  name_->blockSignals(true);
  description_->blockSignals(true);
  const QSignalBlocker bflags(flags_);
  vnum_->setValue(static_cast<int>(room.vnum));
  name_->setText(QString::fromStdString(eleutheria::strip_mud_colors(room.name)));
  description_->setPlainText(QString::fromStdString(room.description));
  zone_hint_->setValue(static_cast<int>(room.zone_hint));
  flags_->setValue(static_cast<std::uint64_t>(room.room_flags));
  flags_summary_->setText(flagNames(static_cast<std::uint64_t>(room.room_flags)));
  set_combo_value(sector_, static_cast<int>(room.sector_type));
  for(int i = 0; i < 6; ++i) {
    for(int c = 1; c < 5; ++c) {
      if(exits_->item(i, c) == nullptr) {
        exits_->setItem(i, c, new QTableWidgetItem);
      }
    }
    const auto& ex = room.exits[static_cast<std::size_t>(i)];
    if(!ex.present) {
      exits_->item(i, 1)->setText({});
      exits_->item(i, 2)->setText({});
      exits_->item(i, 3)->setText({});
      exits_->item(i, 4)->setText({});
      continue;
    }
    exits_->item(i, 1)->setText(QString::number(ex.to_room));
    exits_->item(i, 2)->setText(QString::number(ex.exit_info));
    exits_->item(i, 3)->setText(QString::number(ex.key));
    exits_->item(i, 4)->setText(QString::fromStdString(ex.keyword));
  }
  vnum_->blockSignals(false);
  name_->blockSignals(false);
  description_->blockSignals(false);
  QString extra_text;
  for(const auto& ed : room.extras) {
    extra_text += QString::fromStdString(ed.keywords) + "|" +
                  QString::fromStdString(ed.description).replace('\n', "\\n") + "\n";
  }
  extras_->setPlainText(extra_text);
  rebuildSummary();
}

void RoomEditor::rebuildSummary() {
  entered_from_list_->clear();
  contents_list_->clear();
  if(area_ == nullptr || loaded_vnum_ < 0) {
    return;
  }

  for(const auto& [from_vnum, room] : area_->rooms) {
    for(int d = 0; d < 6; ++d) {
      const auto& ex = room.exits[static_cast<std::size_t>(d)];
      if(!ex.present || ex.to_room != loaded_vnum_) {
        continue;
      }
      auto* item = new QListWidgetItem(
          QStringLiteral("%1 #%2 (%3)")
              .arg(QString::fromUtf8(kDirFrom[d]))
              .arg(from_vnum)
              .arg(entityLabel('R', from_vnum)));
      item->setData(Qt::UserRole, QChar('R'));
      item->setData(Qt::UserRole + 1, static_cast<qlonglong>(from_vnum));
      entered_from_list_->addItem(item);
    }
  }
  if(entered_from_list_->count() == 0) {
    entered_from_list_->addItem(QStringLiteral("(nessuna entrata)"));
  }

  const eleutheria::Zone* zone = eleutheria::zone_for_room(*area_, loaded_vnum_);
  if(zone == nullptr) {
    contents_list_->addItem(QStringLiteral("(Nessuna zona collegata a questa stanza)"));
    return;
  }

  std::int64_t last_mob = 0;
  std::size_t last_mob_index = static_cast<std::size_t>(-1);
  for(std::size_t i = 0; i < zone->commands.size(); ++i) {
    const auto& cmd = zone->commands[i];
    if(cmd.is_comment) {
      continue;
    }
    if(cmd.command == 'M' && cmd.arg3 == loaded_vnum_) {
      last_mob = cmd.arg1;
      last_mob_index = i;
      auto* item = new QListWidgetItem(
          QStringLiteral("Mobile: #%1 %2").arg(cmd.arg1).arg(entityLabel('M', cmd.arg1)));
      item->setData(Qt::UserRole, QChar('M'));
      item->setData(Qt::UserRole + 1, static_cast<qulonglong>(i));
      item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
      contents_list_->addItem(item);
    } else if(cmd.command == 'O' && cmd.arg3 == loaded_vnum_) {
      last_mob = 0;
      last_mob_index = static_cast<std::size_t>(-1);
      auto* item = new QListWidgetItem(
          QStringLiteral("Object: #%1 %2").arg(cmd.arg1).arg(entityLabel('O', cmd.arg1)));
      item->setData(Qt::UserRole, QChar('O'));
      item->setData(Qt::UserRole + 1, static_cast<qulonglong>(i));
      item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
      contents_list_->addItem(item);
    } else if((cmd.command == 'E' || cmd.command == 'G') && last_mob > 0 &&
              last_mob_index != static_cast<std::size_t>(-1)) {
      QString detail;
      if(cmd.command == 'E') {
        const int pos = static_cast<int>(cmd.arg3);
        const char* pos_name =
            (pos >= 0 && pos < static_cast<int>(sizeof(kEquipPositions) / sizeof(kEquipPositions[0])))
                ? kEquipPositions[pos]
                : "?";
        detail = QStringLiteral("-- Equip: #%1 %2  [%3]")
                     .arg(cmd.arg1)
                     .arg(entityLabel('O', cmd.arg1))
                     .arg(QString::fromUtf8(pos_name));
      } else {
        detail = QStringLiteral("-- Give: #%1 %2")
                     .arg(cmd.arg1)
                     .arg(entityLabel('O', cmd.arg1));
      }
      auto* item = new QListWidgetItem(detail);
      item->setData(Qt::UserRole, QChar(cmd.command));
      item->setData(Qt::UserRole + 1, static_cast<qulonglong>(i));
      item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
      item->setData(Qt::UserRole + 3, static_cast<qulonglong>(last_mob_index));
      contents_list_->addItem(item);
    } else if(cmd.command == 'M' || cmd.command == 'O') {
      last_mob = 0;
      last_mob_index = static_cast<std::size_t>(-1);
    }
  }
  if(contents_list_->count() == 0) {
    contents_list_->addItem(QStringLiteral("(nessun mob/oggetto in questa stanza)"));
  }
}

eleutheria::Zone* RoomEditor::currentZone() {
  if(area_ == nullptr || loaded_vnum_ < 0) {
    return nullptr;
  }
  return eleutheria::zone_for_room(*area_, loaded_vnum_);
}

std::optional<std::size_t> RoomEditor::selectedResetIndex() const {
  const auto* item = contents_list_->currentItem();
  if(item == nullptr || !item->data(Qt::UserRole + 1).isValid()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(item->data(Qt::UserRole + 1).toULongLong());
}

std::optional<std::size_t> RoomEditor::selectedMobResetIndex() const {
  const auto* item = contents_list_->currentItem();
  if(item == nullptr) {
    return std::nullopt;
  }
  const QChar kind = item->data(Qt::UserRole).toChar();
  if(kind == QLatin1Char('M')) {
    return static_cast<std::size_t>(item->data(Qt::UserRole + 1).toULongLong());
  }
  if(kind == QLatin1Char('E') || kind == QLatin1Char('G')) {
    return static_cast<std::size_t>(item->data(Qt::UserRole + 3).toULongLong());
  }
  return std::nullopt;
}

void RoomEditor::goToEnteredFrom() {
  const auto* item = entered_from_list_->currentItem();
  if(item == nullptr || !item->data(Qt::UserRole + 1).isValid()) {
    return;
  }
  emit navigateRequested(QLatin1Char('R'), item->data(Qt::UserRole + 1).toLongLong());
}

void RoomEditor::goToReference() {
  const auto* item = contents_list_->currentItem();
  if(item == nullptr || !item->data(Qt::UserRole + 2).isValid()) {
    return;
  }
  const QChar kind = item->data(Qt::UserRole).toChar();
  const qint64 vnum = item->data(Qt::UserRole + 2).toLongLong();
  if(kind == QLatin1Char('M')) {
    emit navigateRequested(QLatin1Char('M'), vnum);
  } else if(kind == QLatin1Char('O') || kind == QLatin1Char('E') || kind == QLatin1Char('G')) {
    emit navigateRequested(QLatin1Char('O'), vnum);
  }
}

void RoomEditor::addMobReset() {
  if(area_ == nullptr || loaded_vnum_ < 0) {
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr) {
    QMessageBox::warning(this, QStringLiteral("Summary"),
                         QStringLiteral("Nessuna zona in cui inserire il reset. Crea prima una zona."));
    return;
  }
  std::map<std::int64_t, QString> choices;
  for(const auto& [vnum, mob] : area_->mobiles) {
    choices[vnum] = entityLabel('M', vnum);
  }
  const auto mob_vnum = pick_entity_vnum(this, QStringLiteral("Add mob"), choices);
  if(!mob_vnum) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'M';
  cmd.if_flag = 0;
  cmd.arg1 = *mob_vnum;
  cmd.arg2 = 1;
  cmd.arg3 = loaded_vnum_;
  cmd.arg4 = 1;
  zone->commands.push_back(cmd);
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

void RoomEditor::addObjectReset() {
  if(area_ == nullptr || loaded_vnum_ < 0) {
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr) {
    QMessageBox::warning(this, QStringLiteral("Summary"),
                         QStringLiteral("Nessuna zona in cui inserire il reset."));
    return;
  }
  std::map<std::int64_t, QString> choices;
  for(const auto& [vnum, obj] : area_->objects) {
    choices[vnum] = entityLabel('O', vnum);
  }
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Add object"), choices);
  if(!obj_vnum) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'O';
  cmd.if_flag = 0;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = 1;
  cmd.arg3 = loaded_vnum_;
  cmd.arg4 = 1;
  zone->commands.push_back(cmd);
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

void RoomEditor::equipSelectedMob() {
  const auto mob_index = selectedMobResetIndex();
  if(!mob_index || area_ == nullptr) {
    QMessageBox::information(this, QStringLiteral("Equip"),
                             QStringLiteral("Seleziona prima un mob (o una sua riga equip/give)."));
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr) {
    return;
  }
  std::map<std::int64_t, QString> choices;
  for(const auto& [vnum, obj] : area_->objects) {
    choices[vnum] = entityLabel('O', vnum);
  }
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Equip object"), choices);
  if(!obj_vnum) {
    return;
  }
  QStringList pos_labels;
  for(const char* p : kEquipPositions) {
    pos_labels << QString::fromUtf8(p);
  }
  bool ok = false;
  const QString pos_name = QInputDialog::getItem(this, QStringLiteral("Wear location"),
                                                 QStringLiteral("Slot:"), pos_labels, 16, false, &ok);
  if(!ok) {
    return;
  }
  const int pos = pos_labels.indexOf(pos_name);
  eleutheria::ResetCommand cmd;
  cmd.command = 'E';
  cmd.if_flag = 1;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = 1;
  cmd.arg3 = pos;
  const auto at = insertAfterMobChain(*zone, *mob_index);
  zone->commands.insert(zone->commands.begin() + static_cast<std::ptrdiff_t>(at), cmd);
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

void RoomEditor::giveSelectedMob() {
  const auto mob_index = selectedMobResetIndex();
  if(!mob_index || area_ == nullptr) {
    QMessageBox::information(this, QStringLiteral("Give"),
                             QStringLiteral("Seleziona prima un mob."));
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr) {
    return;
  }
  std::map<std::int64_t, QString> choices;
  for(const auto& [vnum, obj] : area_->objects) {
    choices[vnum] = entityLabel('O', vnum);
  }
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Give object"), choices);
  if(!obj_vnum) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'G';
  cmd.if_flag = 1;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = 1;
  const auto at = insertAfterMobChain(*zone, *mob_index);
  zone->commands.insert(zone->commands.begin() + static_cast<std::ptrdiff_t>(at), cmd);
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

void RoomEditor::removeSelectedReset() {
  const auto index = selectedResetIndex();
  if(!index) {
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr || *index >= zone->commands.size()) {
    return;
  }
  const char cmd = zone->commands[*index].command;
  if(cmd == 'M') {
    // Rimuove anche E/G/P collegati al mob
    auto first = zone->commands.begin() + static_cast<std::ptrdiff_t>(*index);
    auto last = first + 1;
    while(last != zone->commands.end()) {
      const char c = last->command;
      if(c == 'E' || c == 'G' || c == 'P') {
        ++last;
        continue;
      }
      break;
    }
    zone->commands.erase(first, last);
  } else {
    zone->commands.erase(zone->commands.begin() + static_cast<std::ptrdiff_t>(*index));
  }
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

eleutheria::Room RoomEditor::save() const {
  eleutheria::Room room;
  room.vnum = vnum_->value();
  room.name = name_->text().toStdString();
  room.description = description_->toPlainText().toStdString();
  room.zone_hint = zone_hint_->value();
  room.room_flags = static_cast<std::int64_t>(flags_->value());
  room.sector_type = combo_value(sector_);
  for(int i = 0; i < 6; ++i) {
    auto* to_item = exits_->item(i, 1);
    if(to_item == nullptr) {
      continue;
    }
    const QString to = to_item->text().trimmed();
    if(to.isEmpty()) {
      continue;
    }
    auto& ex = room.exits[static_cast<std::size_t>(i)];
    ex.present = true;
    ex.to_room = to.toLongLong();
    auto* flags_item = exits_->item(i, 2);
    auto* key_item = exits_->item(i, 3);
    auto* keyw_item = exits_->item(i, 4);
    ex.exit_info = flags_item ? flags_item->text().toLongLong() : 0;
    ex.key = (!key_item || key_item->text().isEmpty()) ? -1 : key_item->text().toLongLong();
    ex.keyword = keyw_item ? keyw_item->text().toStdString() : std::string{};
  }
  const auto lines = extras_->toPlainText().split('\n', Qt::SkipEmptyParts);
  for(const auto& line : lines) {
    const int bar = line.indexOf('|');
    eleutheria::ExtraDesc ed;
    if(bar < 0) {
      ed.keywords = line.toStdString();
    } else {
      ed.keywords = line.left(bar).toStdString();
      ed.description = line.mid(bar + 1).replace("\\n", "\n").toStdString();
    }
    room.extras.push_back(std::move(ed));
  }
  return room;
}

MobEditor::MobEditor(const eleutheria::Catalog& catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog) {
  auto* root = new QVBoxLayout(this);
  auto* form = new QFormLayout();
  vnum_ = make_spin(this, 0, 999999);
  name_ = new QLineEdit(this);
  short_ = new QLineEdit(this);
  letter_ = new QComboBox(this);
  for(const char* L : {"A", "L", "N", "B", "S"}) {
    letter_->addItem(L);
  }
  alignment_ = make_spin(this, -1000, 1000);
  mult_att_ = new QDoubleSpinBox(this);
  mult_att_->setRange(0, 20);
  mult_att_->setDecimals(0);
  level_ = make_spin(this, 0, 200);
  thac0_ = make_spin(this, -50, 50);
  armor_ = make_spin(this, -100, 100);
  hit_bonus_ = make_spin(this, -500, 5000);
  dam_dice_ = new QLineEdit("1d4+0", this);
  gold_ = make_spin(this, -1, 2000000000);
  exp_ = make_spin(this, -2000000000, 2000000000);
  race_ = make_spin(this, 0, 1000);
  position_ = new QComboBox(this);
  default_pos_ = new QComboBox(this);
  sex_ = new QComboBox(this);
  fill_named(position_, catalog_.positions());
  fill_named(default_pos_, catalog_.positions());
  fill_named(sex_, catalog_.genders());

  form->addRow("Vnum", vnum_);
  form->addRow("Keywords", name_);
  form->addRow("Short", short_);
  form->addRow("Tipo", letter_);
  form->addRow("Alignment", alignment_);
  form->addRow("Attacchi", mult_att_);
  form->addRow("Level", level_);
  form->addRow("THAC0", thac0_);
  form->addRow("AC", armor_);
  form->addRow("Hit bonus", hit_bonus_);
  form->addRow("Dam dice", dam_dice_);
  form->addRow("Gold", gold_);
  form->addRow("Exp", exp_);
  form->addRow("Race", race_);
  form->addRow("Position", position_);
  form->addRow("Default pos", default_pos_);
  form->addRow("Sex", sex_);
  root->addLayout(form);

  long_ = new LineLimitedPlainTextEdit(this);
  description_ = new LineLimitedPlainTextEdit(this);
  sounds_ = new LineLimitedPlainTextEdit(this);
  configure_mud_text_field(long_);
  configure_mud_text_field(description_);
  configure_mud_text_field(sounds_);
  root->addWidget(new QLabel("Long description", this));
  root->addWidget(long_);
  root->addWidget(new QLabel("Description", this));
  root->addWidget(description_);

  act_ = new FlagEditor(this);
  act_->setFlags(catalog_.act_flags());
  affect_ = new FlagEditor(this);
  affect_->setFlags(catalog_.affect_flags());
  root->addWidget(new QLabel("Act flags", this));
  root->addWidget(act_);
  root->addWidget(new QLabel("Affect flags", this));
  root->addWidget(affect_);
  root->addWidget(new QLabel("Sounds (L)", this));
  root->addWidget(sounds_);

  auto notify = [this]() { emit edited(); };
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(short_, &QLineEdit::textChanged, this, notify);
  connect(act_, &FlagEditor::valueChanged, this, notify);
}

void MobEditor::setMaxLineLength(int max_length) {
  if(long_ != nullptr) {
    long_->setMaxLineLength(max_length);
  }
  if(description_ != nullptr) {
    description_->setMaxLineLength(max_length);
  }
  if(sounds_ != nullptr) {
    sounds_->setMaxLineLength(max_length);
  }
}

void MobEditor::clear() { load(eleutheria::Mobile{}); }

void MobEditor::load(const eleutheria::Mobile& mob) {
  const QSignalBlocker b1(name_);
  const QSignalBlocker b2(short_);
  const QSignalBlocker b3(act_);
  const QSignalBlocker b4(affect_);
  vnum_->setValue(static_cast<int>(mob.vnum));
  name_->setText(QString::fromStdString(mob.name));
  short_->setText(QString::fromStdString(mob.short_descr));
  long_->setPlainText(QString::fromStdString(mob.long_descr));
  description_->setPlainText(QString::fromStdString(mob.description));
  act_->setValue(static_cast<std::uint64_t>(mob.act));
  affect_->setValue(static_cast<std::uint64_t>(mob.affected_by));
  alignment_->setValue(static_cast<int>(mob.alignment));
  letter_->setCurrentText(QString(QChar(mob.letter)));
  mult_att_->setValue(mob.mult_att);
  level_->setValue(static_cast<int>(mob.level));
  thac0_->setValue(static_cast<int>(mob.thac0));
  armor_->setValue(static_cast<int>(mob.armor));
  hit_bonus_->setValue(static_cast<int>(mob.hit_bonus));
  dam_dice_->setText(QString("%1d%2+%3")
                         .arg(mob.dam_dice_n)
                         .arg(mob.dam_dice_s)
                         .arg(mob.dam_bonus));
  gold_->setValue(static_cast<int>(std::min<std::int64_t>(mob.gold, 2000000000)));
  exp_->setValue(static_cast<int>(
      std::max<std::int64_t>(-2000000000, std::min<std::int64_t>(mob.exp, 2000000000))));
  race_->setValue(static_cast<int>(mob.race));
  set_combo_value(position_, static_cast<int>(mob.position));
  set_combo_value(default_pos_, static_cast<int>(mob.default_pos));
  set_combo_value(sex_, static_cast<int>(mob.sex));
  sounds_->setPlainText(QString::fromStdString(mob.sounds + "\n" + mob.distant_sounds));
}

eleutheria::Mobile MobEditor::save() const {
  eleutheria::Mobile mob;
  mob.vnum = vnum_->value();
  mob.name = name_->text().toStdString();
  mob.short_descr = short_->text().toStdString();
  mob.long_descr = long_->toPlainText().toStdString();
  mob.description = description_->toPlainText().toStdString();
  mob.act = static_cast<std::int64_t>(act_->value());
  mob.affected_by = static_cast<std::int64_t>(affect_->value());
  mob.alignment = alignment_->value();
  mob.letter = letter_->currentText().isEmpty() ? 'A' : letter_->currentText().at(0).toLatin1();
  mob.mult_att = mult_att_->value();
  mob.level = level_->value();
  mob.thac0 = thac0_->value();
  mob.armor = armor_->value();
  mob.hit_bonus = hit_bonus_->value();
  {
    const QString dice = dam_dice_->text().trimmed();
    int n = 1, s = 4, b = 0;
    sscanf(dice.toUtf8().constData(), "%dd%d+%d", &n, &s, &b);
    mob.dam_dice_n = n;
    mob.dam_dice_s = s;
    mob.dam_bonus = b;
  }
  mob.gold_exp_race_form = true;
  mob.gold = gold_->value();
  mob.exp = exp_->value();
  mob.race = race_->value();
  mob.position = combo_value(position_);
  mob.default_pos = combo_value(default_pos_);
  mob.sex = combo_value(sex_);
  const auto sound_lines = sounds_->toPlainText().split('\n');
  if(!sound_lines.isEmpty()) {
    mob.sounds = sound_lines[0].toStdString();
  }
  if(sound_lines.size() > 1) {
    mob.distant_sounds = sound_lines[1].toStdString();
  }
  return mob;
}

ObjEditor::ObjEditor(const eleutheria::Catalog& catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  tabs_ = new QTabWidget(this);
  root->addWidget(tabs_);

  // --- Dati ---
  auto* data_page = new QWidget(tabs_);
  auto* data_layout = new QVBoxLayout(data_page);
  auto* identity = new QFormLayout();
  vnum_ = make_spin(data_page, 0, 999999);
  name_ = new QLineEdit(data_page);
  short_ = new QLineEdit(data_page);
  type_ = new QComboBox(data_page);
  fill_named(type_, catalog_.item_types());
  identity->addRow(QStringLiteral("Vnum"), vnum_);
  identity->addRow(QStringLiteral("Keywords"), name_);
  identity->addRow(QStringLiteral("Short"), short_);
  identity->addRow(QStringLiteral("Type"), type_);
  data_layout->addLayout(identity);

  auto* values_box = new QGroupBox(QStringLiteral("Values / economia"), data_page);
  auto* values = new QFormLayout(values_box);
  v0_ = make_spin(values_box, -2000000000, 2000000000);
  v1_ = make_spin(values_box, -2000000000, 2000000000);
  v2_ = make_spin(values_box, -2000000000, 2000000000);
  v3_ = make_spin(values_box, -2000000000, 2000000000);
  weight_ = make_spin(values_box, 0, 2000000000);
  cost_ = make_spin(values_box, 0, 2000000000);
  rent_ = make_spin(values_box, 0, 2000000000);
  v0_label_ = new QLabel(QStringLiteral("Value 0"), values_box);
  v1_label_ = new QLabel(QStringLiteral("Value 1"), values_box);
  v2_label_ = new QLabel(QStringLiteral("Value 2"), values_box);
  v3_label_ = new QLabel(QStringLiteral("Value 3"), values_box);
  values->addRow(v0_label_, v0_);
  values->addRow(v1_label_, v1_);
  values->addRow(v2_label_, v2_);
  values->addRow(v3_label_, v3_);
  values->addRow(QStringLiteral("Peso"), weight_);
  values->addRow(QStringLiteral("Costo"), cost_);
  values->addRow(QStringLiteral("Rent / giorno"), rent_);
  data_layout->addWidget(values_box);
  data_layout->addStretch(1);
  tabs_->addTab(data_page, QStringLiteral("Dati"));

  // --- Testi ---
  auto* text_page = new QWidget(tabs_);
  auto* text_layout = new QVBoxLayout(text_page);
  description_ = new LineLimitedPlainTextEdit(text_page);
  action_ = new LineLimitedPlainTextEdit(text_page);
  configure_mud_text_field(description_);
  configure_mud_text_field(action_);
  description_->setPlaceholderText(QStringLiteral("Descrizione a terra (long)"));
  action_->setPlaceholderText(QStringLiteral("Action description (opzionale)"));
  text_layout->addWidget(new QLabel(QStringLiteral("Description"), text_page));
  text_layout->addWidget(description_, 2);
  text_layout->addWidget(new QLabel(QStringLiteral("Action description"), text_page));
  text_layout->addWidget(action_, 1);
  tabs_->addTab(text_page, QStringLiteral("Testi"));

  // --- Flags ---
  auto* flags_page = new QWidget(tabs_);
  auto* flags_layout = new QHBoxLayout(flags_page);
  auto* extra_col = new QVBoxLayout();
  auto* wear_col = new QVBoxLayout();
  extra_ = new FlagEditor(flags_page);
  extra_->setFlags(catalog_.extra_flags());
  wear_ = new FlagEditor(flags_page);
  wear_->setFlags(catalog_.wear_flags());
  extra_col->addWidget(new QLabel(QStringLiteral("Extra flags"), flags_page));
  extra_col->addWidget(extra_, 1);
  wear_col->addWidget(new QLabel(QStringLiteral("Wear flags"), flags_page));
  wear_col->addWidget(wear_, 1);
  flags_layout->addLayout(extra_col, 1);
  flags_layout->addLayout(wear_col, 1);
  tabs_->addTab(flags_page, QStringLiteral("Flags"));

  // --- Affects ---
  auto* affect_page = new QWidget(tabs_);
  auto* affect_layout = new QVBoxLayout(affect_page);
  affect_layout->addWidget(new QLabel(
      QStringLiteral("Affects (A): tipo APPLY e modificatore. "
                     "Es. Ride % +5 sulla sella."),
      affect_page));
  affects_ = new QTableWidget(eleutheria::kMaxObjAffect, 2, affect_page);
  affects_->setHorizontalHeaderLabels({QStringLiteral("Apply"), QStringLiteral("Modifier")});
  affects_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  affects_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  affects_->verticalHeader()->setVisible(false);
  affects_->setSelectionBehavior(QAbstractItemView::SelectRows);
  affects_->setMinimumHeight(220);
  for(int row = 0; row < eleutheria::kMaxObjAffect; ++row) {
    setAffectRow(row, 0, 0);
  }
  affect_layout->addWidget(affects_, 1);
  tabs_->addTab(affect_page, QStringLiteral("Affects"));

  auto notify = [this]() { notifyEdited(); };
  connect(vnum_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(short_, &QLineEdit::textChanged, this, notify);
  connect(type_, qOverload<int>(&QComboBox::currentIndexChanged), this, notify);
  connect(v0_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(v1_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(v2_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(v3_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(weight_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(cost_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(rent_, qOverload<int>(&QSpinBox::valueChanged), this, notify);
  connect(description_, &QPlainTextEdit::textChanged, this, notify);
  connect(action_, &QPlainTextEdit::textChanged, this, notify);
  connect(extra_, &FlagEditor::valueChanged, this, notify);
  connect(wear_, &FlagEditor::valueChanged, this, notify);
  connect(affects_, &QTableWidget::itemChanged, this, notify);
}

QComboBox* ObjEditor::makeApplyCombo(QWidget* parent) const {
  auto* combo = new QComboBox(parent);
  fill_named(combo, catalog_.apply_types());
  combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  combo->setMinimumContentsLength(28);
  return combo;
}

void ObjEditor::setAffectRow(int row, int location, int modifier) {
  auto* combo = makeApplyCombo(affects_);
  set_combo_value(combo, location);
  connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
    notifyEdited();
  });
  affects_->setCellWidget(row, 0, combo);

  auto* mod_item = new QTableWidgetItem(QString::number(modifier));
  mod_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  const QSignalBlocker blocker(affects_);
  affects_->setItem(row, 1, mod_item);
}

void ObjEditor::notifyEdited() { emit edited(); }

void ObjEditor::setMaxLineLength(int max_length) {
  if(description_ != nullptr) {
    description_->setMaxLineLength(max_length);
  }
  if(action_ != nullptr) {
    action_->setMaxLineLength(max_length);
  }
}

void ObjEditor::clear() { load(eleutheria::GameObject{}); }

void ObjEditor::load(const eleutheria::GameObject& obj) {
  const QSignalBlocker b1(name_);
  const QSignalBlocker b2(short_);
  const QSignalBlocker b3(extra_);
  const QSignalBlocker b4(wear_);
  const QSignalBlocker b5(type_);
  const QSignalBlocker b6(affects_);
  vnum_->setValue(static_cast<int>(obj.vnum));
  name_->setText(QString::fromStdString(obj.name));
  short_->setText(QString::fromStdString(obj.short_description));
  description_->setPlainText(QString::fromStdString(obj.description));
  action_->setPlainText(QString::fromStdString(obj.action_description));
  set_combo_value(type_, static_cast<int>(obj.type_flag));
  extra_->setValue(static_cast<std::uint64_t>(obj.extra_flags));
  wear_->setValue(static_cast<std::uint64_t>(obj.wear_flags));
  v0_->setValue(static_cast<int>(obj.value[0]));
  v1_->setValue(static_cast<int>(obj.value[1]));
  v2_->setValue(static_cast<int>(obj.value[2]));
  v3_->setValue(static_cast<int>(obj.value[3]));
  weight_->setValue(static_cast<int>(obj.weight));
  cost_->setValue(static_cast<int>(obj.cost));
  rent_->setValue(static_cast<int>(obj.cost_per_day));

  for(int row = 0; row < eleutheria::kMaxObjAffect; ++row) {
    int location = 0;
    int modifier = 0;
    if(row < static_cast<int>(obj.affects.size())) {
      location = obj.affects[static_cast<std::size_t>(row)].location;
      modifier = obj.affects[static_cast<std::size_t>(row)].modifier;
    }
    setAffectRow(row, location, modifier);
  }
}

eleutheria::GameObject ObjEditor::save() const {
  eleutheria::GameObject obj;
  obj.vnum = vnum_->value();
  obj.name = name_->text().toStdString();
  obj.short_description = short_->text().toStdString();
  obj.description = description_->toPlainText().toStdString();
  obj.action_description = action_->toPlainText().toStdString();
  obj.type_flag = combo_value(type_);
  obj.extra_flags = static_cast<std::int64_t>(extra_->value());
  obj.wear_flags = static_cast<std::int64_t>(wear_->value());
  obj.value = {v0_->value(), v1_->value(), v2_->value(), v3_->value()};
  obj.weight = weight_->value();
  obj.cost = cost_->value();
  obj.cost_per_day = rent_->value();
  for(int row = 0; row < affects_->rowCount(); ++row) {
    int loc = 0;
    if(auto* combo = qobject_cast<QComboBox*>(affects_->cellWidget(row, 0))) {
      loc = combo_value(combo);
    }
    int mod = 0;
    if(auto* item = affects_->item(row, 1)) {
      mod = item->text().toInt();
    }
    if(loc == 0 && mod == 0) {
      continue;
    }
    obj.affects.push_back({loc, mod});
  }
  return obj;
}

ZoneEditor::ZoneEditor(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  auto* form = new QFormLayout();
  number_ = make_spin(this, 0, 99999);
  name_ = new QLineEdit(this);
  top_ = make_spin(this, 0, 999999);
  lifespan_ = make_spin(this, 0, 10000);
  reset_mode_ = make_spin(this, 0, 3);
  form->addRow("Numero zona", number_);
  form->addRow("Nome", name_);
  form->addRow("Top vnum", top_);
  form->addRow("Lifespan", lifespan_);
  form->addRow("Reset mode", reset_mode_);
  root->addLayout(form);
  commands_ = new QTableWidget(0, 6, this);
  commands_->setHorizontalHeaderLabels({"Cmd", "If", "Arg1", "Arg2", "Arg3", "Arg4/Note"});
  commands_->horizontalHeader()->setStretchLastSection(true);
  root->addWidget(commands_);
  connect(name_, &QLineEdit::textChanged, this, [this]() { emit edited(); });
}

void ZoneEditor::clear() { load(eleutheria::Zone{}); }

void ZoneEditor::load(const eleutheria::Zone& zone) {
  const QSignalBlocker bname(name_);
  number_->setValue(zone.number);
  name_->setText(QString::fromStdString(zone.name));
  top_->setValue(static_cast<int>(zone.top));
  lifespan_->setValue(static_cast<int>(zone.lifespan));
  reset_mode_->setValue(static_cast<int>(zone.reset_mode));
  commands_->setRowCount(0);
  for(const auto& cmd : zone.commands) {
    const int row = commands_->rowCount();
    commands_->insertRow(row);
    if(cmd.is_comment) {
      commands_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(cmd.raw_line)));
      for(int c = 1; c < 6; ++c) {
        commands_->setItem(row, c, new QTableWidgetItem);
      }
      continue;
    }
    commands_->setItem(row, 0, new QTableWidgetItem(QString(QChar(cmd.command))));
    commands_->setItem(row, 1, new QTableWidgetItem(QString::number(cmd.if_flag)));
    commands_->setItem(row, 2, new QTableWidgetItem(QString::number(cmd.arg1)));
    commands_->setItem(row, 3, new QTableWidgetItem(QString::number(cmd.arg2)));
    commands_->setItem(row, 4, new QTableWidgetItem(QString::number(cmd.arg3)));
    commands_->setItem(row, 5,
                       new QTableWidgetItem(QString::number(cmd.arg4) + " " +
                                            QString::fromStdString(cmd.comment)));
  }
}

eleutheria::Zone ZoneEditor::save() const {
  eleutheria::Zone zone;
  zone.number = number_->value();
  zone.name = name_->text().toStdString();
  zone.top = top_->value();
  zone.lifespan = lifespan_->value();
  zone.reset_mode = reset_mode_->value();
  for(int row = 0; row < commands_->rowCount(); ++row) {
    eleutheria::ResetCommand cmd;
    const QString c0 = commands_->item(row, 0)->text();
    if(c0.startsWith('*') || c0.startsWith(';') || c0.size() > 1) {
      cmd.is_comment = true;
      cmd.raw_line = c0.toStdString();
      zone.commands.push_back(std::move(cmd));
      continue;
    }
    cmd.command = c0.isEmpty() ? '*' : c0.at(0).toLatin1();
    cmd.if_flag = commands_->item(row, 1)->text().toInt();
    cmd.arg1 = commands_->item(row, 2)->text().toLongLong();
    cmd.arg2 = commands_->item(row, 3)->text().toLongLong();
    cmd.arg3 = commands_->item(row, 4)->text().toLongLong();
    cmd.comment = commands_->item(row, 5)->text().toStdString();
    zone.commands.push_back(std::move(cmd));
  }
  return zone;
}
