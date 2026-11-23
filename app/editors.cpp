#include "editors.hpp"

#include "extra_desc_editor.hpp"
#include "flag_editor.hpp"
#include "line_limited_edit.hpp"

#include "eleutheria/mob_damage_estimate.hpp"
#include "eleutheria/workspace_index.hpp"
#include "eleutheria/world.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QStringList>
#include <QFormLayout>
#include <QFrame>
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
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

/** Sopprime emit edited() mentre un editor carica dati da file. */
struct LoadingGuard {
  bool& flag;
  explicit LoadingGuard(bool& loading) : flag(loading) { flag = true; }
  ~LoadingGuard() { flag = false; }
  LoadingGuard(const LoadingGuard&) = delete;
  LoadingGuard& operator=(const LoadingGuard&) = delete;
};

QLineEdit* make_num_edit(QWidget* parent) {
  auto* e = new QLineEdit(parent);
  e->setClearButtonEnabled(false);
  return e;
}

void set_num(QLineEdit* edit, std::int64_t value) {
  edit->setText(QString::number(value));
}

void set_num_double(QLineEdit* edit, double value) {
  if(value == static_cast<double>(static_cast<std::int64_t>(value))) {
    edit->setText(QString::number(static_cast<std::int64_t>(value)));
  } else {
    edit->setText(QString::number(value, 'g', 12));
  }
}

std::int64_t num_i64(const QLineEdit* edit, std::int64_t fallback = 0) {
  bool ok = false;
  const auto value = edit->text().trimmed().toLongLong(&ok);
  return ok ? value : fallback;
}

int num_i(const QLineEdit* edit, int fallback = 0) {
  return static_cast<int>(num_i64(edit, fallback));
}

double num_d(const QLineEdit* edit, double fallback = 0.0) {
  bool ok = false;
  const auto value = edit->text().trimmed().toDouble(&ok);
  return ok ? value : fallback;
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

char mob_letter_from_combo(const QComboBox* box) {
  const QVariant data = box->currentData();
  if(data.isValid() && !data.toString().isEmpty()) {
    return data.toString().at(0).toLatin1();
  }
  const QString text = box->currentText().trimmed();
  return text.isEmpty() ? 'A' : text.at(0).toLatin1();
}

void set_mob_letter_combo(QComboBox* box, char letter) {
  const int idx = box->findData(QString(QChar(letter)));
  if(idx >= 0) {
    box->setCurrentIndex(idx);
  } else {
    box->setCurrentIndex(0);
  }
}

void fill_mob_letter_combo(QComboBox* box) {
  box->clear();
  const struct {
    char code;
    const char* desc;
  } types[] = {
      {'N', "un attacco a round, senza rumori"},
      {'A', "piu' attacchi a round, senza rumori"},
      {'B', "come A, per mob non bashabili"},
      {'L', "piu' attacchi; emette suoni periodici (stanza e vicine)"},
      {'S', "formato semplice/vecchio (hit dice e XP assoluti)"},
  };
  for(const auto& t : types) {
    box->addItem(QStringLiteral("%1 - %2").arg(QChar(t.code), QString::fromUtf8(t.desc)),
                 QString(QChar(t.code)));
  }
  box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  box->setMinimumContentsLength(42);
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

/** Prefissi commento equip stile aree di produzione (E ... *! Sul corpo: ...). */
const char* kEquipCommentPrefix[] = {
    "Speciale",       "Dito destro",    "Dito sinistro",  "Collo 1",
    "Collo 2",        "Sul corpo",      "Sulla testa",    "Sulle gambe",
    "Ai piedi",       "Sulle mani",     "Sulle braccia",  "Come scudo",
    "Intorno",        "In vita",        "Polso destro",   "Polso sinistro",
    "Impugnato",      "In mano",        "Sulla schiena",  "Orecchio destro",
    "Orecchio sinistro", "Sugli occhi", "Arma caricata",  "Simbolo clan",
};

constexpr int kEquipPositionCount =
    static_cast<int>(sizeof(kEquipPositions) / sizeof(kEquipPositions[0]));

constexpr const char* kPlaceholderChoice = "— Vnum esterno / placeholder... —";

std::optional<std::int64_t> ask_reset_limit(QWidget* parent, const QString& title,
                                            const QString& label, std::int64_t default_value) {
  bool ok = false;
  const QString text = QInputDialog::getText(parent, title, label, QLineEdit::Normal,
                                             QString::number(default_value), &ok);
  if(!ok) {
    return std::nullopt;
  }
  bool parsed = false;
  const auto value = text.trimmed().toLongLong(&parsed);
  if(!parsed || value < 0) {
    QMessageBox::warning(parent, title, QStringLiteral("Inserisci un intero >= 0."));
    return std::nullopt;
  }
  return value;
}

/** arg2 + arg4 per reset M (posti/spawn e tetto stanza) o O (max mondo e stanza). */
struct ResetCountLimits {
  std::int64_t arg2 = 1;
  std::int64_t arg4 = 1;
};

std::optional<ResetCountLimits> ask_mob_reset_limits(QWidget* parent, std::int64_t cur_arg2 = 1,
                                                     std::int64_t cur_arg4 = 1) {
  const auto arg2 = ask_reset_limit(
      parent, QStringLiteral("Max mob"),
      QStringLiteral(
          "Max istanze di questa riga M (colonna \"Max mondo/riga\").\n"
          "Se e' l'unica M di quel vnum in zona e il valore e' > 1, sono i posti di spawn.\n"
          "Altrimenti ogni riga conta come 1 posto. 0 ammessi solo se sai cosa fai:"),
      cur_arg2);
  if(!arg2) {
    return std::nullopt;
  }
  const auto arg4 = ask_reset_limit(
      parent, QStringLiteral("Max in stanza"),
      QStringLiteral(
          "Max copie di questo mob in questa stanza (colonna \"Max stanza\").\n"
          "0 = nessun tetto per stanza:"),
      cur_arg4);
  if(!arg4) {
    return std::nullopt;
  }
  return ResetCountLimits{*arg2, *arg4};
}

std::optional<ResetCountLimits> ask_obj_room_reset_limits(QWidget* parent, std::int64_t cur_arg2 = 1,
                                                          std::int64_t cur_arg4 = 1) {
  const auto arg2 = ask_reset_limit(
      parent, QStringLiteral("Max nel mondo"),
      QStringLiteral(
          "Max copie di questo oggetto nel mondo intero (colonna \"Max mondo/riga\").\n"
          "0 = nessun tetto mondiale (puo' accumularsi a ogni reset):"),
      cur_arg2);
  if(!arg2) {
    return std::nullopt;
  }
  const auto arg4 = ask_reset_limit(
      parent, QStringLiteral("Max in stanza"),
      QStringLiteral(
          "Max copie di questo oggetto in questa stanza (colonna \"Max stanza\").\n"
          "0 = nessun tetto per stanza:"),
      cur_arg4);
  if(!arg4) {
    return std::nullopt;
  }
  return ResetCountLimits{*arg2, *arg4};
}

std::optional<std::int64_t> ask_obj_world_max(QWidget* parent, std::int64_t cur_arg2 = 1) {
  return ask_reset_limit(
      parent, QStringLiteral("Max nel mondo"),
      QStringLiteral(
          "Max copie di questo oggetto nel mondo intero (colonna \"Max mondo/riga\").\n"
          "0 = nessun tetto mondiale. Equip/Give non hanno max stanza:"),
      cur_arg2);
}

std::optional<std::int64_t> pick_placeholder_vnum(QWidget* parent, const QString& title) {
  bool ok = false;
  const QString text = QInputDialog::getText(
      parent, title, QStringLiteral("Vnum (anche se assente nel workspace):"), QLineEdit::Normal,
      QStringLiteral("0"), &ok);
  if(!ok) {
    return std::nullopt;
  }
  bool parsed = false;
  const auto value = text.trimmed().toLongLong(&parsed);
  if(!parsed) {
    return std::nullopt;
  }
  return value;
}

/**
 * Opzione B: lista da area corrente + altre aree workspace, oppure placeholder libero.
 * kind: 'M' mob, 'O' object, 'R' room.
 */
std::optional<std::int64_t> pick_entity_vnum(QWidget* parent, const QString& title, char kind,
                                            const eleutheria::Area* current_area,
                                            const QString& current_area_name,
                                            const eleutheria::WorkspaceEntityIndex* index) {
  QStringList labels;
  QList<std::int64_t> vnums;

  auto add_row = [&](std::int64_t vnum, const QString& area_name, const QString& label,
                     bool current) {
    const QString prefix =
        current ? QStringLiteral("[questa area]")
                : QStringLiteral("[%1]").arg(area_name.isEmpty() ? QStringLiteral("?") : area_name);
    labels << QStringLiteral("%1  %2  %3").arg(vnum, 6).arg(prefix, -14).arg(label);
    vnums << vnum;
  };

  if(kind == 'M' && current_area != nullptr) {
    for(const auto& [vnum, mob] : current_area->mobiles) {
      add_row(vnum, current_area_name,
              QString::fromStdString(eleutheria::strip_mud_colors(eleutheria::mob_list_label(mob))),
              true);
    }
  } else if(kind == 'O' && current_area != nullptr) {
    for(const auto& [vnum, obj] : current_area->objects) {
      add_row(vnum, current_area_name,
              QString::fromStdString(eleutheria::strip_mud_colors(eleutheria::obj_list_label(obj))),
              true);
    }
  } else if(kind == 'R' && current_area != nullptr) {
    for(const auto& [vnum, room] : current_area->rooms) {
      add_row(vnum, current_area_name,
              QString::fromStdString(eleutheria::strip_mud_colors(eleutheria::room_list_label(room))),
              true);
    }
  }

  if(index != nullptr) {
    const auto* map = kind == 'M'   ? &index->mobiles
                      : kind == 'O' ? &index->objects
                                    : &index->rooms;
    for(const auto& [vnum, ent] : *map) {
      if(current_area != nullptr) {
        if(kind == 'M' && current_area->mobiles.count(vnum)) {
          continue;
        }
        if(kind == 'O' && current_area->objects.count(vnum)) {
          continue;
        }
        if(kind == 'R' && current_area->rooms.count(vnum)) {
          continue;
        }
      }
      add_row(vnum, QString::fromStdString(ent.area_name), QString::fromStdString(ent.label),
              false);
    }
  }

  labels << QString::fromUtf8(kPlaceholderChoice);
  vnums << -1;

  if(labels.size() == 1) {
    // Solo placeholder
    return pick_placeholder_vnum(parent, title);
  }

  bool ok = false;
  const QString picked = QInputDialog::getItem(
      parent, title,
      QStringLiteral("Scegli dal workspace, oppure \"Vnum esterno / placeholder\":"), labels, 0,
      false, &ok);
  if(!ok || picked.isEmpty()) {
    return std::nullopt;
  }
  if(picked.startsWith(QString::fromUtf8(kPlaceholderChoice).left(3)) ||
     picked.contains(QStringLiteral("placeholder"))) {
    return pick_placeholder_vnum(parent, title);
  }
  const int idx = labels.indexOf(picked);
  if(idx < 0 || idx >= vnums.size() || vnums[idx] < 0) {
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
  vnum_ = make_num_edit(summary_page);
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
  edit_limits_btn_ = new QPushButton(QStringLiteral("Modifica max..."), summary_page);
  equip_btn_ = new QPushButton(QStringLiteral("Equip..."), summary_page);
  give_btn_ = new QPushButton(QStringLiteral("Give..."), summary_page);
  for(auto* b : {goto_ref_btn_, add_mob_btn_, add_obj_btn_, edit_limits_btn_, remove_btn_,
                 equip_btn_, give_btn_}) {
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
  zone_hint_ = make_num_edit(room_page);
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

  exits_ = new QTableWidget(6, 6, room_page);
  exits_->setHorizontalHeaderLabels(
      {QStringLiteral("Dir"), QStringLiteral("To"), QStringLiteral("Flags"), QStringLiteral("Key"),
       QStringLiteral("Keywords"), QStringLiteral("Desc generale")});
  exits_->verticalHeader()->setVisible(false);
  exits_->horizontalHeader()->setStretchLastSection(true);
  const char* dirs[] = {"N", "E", "S", "W", "U", "D"};
  for(int i = 0; i < 6; ++i) {
    exits_->setItem(i, 0, new QTableWidgetItem(QString::fromUtf8(dirs[i])));
    exits_->item(i, 0)->setFlags(Qt::ItemIsEnabled);
    for(int c = 1; c < 6; ++c) {
      exits_->setItem(i, c, new QTableWidgetItem);
    }
  }
  room_layout->addWidget(new QLabel(
      QStringLiteral(
          "Uscite: To puo' essere stanza di questa area, di un'altra area, o placeholder."),
      room_page));
  room_layout->addWidget(exits_);
  auto* exit_btns = new QHBoxLayout();
  auto* browse_exit_btn =
      new QPushButton(QStringLiteral("Sfoglia destinazione (riga selezionata)..."), room_page);
  exit_btns->addWidget(browse_exit_btn);
  exit_btns->addStretch();
  room_layout->addLayout(exit_btns);
  connect(browse_exit_btn, &QPushButton::clicked, this, [this]() {
    const int row = exits_->currentRow();
    if(row < 0 || row >= 6) {
      QMessageBox::information(this, QStringLiteral("Uscite"),
                               QStringLiteral("Seleziona prima una riga uscita (N/E/S/W/U/D)."));
      return;
    }
    const auto vnum = pick_entity_vnum(this, QStringLiteral("Destinazione uscita"), 'R', area_,
                                       current_area_name_, workspace_index_);
    if(!vnum) {
      return;
    }
    if(exits_->item(row, 1) == nullptr) {
      exits_->setItem(row, 1, new QTableWidgetItem);
    }
    exits_->item(row, 1)->setText(QString::number(*vnum));
    emit edited();
  });

  extras_ = new ExtraDescListEditor(room_page);
  room_layout->addWidget(extras_, 1);
  connect(extras_, &ExtraDescListEditor::edited, this, [this]() {
    if(loading_) {
      return;
    }
    emit edited();
  });
  tabs_->addTab(room_page, QStringLiteral("Dettaglio"));

  // --- Avanzate (teleport / river / tunnel / bright) ---
  auto* adv_page = new QWidget(tabs_);
  auto* adv_layout = new QVBoxLayout(adv_page);
  auto* tele_box = new QGroupBox(QStringLiteral("Teleport (sector riga = -1)"), adv_page);
  auto* tele_form = new QFormLayout(tele_box);
  teleport_enable_ = new QCheckBox(QStringLiteral("Abilita teleport"), tele_box);
  tele_time_ = make_num_edit(tele_box);
  tele_targ_ = make_num_edit(tele_box);
  tele_mask_ = make_num_edit(tele_box);
  tele_cnt_ = make_num_edit(tele_box);
  tele_form->addRow(teleport_enable_);
  tele_form->addRow(QStringLiteral("tele_time"), tele_time_);
  tele_form->addRow(QStringLiteral("tele_targ"), tele_targ_);
  tele_form->addRow(QStringLiteral("tele_mask"), tele_mask_);
  tele_form->addRow(QStringLiteral("tele_cnt (se mask ha COUNT)"), tele_cnt_);
  adv_layout->addWidget(tele_box);

  auto* river_box = new QGroupBox(QStringLiteral("River (settori acqua 7/9)"), adv_page);
  auto* river_form = new QFormLayout(river_box);
  river_enable_ = new QCheckBox(QStringLiteral("Scrivi river_speed/dir"), river_box);
  river_speed_ = make_num_edit(river_box);
  river_dir_ = make_num_edit(river_box);
  river_form->addRow(river_enable_);
  river_form->addRow(QStringLiteral("river_speed"), river_speed_);
  river_form->addRow(QStringLiteral("river_dir (0-5)"), river_dir_);
  adv_layout->addWidget(river_box);

  auto* tunnel_box = new QGroupBox(QStringLiteral("Tunnel / moblim"), adv_page);
  auto* tunnel_form = new QFormLayout(tunnel_box);
  tunnel_enable_ = new QCheckBox(QStringLiteral("Limite mob (flag TUNNEL)"), tunnel_box);
  moblim_ = make_num_edit(tunnel_box);
  tunnel_form->addRow(tunnel_enable_);
  tunnel_form->addRow(QStringLiteral("moblim"), moblim_);
  adv_layout->addWidget(tunnel_box);

  auto* bright_box = new QGroupBox(QStringLiteral("Bright (tag L)"), adv_page);
  auto* bright_layout = new QVBoxLayout(bright_box);
  bright_enable_ = new QCheckBox(QStringLiteral("Scrivi descrizioni bright"), bright_box);
  bright_night_ = new LineLimitedPlainTextEdit(bright_box);
  bright_day_ = new LineLimitedPlainTextEdit(bright_box);
  configure_mud_text_field(bright_night_);
  configure_mud_text_field(bright_day_);
  bright_layout->addWidget(bright_enable_);
  bright_layout->addWidget(new QLabel(QStringLiteral("Night"), bright_box));
  bright_layout->addWidget(bright_night_);
  bright_layout->addWidget(new QLabel(QStringLiteral("Day"), bright_box));
  bright_layout->addWidget(bright_day_);
  adv_layout->addWidget(bright_box, 1);
  tabs_->addTab(adv_page, QStringLiteral("Avanzate"));

  connect(add_mob_btn_, &QPushButton::clicked, this, &RoomEditor::addMobReset);
  connect(add_obj_btn_, &QPushButton::clicked, this, &RoomEditor::addObjectReset);
  connect(equip_btn_, &QPushButton::clicked, this, &RoomEditor::equipSelectedMob);
  connect(give_btn_, &QPushButton::clicked, this, &RoomEditor::giveSelectedMob);
  connect(remove_btn_, &QPushButton::clicked, this, &RoomEditor::removeSelectedReset);
  connect(edit_limits_btn_, &QPushButton::clicked, this, &RoomEditor::editSelectedResetLimits);
  connect(goto_entered_btn_, &QPushButton::clicked, this, &RoomEditor::goToEnteredFrom);
  connect(goto_ref_btn_, &QPushButton::clicked, this, &RoomEditor::goToReference);
  connect(entered_from_list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem*) { goToEnteredFrom(); });
  connect(contents_list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem*) { goToReference(); });

  auto notify = [this]() {
    if(loading_) {
      return;
    }
    emit edited();
  };
  connect(vnum_, &QLineEdit::textChanged, this, notify);
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(description_, &QPlainTextEdit::textChanged, this, notify);
  connect(zone_hint_, &QLineEdit::textChanged, this, notify);
  connect(sector_, qOverload<int>(&QComboBox::currentIndexChanged), this, notify);
  connect(flags_, &FlagEditor::valueChanged, this, [this, notify]() {
    flags_summary_->setText(flagNames(flags_->value()));
    notify();
  });
  connect(teleport_enable_, &QCheckBox::toggled, this, notify);
  connect(tele_time_, &QLineEdit::textChanged, this, notify);
  connect(tele_targ_, &QLineEdit::textChanged, this, notify);
  connect(tele_mask_, &QLineEdit::textChanged, this, notify);
  connect(tele_cnt_, &QLineEdit::textChanged, this, notify);
  connect(river_enable_, &QCheckBox::toggled, this, notify);
  connect(river_speed_, &QLineEdit::textChanged, this, notify);
  connect(river_dir_, &QLineEdit::textChanged, this, notify);
  connect(tunnel_enable_, &QCheckBox::toggled, this, notify);
  connect(moblim_, &QLineEdit::textChanged, this, notify);
  connect(bright_enable_, &QCheckBox::toggled, this, notify);
  connect(bright_night_, &QPlainTextEdit::textChanged, this, notify);
  connect(bright_day_, &QPlainTextEdit::textChanged, this, notify);
}

void RoomEditor::setMaxLineLength(int max_length) {
  if(description_ != nullptr) {
    description_->setMaxLineLength(max_length);
  }
  if(extras_ != nullptr) {
    extras_->setMaxLineLength(max_length);
  }
  if(bright_night_ != nullptr) {
    bright_night_->setMaxLineLength(max_length);
  }
  if(bright_day_ != nullptr) {
    bright_day_->setMaxLineLength(max_length);
  }
}

void RoomEditor::setArea(eleutheria::Area* area) { area_ = area; }

void RoomEditor::setWorkspaceIndex(const eleutheria::WorkspaceEntityIndex* index,
                                   const QString& current_area_name) {
  workspace_index_ = index;
  current_area_name_ = current_area_name;
}

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
  auto from_index = [&](const eleutheria::IndexedEntity* found) -> QString {
    if(found == nullptr) {
      return QStringLiteral("(esterno/placeholder)");
    }
    return QStringLiteral("[%1] %2")
        .arg(QString::fromStdString(found->area_name))
        .arg(QString::fromStdString(found->label));
  };
  if(kind == 'M') {
    if(area_ != nullptr) {
      const auto it = area_->mobiles.find(vnum);
      if(it != area_->mobiles.end()) {
        return QString::fromStdString(eleutheria::strip_mud_colors(
            it->second.short_descr.empty() ? it->second.name : it->second.short_descr));
      }
    }
    return from_index(workspace_index_ ? workspace_index_->find_mob(vnum) : nullptr);
  }
  if(kind == 'O') {
    if(area_ != nullptr) {
      const auto it = area_->objects.find(vnum);
      if(it != area_->objects.end()) {
        return QString::fromStdString(eleutheria::strip_mud_colors(
            it->second.short_description.empty() ? it->second.name
                                                 : it->second.short_description));
      }
    }
    return from_index(workspace_index_ ? workspace_index_->find_obj(vnum) : nullptr);
  }
  if(kind == 'R') {
    if(area_ != nullptr) {
      const auto it = area_->rooms.find(vnum);
      if(it != area_->rooms.end()) {
        return QString::fromStdString(eleutheria::strip_mud_colors(it->second.name));
      }
    }
    return from_index(workspace_index_ ? workspace_index_->find_room(vnum) : nullptr);
  }
  return QStringLiteral("?");
}

QString RoomEditor::commentEntityName(char kind, std::int64_t vnum) const {
  auto clean = [](const std::string& raw) {
    return QString::fromStdString(eleutheria::strip_mud_colors(raw)).trimmed();
  };
  if(kind == 'M') {
    if(area_ != nullptr) {
      const auto it = area_->mobiles.find(vnum);
      if(it != area_->mobiles.end()) {
        const QString name =
            clean(it->second.short_descr.empty() ? it->second.name : it->second.short_descr);
        if(!name.isEmpty()) {
          return name;
        }
      }
    }
    if(workspace_index_ != nullptr) {
      if(const auto* found = workspace_index_->find_mob(vnum)) {
        QString label = QString::fromStdString(found->label).trimmed();
        // label indice spesso "#vnum testo"
        if(label.startsWith(QLatin1Char('#'))) {
          const int sp = label.indexOf(QLatin1Char(' '));
          if(sp > 0) {
            label = label.mid(sp + 1).trimmed();
          }
        }
        if(!label.isEmpty()) {
          return label;
        }
      }
    }
    return QStringLiteral("xxx");
  }
  if(kind == 'O') {
    if(area_ != nullptr) {
      const auto it = area_->objects.find(vnum);
      if(it != area_->objects.end()) {
        const QString name = clean(it->second.short_description.empty() ? it->second.name
                                                                        : it->second.short_description);
        if(!name.isEmpty()) {
          return name;
        }
      }
    }
    if(workspace_index_ != nullptr) {
      if(const auto* found = workspace_index_->find_obj(vnum)) {
        QString label = QString::fromStdString(found->label).trimmed();
        if(label.startsWith(QLatin1Char('#'))) {
          const int sp = label.indexOf(QLatin1Char(' '));
          if(sp > 0) {
            label = label.mid(sp + 1).trimmed();
          }
        }
        if(!label.isEmpty()) {
          return label;
        }
      }
    }
    return QStringLiteral("xxx");
  }
  if(kind == 'R') {
    if(area_ != nullptr) {
      const auto it = area_->rooms.find(vnum);
      if(it != area_->rooms.end()) {
        const QString name = clean(it->second.name);
        if(!name.isEmpty()) {
          return name;
        }
      }
    }
    if(workspace_index_ != nullptr) {
      if(const auto* found = workspace_index_->find_room(vnum)) {
        QString label = QString::fromStdString(found->label).trimmed();
        if(label.startsWith(QLatin1Char('#'))) {
          const int sp = label.indexOf(QLatin1Char(' '));
          if(sp > 0) {
            label = label.mid(sp + 1).trimmed();
          }
        }
        if(!label.isEmpty()) {
          return label;
        }
      }
    }
    return QStringLiteral("xxx");
  }
  return QStringLiteral("xxx");
}

std::string RoomEditor::makeResetCommentM(std::int64_t mob_vnum, std::int64_t room_vnum) const {
  return QStringLiteral("*! Mob %1 (%2) nella stanza %3 (%4)")
      .arg(mob_vnum)
      .arg(commentEntityName('M', mob_vnum))
      .arg(room_vnum)
      .arg(commentEntityName('R', room_vnum))
      .toStdString();
}

std::string RoomEditor::makeResetCommentO(std::int64_t obj_vnum, std::int64_t room_vnum) const {
  return QStringLiteral("*! Carica oggetto %1 (%2) nella stanza %3 (%4)")
      .arg(obj_vnum)
      .arg(commentEntityName('O', obj_vnum))
      .arg(room_vnum)
      .arg(commentEntityName('R', room_vnum))
      .toStdString();
}

std::string RoomEditor::makeResetCommentE(std::int64_t obj_vnum, int wear_pos) const {
  const char* prefix = "Equip";
  if(wear_pos >= 0 && wear_pos < kEquipPositionCount) {
    prefix = kEquipCommentPrefix[wear_pos];
  }
  return QStringLiteral("*! %1: %2 (%3)")
      .arg(QString::fromUtf8(prefix))
      .arg(obj_vnum, 6, 10, QLatin1Char('0'))
      .arg(commentEntityName('O', obj_vnum))
      .toStdString();
}

std::string RoomEditor::makeResetCommentG(std::int64_t obj_vnum) const {
  return QStringLiteral("*! In inventario: %1 (%2)")
      .arg(obj_vnum, 6, 10, QLatin1Char('0'))
      .arg(commentEntityName('O', obj_vnum))
      .toStdString();
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
  const LoadingGuard guard(loading_);
  loaded_vnum_ = room.vnum;
  vnum_->blockSignals(true);
  name_->blockSignals(true);
  description_->blockSignals(true);
  const QSignalBlocker bflags(flags_);
  set_num(vnum_, room.vnum);
  name_->setText(QString::fromStdString(eleutheria::strip_mud_colors(room.name)));
  description_->setPlainText(QString::fromStdString(room.description));
  set_num(zone_hint_, room.zone_hint);
  flags_->setValue(static_cast<std::uint64_t>(room.room_flags));
  flags_summary_->setText(flagNames(static_cast<std::uint64_t>(room.room_flags)));
  set_combo_value(sector_, static_cast<int>(room.sector_type));
  for(int i = 0; i < 6; ++i) {
    for(int c = 1; c < 6; ++c) {
      if(exits_->item(i, c) == nullptr) {
        exits_->setItem(i, c, new QTableWidgetItem);
      }
    }
    const auto& ex = room.exits[static_cast<std::size_t>(i)];
    if(!ex.present) {
      for(int c = 1; c < 6; ++c) {
        exits_->item(i, c)->setText({});
      }
      continue;
    }
    exits_->item(i, 1)->setText(QString::number(ex.to_room));
    exits_->item(i, 2)->setText(QString::number(ex.exit_info));
    exits_->item(i, 3)->setText(QString::number(ex.key));
    exits_->item(i, 4)->setText(QString::fromStdString(ex.keyword));
    exits_->item(i, 5)->setText(QString::fromStdString(ex.general_description));
  }
  vnum_->blockSignals(false);
  name_->blockSignals(false);
  description_->blockSignals(false);
  extras_->setExtras(room.extras);

  teleport_enable_->setChecked(room.has_teleport);
  set_num(tele_time_, room.tele_time);
  set_num(tele_targ_, room.tele_targ);
  set_num(tele_mask_, room.tele_mask);
  set_num(tele_cnt_, room.tele_cnt);
  river_enable_->setChecked(room.has_river);
  set_num(river_speed_, room.river_speed);
  set_num(river_dir_, room.river_dir);
  tunnel_enable_->setChecked(room.has_tunnel_limit);
  set_num(moblim_, room.moblim < 1 ? 1 : room.moblim);
  bright_enable_->setChecked(room.has_bright);
  bright_night_->setPlainText(QString::fromStdString(room.bright_night));
  bright_day_->setPlainText(QString::fromStdString(room.bright_day));
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
          QStringLiteral("Mobile: #%1 %2  [max riga %3, max stanza %4]")
              .arg(cmd.arg1)
              .arg(entityLabel('M', cmd.arg1))
              .arg(cmd.arg2)
              .arg(cmd.arg4));
      item->setData(Qt::UserRole, QChar('M'));
      item->setData(Qt::UserRole + 1, static_cast<qulonglong>(i));
      item->setData(Qt::UserRole + 2, static_cast<qlonglong>(cmd.arg1));
      contents_list_->addItem(item);
    } else if(cmd.command == 'O' && cmd.arg3 == loaded_vnum_) {
      last_mob = 0;
      last_mob_index = static_cast<std::size_t>(-1);
      auto* item = new QListWidgetItem(
          QStringLiteral("Object: #%1 %2  [max mondo %3, max stanza %4]")
              .arg(cmd.arg1)
              .arg(entityLabel('O', cmd.arg1))
              .arg(cmd.arg2)
              .arg(cmd.arg4));
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
            (pos >= 0 && pos < kEquipPositionCount) ? kEquipPositions[pos] : "?";
        detail = QStringLiteral("-- Equip: #%1 %2  [%3]  [max mondo %4]")
                     .arg(cmd.arg1)
                     .arg(entityLabel('O', cmd.arg1))
                     .arg(QString::fromUtf8(pos_name))
                     .arg(cmd.arg2);
      } else {
        detail = QStringLiteral("-- Give: #%1 %2  [max mondo %3]")
                     .arg(cmd.arg1)
                     .arg(entityLabel('O', cmd.arg1))
                     .arg(cmd.arg2);
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
  const auto mob_vnum = pick_entity_vnum(this, QStringLiteral("Add mob"), 'M', area_,
                                         current_area_name_, workspace_index_);
  if(!mob_vnum) {
    return;
  }
  const auto limits = ask_mob_reset_limits(this);
  if(!limits) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'M';
  cmd.if_flag = 0;
  cmd.arg1 = *mob_vnum;
  cmd.arg2 = limits->arg2;
  cmd.arg3 = loaded_vnum_;
  cmd.arg4 = limits->arg4;
  cmd.comment = makeResetCommentM(*mob_vnum, loaded_vnum_);
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
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Add object"), 'O', area_,
                                         current_area_name_, workspace_index_);
  if(!obj_vnum) {
    return;
  }
  const auto limits = ask_obj_room_reset_limits(this);
  if(!limits) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'O';
  cmd.if_flag = 0;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = limits->arg2;
  cmd.arg3 = loaded_vnum_;
  cmd.arg4 = limits->arg4;
  cmd.comment = makeResetCommentO(*obj_vnum, loaded_vnum_);
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
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Equip object"), 'O', area_,
                                         current_area_name_, workspace_index_);
  if(!obj_vnum) {
    return;
  }
  QStringList pos_labels;
  for(int i = 0; i < kEquipPositionCount; ++i) {
    pos_labels << QString::fromUtf8(kEquipPositions[i]);
  }
  bool ok = false;
  const QString pos_name = QInputDialog::getItem(this, QStringLiteral("Wear location"),
                                                 QStringLiteral("Slot:"), pos_labels, 16, false, &ok);
  if(!ok) {
    return;
  }
  const int pos = pos_labels.indexOf(pos_name);
  const auto world_max = ask_obj_world_max(this);
  if(!world_max) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'E';
  cmd.if_flag = 1;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = *world_max;
  cmd.arg3 = pos;
  cmd.comment = makeResetCommentE(*obj_vnum, pos);
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
  const auto obj_vnum = pick_entity_vnum(this, QStringLiteral("Give object"), 'O', area_,
                                         current_area_name_, workspace_index_);
  if(!obj_vnum) {
    return;
  }
  const auto world_max = ask_obj_world_max(this);
  if(!world_max) {
    return;
  }
  eleutheria::ResetCommand cmd;
  cmd.command = 'G';
  cmd.if_flag = 1;
  cmd.arg1 = *obj_vnum;
  cmd.arg2 = *world_max;
  cmd.comment = makeResetCommentG(*obj_vnum);
  const auto at = insertAfterMobChain(*zone, *mob_index);
  zone->commands.insert(zone->commands.begin() + static_cast<std::ptrdiff_t>(at), cmd);
  emit areaResetsChanged();
  emit edited();
  rebuildSummary();
}

void RoomEditor::editSelectedResetLimits() {
  const auto index = selectedResetIndex();
  if(!index) {
    QMessageBox::information(this, QStringLiteral("Max reset"),
                             QStringLiteral("Seleziona prima un mob, oggetto, equip o give."));
    return;
  }
  eleutheria::Zone* zone = currentZone();
  if(zone == nullptr || *index >= zone->commands.size()) {
    return;
  }
  auto& cmd = zone->commands[*index];
  if(cmd.command == 'M') {
    const auto limits = ask_mob_reset_limits(this, cmd.arg2, cmd.arg4);
    if(!limits) {
      return;
    }
    cmd.arg2 = limits->arg2;
    cmd.arg4 = limits->arg4;
  } else if(cmd.command == 'O') {
    const auto limits = ask_obj_room_reset_limits(this, cmd.arg2, cmd.arg4);
    if(!limits) {
      return;
    }
    cmd.arg2 = limits->arg2;
    cmd.arg4 = limits->arg4;
  } else if(cmd.command == 'E' || cmd.command == 'G') {
    const auto world_max = ask_obj_world_max(this, cmd.arg2);
    if(!world_max) {
      return;
    }
    cmd.arg2 = *world_max;
  } else {
    QMessageBox::information(
        this, QStringLiteral("Max reset"),
        QStringLiteral("I max si modificano solo per comandi M, O, E e G."));
    return;
  }
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

eleutheria::Room RoomEditor::save(const eleutheria::Room* previous) const {
  eleutheria::Room room = previous ? *previous : eleutheria::Room{};
  room.vnum = num_i64(vnum_);
  room.name = name_->text().toStdString();
  room.description = description_->toPlainText().toStdString();
  room.zone_hint = num_i64(zone_hint_);
  room.room_flags = static_cast<std::int64_t>(flags_->value());
  room.sector_type = combo_value(sector_);
  for(int i = 0; i < 6; ++i) {
    auto* to_item = exits_->item(i, 1);
    if(to_item == nullptr) {
      room.exits[static_cast<std::size_t>(i)] = {};
      continue;
    }
    const QString to = to_item->text().trimmed();
    if(to.isEmpty()) {
      room.exits[static_cast<std::size_t>(i)] = {};
      continue;
    }
    auto& ex = room.exits[static_cast<std::size_t>(i)];
    const auto previous_ex =
        previous ? previous->exits[static_cast<std::size_t>(i)] : eleutheria::Exit{};
    ex = previous_ex;
    ex.present = true;
    ex.to_room = to.toLongLong();
    auto* flags_item = exits_->item(i, 2);
    auto* key_item = exits_->item(i, 3);
    auto* keyw_item = exits_->item(i, 4);
    auto* gdesc_item = exits_->item(i, 5);
    ex.exit_info = flags_item ? flags_item->text().toLongLong() : 0;
    ex.key = (!key_item || key_item->text().isEmpty()) ? -1 : key_item->text().toLongLong();
    ex.keyword = keyw_item ? keyw_item->text().toStdString() : std::string{};
    if(gdesc_item != nullptr) {
      ex.general_description = gdesc_item->text().toStdString();
    }
  }
  room.extras = extras_->extras();

  room.has_teleport = teleport_enable_->isChecked();
  room.tele_time = num_i64(tele_time_);
  room.tele_targ = num_i64(tele_targ_);
  room.tele_mask = num_i64(tele_mask_);
  room.tele_cnt = num_i64(tele_cnt_);
  room.has_river = river_enable_->isChecked();
  room.river_speed = num_i64(river_speed_);
  room.river_dir = num_i(river_dir_);
  room.has_tunnel_limit = tunnel_enable_->isChecked();
  room.moblim = num_i64(moblim_);
  room.has_bright = bright_enable_->isChecked();
  room.bright_night = bright_night_->toPlainText().toStdString();
  room.bright_day = bright_day_->toPlainText().toStdString();
  return room;
}

MobEditor::MobEditor(const eleutheria::Catalog& catalog, QWidget* parent)
    : QWidget(parent), catalog_(catalog) {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  tabs_ = new QTabWidget(this);
  root->addWidget(tabs_);

  // --- Dati ---
  auto* data_page = new QWidget(tabs_);
  auto* data_scroll = new QScrollArea(tabs_);
  data_scroll->setWidgetResizable(true);
  data_scroll->setFrameShape(QFrame::NoFrame);
  data_scroll->setWidget(data_page);
  auto* data_layout = new QVBoxLayout(data_page);
  auto* form = new QFormLayout();
  vnum_ = make_num_edit(data_page);
  name_ = new QLineEdit(data_page);
  short_ = new QLineEdit(data_page);
  letter_ = new QComboBox(data_page);
  fill_mob_letter_combo(letter_);
  alignment_ = make_num_edit(data_page);
  mult_att_ = make_num_edit(data_page);
  level_ = make_num_edit(data_page);
  thac0_ = make_num_edit(data_page);
  armor_ = make_num_edit(data_page);
  hit_bonus_ = make_num_edit(data_page);
  hit_dice_ = new QLineEdit(QStringLiteral("1d8+0"), data_page);
  dam_dice_ = new QLineEdit(QStringLiteral("1d4+0"), data_page);
  gold_ = make_num_edit(data_page);
  exp_ = make_num_edit(data_page);
  exp_hint_ = new QLabel(data_page);
  exp_hint_->setWordWrap(true);
  exp_hint_->setStyleSheet(QStringLiteral("color: #888;"));
  race_ = make_num_edit(data_page);
  spellpower_enable_ = new QCheckBox(QStringLiteral("Scrivi spellpower nel file"), data_page);
  spellpower_ = make_num_edit(data_page);
  position_ = new QComboBox(data_page);
  default_pos_ = new QComboBox(data_page);
  sex_ = new QComboBox(data_page);
  fill_named(position_, catalog_.positions());
  fill_named(default_pos_, catalog_.positions());
  fill_named(sex_, catalog_.genders());

  form->addRow(QStringLiteral("Vnum"), vnum_);
  form->addRow(QStringLiteral("Keywords"), name_);
  form->addRow(QStringLiteral("Short"), short_);
  form->addRow(QStringLiteral("Tipo"), letter_);
  form->addRow(QStringLiteral("Alignment"), alignment_);
  form->addRow(QStringLiteral("Attacchi"), mult_att_);
  form->addRow(QStringLiteral("Level"), level_);
  form->addRow(QStringLiteral("THAC0"), thac0_);
  form->addRow(QStringLiteral("AC"), armor_);
  hit_bonus_label_ = new QLabel(QStringLiteral("Hit bonus (A/N/B/L)"), data_page);
  form->addRow(hit_bonus_label_, hit_bonus_);
  hit_dice_label_ = new QLabel(QStringLiteral("Hit dice (solo S)"), data_page);
  form->addRow(hit_dice_label_, hit_dice_);
  form->addRow(QStringLiteral("Dam dice"), dam_dice_);
  form->addRow(QStringLiteral("Gold"), gold_);
  form->addRow(QStringLiteral("Exp (valore nel .mob)"), exp_);
  form->addRow(exp_hint_);
  form->addRow(QStringLiteral("Race"), race_);
  form->addRow(spellpower_enable_);
  form->addRow(QStringLiteral("Spellpower"), spellpower_);
  form->addRow(QStringLiteral("Position"), position_);
  form->addRow(QStringLiteral("Default pos"), default_pos_);
  form->addRow(QStringLiteral("Sex"), sex_);
  data_layout->addLayout(form);

  damage_sim_box_ = new QGroupBox(QStringLiteral("Stima danni"), data_page);
  auto* dam_layout = new QVBoxLayout(damage_sim_box_);
  damage_estimate_ = new QLabel(damage_sim_box_);
  damage_estimate_->setWordWrap(true);
  damage_estimate_->setTextFormat(Qt::RichText);
  damage_estimate_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  dam_layout->addWidget(damage_estimate_);
  data_layout->addWidget(damage_sim_box_);
  data_layout->addStretch(1);
  tabs_->addTab(data_scroll, QStringLiteral("Dati"));

  // --- Testi ---
  auto* text_page = new QWidget(tabs_);
  auto* text_layout = new QVBoxLayout(text_page);
  long_ = new LineLimitedPlainTextEdit(text_page);
  description_ = new LineLimitedPlainTextEdit(text_page);
  sounds_ = new LineLimitedPlainTextEdit(text_page);
  distant_sounds_ = new LineLimitedPlainTextEdit(text_page);
  configure_mud_text_field(long_);
  configure_mud_text_field(description_);
  configure_mud_text_field(sounds_);
  configure_mud_text_field(distant_sounds_);
  text_layout->addWidget(new QLabel(QStringLiteral("Long description"), text_page));
  text_layout->addWidget(long_, 2);
  text_layout->addWidget(new QLabel(QStringLiteral("Description"), text_page));
  text_layout->addWidget(description_, 2);
  text_layout->addWidget(new QLabel(
      QStringLiteral(
          "Suono in stanza (tipo L): messaggio periodico dove si trova il mob"),
      text_page));
  text_layout->addWidget(sounds_, 1);
  text_layout->addWidget(new QLabel(
      QStringLiteral(
          "Suono stanze vicine (tipo L): messaggio sentito nelle room adiacenti"),
      text_page));
  text_layout->addWidget(distant_sounds_, 1);
  tabs_->addTab(text_page, QStringLiteral("Testi"));

  // --- Flags (layout compatto, scroll se serve) ---
  auto* flags_page = new QWidget(tabs_);
  auto* flags_scroll = new QScrollArea(tabs_);
  flags_scroll->setWidgetResizable(true);
  flags_scroll->setFrameShape(QFrame::NoFrame);
  flags_scroll->setWidget(flags_page);
  auto* flags_layout = new QVBoxLayout(flags_page);
  flags_layout->setAlignment(Qt::AlignTop);
  act_ = new FlagEditor(flags_page);
  act_->setFlags(catalog_.act_flags());
  affect_ = new FlagEditor(flags_page);
  affect_->setFlags(catalog_.affect_flags());
  flags_layout->addWidget(new QLabel(QStringLiteral("Act flags"), flags_page));
  flags_layout->addWidget(act_, 0, Qt::AlignTop);
  flags_layout->addWidget(new QLabel(QStringLiteral("Affect flags"), flags_page));
  flags_layout->addWidget(affect_, 0, Qt::AlignTop);
  flags_layout->addStretch(1);
  tabs_->addTab(flags_scroll, QStringLiteral("Flags"));

  // --- Resistenze ---
  auto* res_page = new QWidget(tabs_);
  auto* res_scroll = new QScrollArea(tabs_);
  res_scroll->setWidgetResizable(true);
  res_scroll->setFrameShape(QFrame::NoFrame);
  res_scroll->setWidget(res_page);
  auto* res_layout = new QVBoxLayout(res_page);
  res_layout->setAlignment(Qt::AlignTop);
  immunities_enable_ = new QCheckBox(
      QStringLiteral("Scrivi immune / M_immune / susc nel file (sex codificato +3)"), res_page);
  res_layout->addWidget(immunities_enable_);
  immune_ = new FlagEditor(res_page);
  immune_->setFlags(catalog_.immunity_flags());
  m_immune_ = new FlagEditor(res_page);
  m_immune_->setFlags(catalog_.immunity_flags());
  susc_ = new FlagEditor(res_page);
  susc_->setFlags(catalog_.immunity_flags());
  res_layout->addWidget(new QLabel(QStringLiteral("Immune"), res_page));
  res_layout->addWidget(immune_, 0, Qt::AlignTop);
  res_layout->addWidget(new QLabel(QStringLiteral("M_immune"), res_page));
  res_layout->addWidget(m_immune_, 0, Qt::AlignTop);
  res_layout->addWidget(new QLabel(QStringLiteral("Susc"), res_page));
  res_layout->addWidget(susc_, 0, Qt::AlignTop);
  res_layout->addStretch(1);
  tabs_->addTab(res_scroll, QStringLiteral("Resistenze"));

  auto notify = [this]() {
    if(loading_) {
      return;
    }
    updateExpHint();
    updateDamageEstimate();
    emit edited();
  };
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(short_, &QLineEdit::textChanged, this, notify);
  connect(act_, &FlagEditor::valueChanged, this, notify);
  connect(affect_, &FlagEditor::valueChanged, this, notify);
  connect(letter_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, notify](int) {
    updateTypeDependentUi();
    notify();
  });
  connect(level_, &QLineEdit::textChanged, this, notify);
  connect(exp_, &QLineEdit::textChanged, this, notify);
  connect(gold_, &QLineEdit::textChanged, this, notify);
  connect(hit_bonus_, &QLineEdit::textChanged, this, notify);
  connect(mult_att_, &QLineEdit::textChanged, this, notify);
  connect(dam_dice_, &QLineEdit::textChanged, this, notify);
  updateTypeDependentUi();
  updateExpHint();
  updateDamageEstimate();
}

void MobEditor::updateTypeDependentUi() {
  const char letter = mob_letter_from_combo(letter_);
  const bool is_s = letter == 'S';
  const bool is_n = letter == 'N';
  hit_dice_->setEnabled(is_s);
  hit_bonus_->setEnabled(!is_s);
  if(hit_dice_label_ != nullptr) {
    hit_dice_label_->setEnabled(is_s);
  }
  if(hit_bonus_label_ != nullptr) {
    hit_bonus_label_->setEnabled(!is_s);
  }
  mult_att_->setEnabled(!is_n && !is_s);
}

void MobEditor::updateExpHint() {
  if(exp_hint_ == nullptr) {
    return;
  }
  eleutheria::Mobile probe;
  probe.letter = mob_letter_from_combo(letter_);
  probe.level = num_i64(level_);
  probe.exp = num_i64(exp_);
  probe.gold = num_i64(gold_);
  probe.hit_bonus = num_i64(hit_bonus_);
  const auto est = eleutheria::estimate_mob_exp(probe);
  if(probe.letter == 'S') {
    exp_hint_->setText(QStringLiteral(
        "Tipo S: il valore e' l'XP assoluto del mob (niente formula bonus). XP = %1.")
                           .arg(est.estimated_xp));
    return;
  }
  if(!est.uses_bonus_formula) {
    exp_hint_->setText(
        QStringLiteral(
            "XP assoluti al load: %1. "
            "Valore negativo nel file = XP fisso (senza formula e senza aggiungere gold).")
            .arg(est.estimated_xp));
    return;
  }
  QString msg = QStringLiteral(
                    "Bonus XP (tipico 0-32). Stima al load ~%1 = "
                    "base(%2) + phit(%3)*hp_stimati(%4) + sab(%5)*bonus(%6) + gold(%7). "
                    "Il bonus moltiplica sab: a lv %2 sab=%5, quindi bonus 1 aggiunge ~%5 XP, "
                    "bonus 10 ~%8 XP (oltre a base/hp/gold).")
                    .arg(est.estimated_xp)
                    .arg(probe.level)
                    .arg(est.phit)
                    .arg(est.estimated_hit)
                    .arg(est.sab)
                    .arg(probe.exp)
                    .arg(probe.gold)
                    .arg(est.sab * 10);
  if(est.bonus_unusually_high) {
    msg += QStringLiteral(" Attenzione: medit segnala bonus > 32 come troppo alto.");
  }
  exp_hint_->setText(msg);
}

void MobEditor::updateDamageEstimate() {
  if(damage_estimate_ == nullptr || act_ == nullptr || damage_sim_box_ == nullptr) {
    return;
  }

  QString mob_label = short_->text().trimmed();
  if(mob_label.isEmpty()) {
    mob_label = name_->text().trimmed();
  }
  if(mob_label.isEmpty()) {
    mob_label = QStringLiteral("(nuovo mob)");
  }
  mob_label.remove(QRegularExpression(QStringLiteral("\\$[cC][0-9]{4}")));
  mob_label = mob_label.simplified();

  const eleutheria::DiceValues dam = eleutheria::parse_dice_text(dam_dice_->text().toStdString());
  const char mobtype =
      mob_letter_from_combo(letter_);
  const int level = num_i(level_);
  const int mult = static_cast<int>(num_d(mult_att_));
  const int attacks = eleutheria::mob_type_uses_mult_att(mobtype) ? std::max(1, mult) : 1;

  const auto sim = eleutheria::simulate_mob_round_damage(static_cast<std::int64_t>(act_->value()),
                                                         mobtype, level, mult, dam);

  damage_sim_box_->setTitle(QStringLiteral("Stima danni — %1").arg(mob_label));

  const QString dice_text =
      QStringLiteral("%1d%2%3")
          .arg(dam.number)
          .arg(dam.size)
          .arg(dam.plus > 0   ? QStringLiteral("+%1").arg(dam.plus)
               : dam.plus < 0 ? QString::number(dam.plus)
                              : QString());

  const auto role_color = [](eleutheria::MobDamageRole role) -> QString {
    switch(role) {
      case eleutheria::MobDamageRole::Fighter:
        return QStringLiteral("#1b7a3c");
      case eleutheria::MobDamageRole::Mage:
        return QStringLiteral("#1a5fad");
      case eleutheria::MobDamageRole::Cleric:
        return QStringLiteral("#a65b00");
      case eleutheria::MobDamageRole::Psionic:
        return QStringLiteral("#0b6e7a");
    }
    return QStringLiteral("#888888");
  };

  QString html;
  html += QStringLiteral(
              "<div style='line-height:1.35;'>"
              "<div style='margin-bottom:6px;'>"
              "<span style='opacity:0.85;'>"
              "lvl <b>%1</b> · <b>%2</b> att · danno <b>%3</b>"
              "</span></div>")
              .arg(level)
              .arg(attacks)
              .arg(dice_text.toHtmlEscaped());

  for(const auto& component : sim.components) {
    QString label = QString::fromStdString(eleutheria::mob_damage_role_label(component.role));
    if(!component.act_labels.empty()) {
      label += QStringLiteral(" · %1").arg(QString::fromStdString(component.act_labels));
    }
    const QString color = role_color(component.role);
    html += QStringLiteral(
                "<div style='margin:3px 0;padding-left:6px;border-left:3px solid %1;'>"
                "<span style='font-weight:700;color:%1;'>%2</span> "
                "<span style='opacity:0.75;'>%3–%4</span> "
                "<span style='font-weight:700;color:%1;'>media %5</span> "
                "<span style='opacity:0.55;font-size:9pt;'>[%6]</span>"
                "</div>")
                .arg(color, label.toHtmlEscaped())
                .arg(component.min_damage)
                .arg(component.max_damage)
                .arg(component.avg_damage, 0, 'f', 1)
                .arg(QString::fromStdString(component.summary).toHtmlEscaped());
  }
  html += QStringLiteral("</div>");
  damage_estimate_->setText(html);
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
  if(distant_sounds_ != nullptr) {
    distant_sounds_->setMaxLineLength(max_length);
  }
}

void MobEditor::clear() { load(eleutheria::Mobile{}); }

void MobEditor::load(const eleutheria::Mobile& mob) {
  const LoadingGuard guard(loading_);
  const QSignalBlocker b1(name_);
  const QSignalBlocker b2(short_);
  const QSignalBlocker b3(act_);
  const QSignalBlocker b4(affect_);
  const QSignalBlocker b5(immune_);
  const QSignalBlocker b6(m_immune_);
  const QSignalBlocker b7(susc_);
  set_num(vnum_, mob.vnum);
  name_->setText(QString::fromStdString(mob.name));
  short_->setText(QString::fromStdString(mob.short_descr));
  long_->setPlainText(QString::fromStdString(mob.long_descr));
  description_->setPlainText(QString::fromStdString(mob.description));
  act_->setValue(static_cast<std::uint64_t>(mob.act));
  affect_->setValue(static_cast<std::uint64_t>(mob.affected_by));
  set_num(alignment_, mob.alignment);
  set_mob_letter_combo(letter_, mob.letter);
  set_num_double(mult_att_, mob.mult_att);
  set_num(level_, mob.level);
  set_num(thac0_, mob.thac0);
  set_num(armor_, mob.armor);
  set_num(hit_bonus_, mob.hit_bonus);
  hit_dice_->setText(QStringLiteral("%1d%2+%3")
                         .arg(mob.hit_dice_n)
                         .arg(mob.hit_dice_s)
                         .arg(mob.hit_dice_b));
  dam_dice_->setText(QStringLiteral("%1d%2+%3")
                         .arg(mob.dam_dice_n)
                         .arg(mob.dam_dice_s)
                         .arg(mob.dam_bonus));
  set_num(gold_, mob.gold);
  set_num(exp_, mob.exp);
  set_num(race_, mob.race);
  spellpower_enable_->setChecked(mob.has_spellpower);
  set_num(spellpower_, mob.spellpower);
  set_combo_value(position_, static_cast<int>(mob.position));
  set_combo_value(default_pos_, static_cast<int>(mob.default_pos));
  set_combo_value(sex_, static_cast<int>(mob.sex));
  immunities_enable_->setChecked(mob.has_immunities);
  immune_->setValue(static_cast<std::uint64_t>(mob.immune));
  m_immune_->setValue(static_cast<std::uint64_t>(mob.m_immune));
  susc_->setValue(static_cast<std::uint64_t>(mob.susc));
  sounds_->setPlainText(QString::fromStdString(mob.sounds));
  distant_sounds_->setPlainText(QString::fromStdString(mob.distant_sounds));
  updateTypeDependentUi();
  updateExpHint();
  updateDamageEstimate();
}

eleutheria::Mobile MobEditor::save(const eleutheria::Mobile* previous) const {
  eleutheria::Mobile mob = previous ? *previous : eleutheria::Mobile{};
  mob.vnum = num_i64(vnum_);
  mob.name = name_->text().toStdString();
  mob.short_descr = short_->text().toStdString();
  mob.long_descr = long_->toPlainText().toStdString();
  mob.description = description_->toPlainText().toStdString();
  mob.act = static_cast<std::int64_t>(act_->value());
  mob.affected_by = static_cast<std::int64_t>(affect_->value());
  mob.alignment = num_i64(alignment_);
  mob.letter = mob_letter_from_combo(letter_);
  mob.mult_att = num_d(mult_att_);
  mob.level = num_i64(level_);
  mob.thac0 = num_i64(thac0_);
  mob.armor = num_i64(armor_);
  mob.hit_bonus = num_i64(hit_bonus_);
  {
    const QString dice = hit_dice_->text().trimmed();
    int n = 1, s = 8, b = 0;
    sscanf(dice.toUtf8().constData(), "%dd%d+%d", &n, &s, &b);
    mob.hit_dice_n = n;
    mob.hit_dice_s = s;
    mob.hit_dice_b = b;
  }
  {
    const QString dice = dam_dice_->text().trimmed();
    int n = 1, s = 4, b = 0;
    sscanf(dice.toUtf8().constData(), "%dd%d+%d", &n, &s, &b);
    mob.dam_dice_n = n;
    mob.dam_dice_s = s;
    mob.dam_bonus = b;
  }
  if(previous == nullptr) {
    mob.gold_exp_race_form = true;
  }
  mob.gold = num_i64(gold_);
  mob.exp = num_i64(exp_);
  mob.race = num_i64(race_);
  mob.has_spellpower = spellpower_enable_->isChecked();
  mob.spellpower = num_i(spellpower_);
  mob.position = combo_value(position_);
  mob.default_pos = combo_value(default_pos_);
  mob.sex = combo_value(sex_);
  mob.has_immunities = immunities_enable_->isChecked();
  mob.immune = static_cast<std::int64_t>(immune_->value());
  mob.m_immune = static_cast<std::int64_t>(m_immune_->value());
  mob.susc = static_cast<std::int64_t>(susc_->value());
  mob.sounds = sounds_->toPlainText().toStdString();
  mob.distant_sounds = distant_sounds_->toPlainText().toStdString();
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
  vnum_ = make_num_edit(data_page);
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
  v0_ = make_num_edit(values_box);
  v1_ = make_num_edit(values_box);
  v2_ = make_num_edit(values_box);
  v3_ = make_num_edit(values_box);
  weight_ = make_num_edit(values_box);
  cost_ = make_num_edit(values_box);
  rent_ = make_num_edit(values_box);
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
  auto* flags_scroll = new QScrollArea(tabs_);
  flags_scroll->setWidgetResizable(true);
  flags_scroll->setFrameShape(QFrame::NoFrame);
  flags_scroll->setWidget(flags_page);
  auto* flags_layout = new QVBoxLayout(flags_page);
  flags_layout->setAlignment(Qt::AlignTop);
  extra_ = new FlagEditor(flags_page);
  extra_->setFlags(catalog_.extra_flags());
  extra2_ = new FlagEditor(flags_page);
  extra2_->setFlags(catalog_.extra_flags2());
  wear_ = new FlagEditor(flags_page);
  wear_->setFlags(catalog_.wear_flags());
  flags_layout->addWidget(new QLabel(QStringLiteral("Extra flags"), flags_page));
  flags_layout->addWidget(extra_, 0, Qt::AlignTop);
  flags_layout->addWidget(new QLabel(QStringLiteral("Extra flags 2 (F)"), flags_page));
  flags_layout->addWidget(extra2_, 0, Qt::AlignTop);
  flags_layout->addWidget(new QLabel(QStringLiteral("Wear flags"), flags_page));
  flags_layout->addWidget(wear_, 0, Qt::AlignTop);
  flags_layout->addStretch(1);
  tabs_->addTab(flags_scroll, QStringLiteral("Flags"));

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

  // --- Extra / Forbidden ---
  auto* extra_page = new QWidget(tabs_);
  auto* extra_layout = new QVBoxLayout(extra_page);
  extras_ = new ExtraDescListEditor(extra_page);
  extra_layout->addWidget(extras_, 2);
  connect(extras_, &ExtraDescListEditor::edited, this, [this]() { notifyEdited(); });
  forbidden_enable_ = new QCheckBox(QStringLiteral("Scrivi blocco P (forbidden)"), extra_page);
  forbidden_char_ = new LineLimitedPlainTextEdit(extra_page);
  forbidden_room_ = new LineLimitedPlainTextEdit(extra_page);
  configure_mud_text_field(forbidden_char_);
  configure_mud_text_field(forbidden_room_);
  extra_layout->addWidget(forbidden_enable_);
  extra_layout->addWidget(new QLabel(QStringLiteral("Forbidden to char"), extra_page));
  extra_layout->addWidget(forbidden_char_, 1);
  extra_layout->addWidget(new QLabel(QStringLiteral("Forbidden to room"), extra_page));
  extra_layout->addWidget(forbidden_room_, 1);
  tabs_->addTab(extra_page, QStringLiteral("Extra / P"));

  auto notify = [this]() { notifyEdited(); };
  connect(vnum_, &QLineEdit::textChanged, this, notify);
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(short_, &QLineEdit::textChanged, this, notify);
  connect(type_, qOverload<int>(&QComboBox::currentIndexChanged), this, notify);
  connect(v0_, &QLineEdit::textChanged, this, notify);
  connect(v1_, &QLineEdit::textChanged, this, notify);
  connect(v2_, &QLineEdit::textChanged, this, notify);
  connect(v3_, &QLineEdit::textChanged, this, notify);
  connect(weight_, &QLineEdit::textChanged, this, notify);
  connect(cost_, &QLineEdit::textChanged, this, notify);
  connect(rent_, &QLineEdit::textChanged, this, notify);
  connect(description_, &QPlainTextEdit::textChanged, this, notify);
  connect(action_, &QPlainTextEdit::textChanged, this, notify);
  connect(extra_, &FlagEditor::valueChanged, this, notify);
  connect(extra2_, &FlagEditor::valueChanged, this, notify);
  connect(wear_, &FlagEditor::valueChanged, this, notify);
  connect(affects_, &QTableWidget::itemChanged, this, notify);
  connect(forbidden_enable_, &QCheckBox::toggled, this, [this](bool) { notifyEdited(); });
  connect(forbidden_char_, &QPlainTextEdit::textChanged, this, notify);
  connect(forbidden_room_, &QPlainTextEdit::textChanged, this, notify);
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

void ObjEditor::notifyEdited() {
  if(loading_) {
    return;
  }
  emit edited();
}

void ObjEditor::setMaxLineLength(int max_length) {
  if(description_ != nullptr) {
    description_->setMaxLineLength(max_length);
  }
  if(action_ != nullptr) {
    action_->setMaxLineLength(max_length);
  }
  if(extras_ != nullptr) {
    extras_->setMaxLineLength(max_length);
  }
  if(forbidden_char_ != nullptr) {
    forbidden_char_->setMaxLineLength(max_length);
  }
  if(forbidden_room_ != nullptr) {
    forbidden_room_->setMaxLineLength(max_length);
  }
}

void ObjEditor::clear() { load(eleutheria::GameObject{}); }

void ObjEditor::load(const eleutheria::GameObject& obj) {
  const LoadingGuard guard(loading_);
  const QSignalBlocker b1(name_);
  const QSignalBlocker b2(short_);
  const QSignalBlocker b3(extra_);
  const QSignalBlocker b4(wear_);
  const QSignalBlocker b5(type_);
  const QSignalBlocker b6(affects_);
  const QSignalBlocker b7(extra2_);
  set_num(vnum_, obj.vnum);
  name_->setText(QString::fromStdString(obj.name));
  short_->setText(QString::fromStdString(obj.short_description));
  description_->setPlainText(QString::fromStdString(obj.description));
  action_->setPlainText(QString::fromStdString(obj.action_description));
  set_combo_value(type_, static_cast<int>(obj.type_flag));
  extra_->setValue(static_cast<std::uint64_t>(obj.extra_flags));
  extra2_->setValue(static_cast<std::uint64_t>(obj.extra_flags2));
  wear_->setValue(static_cast<std::uint64_t>(obj.wear_flags));
  set_num(v0_, obj.value[0]);
  set_num(v1_, obj.value[1]);
  set_num(v2_, obj.value[2]);
  set_num(v3_, obj.value[3]);
  set_num(weight_, obj.weight);
  set_num(cost_, obj.cost);
  set_num(rent_, obj.cost_per_day);

  for(int row = 0; row < eleutheria::kMaxObjAffect; ++row) {
    int location = 0;
    int modifier = 0;
    if(row < static_cast<int>(obj.affects.size())) {
      location = obj.affects[static_cast<std::size_t>(row)].location;
      modifier = obj.affects[static_cast<std::size_t>(row)].modifier;
    }
    setAffectRow(row, location, modifier);
  }

  extras_->setExtras(obj.extras);
  forbidden_enable_->setChecked(obj.has_forbidden);
  forbidden_char_->setPlainText(QString::fromStdString(obj.forbidden_to_char));
  forbidden_room_->setPlainText(QString::fromStdString(obj.forbidden_to_room));
}

eleutheria::GameObject ObjEditor::save(const eleutheria::GameObject* previous) const {
  eleutheria::GameObject obj = previous ? *previous : eleutheria::GameObject{};
  obj.vnum = num_i64(vnum_);
  obj.name = name_->text().toStdString();
  obj.short_description = short_->text().toStdString();
  obj.description = description_->toPlainText().toStdString();
  obj.action_description = action_->toPlainText().toStdString();
  obj.type_flag = combo_value(type_);
  obj.extra_flags = static_cast<std::int64_t>(extra_->value());
  obj.extra_flags2 = static_cast<std::int64_t>(extra2_->value());
  obj.has_extra_flags2 = obj.extra_flags2 != 0;
  obj.wear_flags = static_cast<std::int64_t>(wear_->value());
  obj.value = {num_i64(v0_), num_i64(v1_), num_i64(v2_), num_i64(v3_)};
  obj.weight = num_i64(weight_);
  obj.cost = num_i64(cost_);
  obj.cost_per_day = num_i64(rent_);
  obj.affects.clear();
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
  obj.extras = extras_->extras();
  obj.has_forbidden = forbidden_enable_->isChecked();
  obj.forbidden_to_char = forbidden_char_->toPlainText().toStdString();
  obj.forbidden_to_room = forbidden_room_->toPlainText().toStdString();
  return obj;
}

ZoneEditor::ZoneEditor(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  auto* form = new QFormLayout();
  number_ = make_num_edit(this);
  name_ = new QLineEdit(this);
  top_ = make_num_edit(this);
  lifespan_ = make_num_edit(this);
  reset_mode_ = make_num_edit(this);
  form->addRow("Numero zona", number_);
  form->addRow("Nome", name_);
  form->addRow("Top vnum", top_);
  form->addRow("Lifespan", lifespan_);
  form->addRow("Reset mode", reset_mode_);
  root->addLayout(form);
  root->addWidget(new QLabel(
      QStringLiteral(
          "Reset .zon: Vnum e stanza/slot possono essere di altre aree o xxx esterni.\n"
          "M: Max mondo/riga = posti spawn (se unica M del vnum), Max stanza = tetto in room.\n"
          "O: Max mondo/riga = tetto mondiale, Max stanza = tetto in room.\n"
          "E/G: solo Max mondo/riga (tetto mondiale); Max stanza non si usa.\n"
          "D: Stanza/slot = direzione; altri campi come da formato zona."),
      this));
  commands_ = new QTableWidget(0, 7, this);
  commands_->setHorizontalHeaderLabels(
      {QStringLiteral("Cmd"), QStringLiteral("If"), QStringLiteral("Vnum"),
       QStringLiteral("Max mondo/riga"), QStringLiteral("Stanza/slot"),
       QStringLiteral("Max stanza"), QStringLiteral("Commento")});
  commands_->horizontalHeader()->setStretchLastSection(true);
  commands_->verticalHeader()->setVisible(false);
  root->addWidget(commands_);
  auto notify = [this]() {
    if(loading_) {
      return;
    }
    emit edited();
  };
  connect(number_, &QLineEdit::textChanged, this, notify);
  connect(name_, &QLineEdit::textChanged, this, notify);
  connect(top_, &QLineEdit::textChanged, this, notify);
  connect(lifespan_, &QLineEdit::textChanged, this, notify);
  connect(reset_mode_, &QLineEdit::textChanged, this, notify);
  connect(commands_, &QTableWidget::itemChanged, this, notify);
}

void ZoneEditor::clear() { load(eleutheria::Zone{}); }

void ZoneEditor::load(const eleutheria::Zone& zone) {
  const LoadingGuard guard(loading_);
  const QSignalBlocker bname(name_);
  set_num(number_, zone.number);
  name_->setText(QString::fromStdString(zone.name));
  set_num(top_, zone.top);
  set_num(lifespan_, zone.lifespan);
  set_num(reset_mode_, zone.reset_mode);
  const QSignalBlocker block_cmds(commands_);
  commands_->setRowCount(0);
  for(const auto& cmd : zone.commands) {
    const int row = commands_->rowCount();
    commands_->insertRow(row);
    if(cmd.is_comment) {
      commands_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(cmd.raw_line)));
      for(int c = 1; c < 7; ++c) {
        commands_->setItem(row, c, new QTableWidgetItem);
      }
      continue;
    }
    commands_->setItem(row, 0, new QTableWidgetItem(QString(QChar(cmd.command))));
    commands_->setItem(row, 1, new QTableWidgetItem(QString::number(cmd.if_flag)));
    commands_->setItem(row, 2, new QTableWidgetItem(QString::number(cmd.arg1)));
    commands_->setItem(row, 3, new QTableWidgetItem(QString::number(cmd.arg2)));
    commands_->setItem(row, 4, new QTableWidgetItem(QString::number(cmd.arg3)));
    commands_->setItem(row, 5, new QTableWidgetItem(QString::number(cmd.arg4)));
    commands_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(cmd.comment)));
  }
}

eleutheria::Zone ZoneEditor::save() const {
  eleutheria::Zone zone;
  zone.number = num_i(number_);
  zone.name = name_->text().toStdString();
  zone.top = num_i64(top_);
  zone.lifespan = num_i64(lifespan_);
  zone.reset_mode = num_i(reset_mode_);
  for(int row = 0; row < commands_->rowCount(); ++row) {
    eleutheria::ResetCommand cmd;
    const QString c0 =
        commands_->item(row, 0) ? commands_->item(row, 0)->text() : QString{};
    if(c0.startsWith('*') || c0.startsWith(';') || c0.size() > 1) {
      cmd.is_comment = true;
      cmd.raw_line = c0.toStdString();
      zone.commands.push_back(std::move(cmd));
      continue;
    }
    cmd.command = c0.isEmpty() ? '*' : c0.at(0).toLatin1();
    auto cell_ll = [&](int col) -> std::int64_t {
      if(commands_->item(row, col) == nullptr) {
        return 0;
      }
      return commands_->item(row, col)->text().toLongLong();
    };
    cmd.if_flag = static_cast<int>(cell_ll(1));
    cmd.arg1 = cell_ll(2);
    cmd.arg2 = cell_ll(3);
    cmd.arg3 = cell_ll(4);
    cmd.arg4 = cell_ll(5);
    cmd.comment = commands_->item(row, 6) ? commands_->item(row, 6)->text().toStdString()
                                          : std::string{};
    zone.commands.push_back(std::move(cmd));
  }
  return zone;
}

SpeEditor::SpeEditor(QWidget* parent) : QWidget(parent) {
  auto* root = new QVBoxLayout(this);
  root->addWidget(new QLabel(
      QStringLiteral(
          "Special .spe: M/O/R + vnum + nome (anche fittizio) + parms opzionali. "
          "Il vnum puo' essere di questa area, di un'altra area del workspace, "
          "oppure un placeholder numerico. "
          "Il server assegna la procedura solo se il nome esiste a runtime."),
      this));
  table_ = new QTableWidget(0, 4, this);
  table_->setHorizontalHeaderLabels(
      {QStringLiteral("Tipo"), QStringLiteral("Vnum"), QStringLiteral("Nome"),
       QStringLiteral("Parms / commento")});
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->verticalHeader()->setVisible(false);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  root->addWidget(table_, 1);
  auto* btns = new QHBoxLayout();
  add_btn_ = new QPushButton(QStringLiteral("Aggiungi"), this);
  browse_btn_ = new QPushButton(QStringLiteral("Sfoglia vnum..."), this);
  remove_btn_ = new QPushButton(QStringLiteral("Rimuovi"), this);
  btns->addWidget(add_btn_);
  btns->addWidget(browse_btn_);
  btns->addWidget(remove_btn_);
  btns->addStretch();
  root->addLayout(btns);
  connect(add_btn_, &QPushButton::clicked, this, &SpeEditor::addRow);
  connect(browse_btn_, &QPushButton::clicked, this, &SpeEditor::browseVnumForSelected);
  connect(remove_btn_, &QPushButton::clicked, this, &SpeEditor::removeSelected);
  connect(table_, &QTableWidget::itemChanged, this, [this]() { emit edited(); });
}

void SpeEditor::setArea(eleutheria::Area* area) { area_ = area; }

void SpeEditor::setWorkspaceIndex(const eleutheria::WorkspaceEntityIndex* index,
                                  const QString& current_area_name) {
  workspace_index_ = index;
  current_area_name_ = current_area_name;
}

void SpeEditor::clear() { load({}); }

void SpeEditor::browseVnumForSelected() {
  const int row = table_->currentRow();
  if(row < 0) {
    QMessageBox::information(this, QStringLiteral("Special"),
                             QStringLiteral("Seleziona prima una riga M/O/R."));
    return;
  }
  const QString kind_text =
      table_->item(row, 0) ? table_->item(row, 0)->text().trimmed().toUpper() : QString();
  if(kind_text.isEmpty() || kind_text.startsWith(QLatin1Char('*')) ||
     kind_text.startsWith(QLatin1Char(';'))) {
    QMessageBox::information(this, QStringLiteral("Special"),
                             QStringLiteral("Sfoglia vale solo per righe M/O/R."));
    return;
  }
  const char kind = kind_text.at(0).toLatin1();
  if(kind != 'M' && kind != 'O' && kind != 'R') {
    return;
  }
  const auto vnum = pick_entity_vnum(this, QStringLiteral("Special vnum"), kind, area_,
                                     current_area_name_, workspace_index_);
  if(!vnum) {
    return;
  }
  if(table_->item(row, 1) == nullptr) {
    table_->setItem(row, 1, new QTableWidgetItem);
  }
  table_->item(row, 1)->setText(QString::number(*vnum));
  if(!loading_) {
    emit edited();
  }
}

void SpeEditor::load(const std::vector<eleutheria::SpecialAssignment>& specials) {
  const LoadingGuard guard(loading_);
  const QSignalBlocker block(table_);
  table_->setRowCount(0);
  for(const auto& sa : specials) {
    const int row = table_->rowCount();
    table_->insertRow(row);
    if(sa.is_comment) {
      table_->setItem(row, 0, new QTableWidgetItem(QStringLiteral("*")));
      table_->setItem(row, 1, new QTableWidgetItem);
      table_->setItem(row, 2, new QTableWidgetItem);
      table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(sa.raw_line)));
      continue;
    }
    table_->setItem(row, 0, new QTableWidgetItem(QString(QChar(sa.kind))));
    table_->setItem(row, 1, new QTableWidgetItem(QString::number(sa.vnum)));
    table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(sa.name)));
    table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(sa.parms)));
  }
}

std::vector<eleutheria::SpecialAssignment> SpeEditor::save() const {
  std::vector<eleutheria::SpecialAssignment> out;
  out.reserve(static_cast<std::size_t>(table_->rowCount()));
  for(int row = 0; row < table_->rowCount(); ++row) {
    eleutheria::SpecialAssignment sa;
    const QString kind =
        table_->item(row, 0) ? table_->item(row, 0)->text().trimmed() : QString{};
    const QString col3 =
        table_->item(row, 3) ? table_->item(row, 3)->text() : QString{};
    if(kind.isEmpty() || kind.startsWith(QLatin1Char('*')) || kind.startsWith(QLatin1Char(';')) ||
       kind.startsWith(QLatin1Char('$'))) {
      sa.is_comment = true;
      if(kind.size() <= 1 && !col3.isEmpty()) {
        sa.raw_line = col3.toStdString();
      } else if(!kind.isEmpty() && col3.isEmpty()) {
        sa.raw_line = kind.toStdString();
      } else {
        sa.raw_line = (kind + QLatin1Char(' ') + col3).trimmed().toStdString();
      }
      out.push_back(std::move(sa));
      continue;
    }
    sa.kind = kind.at(0).toUpper().toLatin1();
    if(sa.kind != 'M' && sa.kind != 'O' && sa.kind != 'R') {
      sa.is_comment = true;
      sa.raw_line = (kind + QLatin1Char(' ') +
                     (table_->item(row, 1) ? table_->item(row, 1)->text() : QString{}) +
                     QLatin1Char(' ') +
                     (table_->item(row, 2) ? table_->item(row, 2)->text() : QString{}) +
                     QLatin1Char(' ') + col3)
                        .trimmed()
                        .toStdString();
      out.push_back(std::move(sa));
      continue;
    }
    sa.vnum = table_->item(row, 1) ? table_->item(row, 1)->text().toLongLong() : 0;
    sa.name = table_->item(row, 2) ? table_->item(row, 2)->text().toStdString() : std::string{};
    sa.parms = col3.toStdString();
    out.push_back(std::move(sa));
  }
  return out;
}

void SpeEditor::addRow() {
  const int row = table_->rowCount();
  table_->insertRow(row);
  table_->setItem(row, 0, new QTableWidgetItem(QStringLiteral("M")));
  table_->setItem(row, 1, new QTableWidgetItem(QStringLiteral("0")));
  table_->setItem(row, 2, new QTableWidgetItem(QStringLiteral("NomeSpecial")));
  table_->setItem(row, 3, new QTableWidgetItem);
  if(!loading_) {
    emit edited();
  }
}

void SpeEditor::removeSelected() {
  const auto rows = table_->selectionModel()->selectedRows();
  QList<int> indexes;
  for(const auto& idx : rows) {
    indexes.push_back(idx.row());
  }
  std::sort(indexes.begin(), indexes.end(), std::greater<int>());
  for(int row : indexes) {
    table_->removeRow(row);
  }
  if(!indexes.isEmpty() && !loading_) {
    emit edited();
  }
}
