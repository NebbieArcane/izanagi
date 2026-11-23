#pragma once

#include "eleutheria/catalog.hpp"
#include "eleutheria/types.hpp"
#include "eleutheria/workspace_index.hpp"

#include <QWidget>
#include <optional>

class QLineEdit;
class LineLimitedPlainTextEdit;
class QComboBox;
class QTableWidget;
class QListWidget;
class QPushButton;
class QTabWidget;
class QLabel;
class QCheckBox;
class QGroupBox;
class FlagEditor;
class ExtraDescListEditor;

class RoomEditor : public QWidget {
  Q_OBJECT
 public:
  explicit RoomEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void setArea(eleutheria::Area* area);
  void setWorkspaceIndex(const eleutheria::WorkspaceEntityIndex* index,
                         const QString& current_area_name = {});
  void load(const eleutheria::Room& room);
  /** Parte da previous (merge-on-save): campi non in UI restano intatti. */
  eleutheria::Room save(const eleutheria::Room* previous = nullptr) const;
  void clear();
  void rebuildSummary();
  void setMaxLineLength(int max_length);

 signals:
  void edited();
  void areaResetsChanged();
  // kind: 'R' room, 'M' mob, 'O' object
  void navigateRequested(QChar kind, qint64 vnum);

 private slots:
  void addMobReset();
  void addObjectReset();
  void equipSelectedMob();
  void giveSelectedMob();
  void removeSelectedReset();
  void editSelectedResetLimits();
  void goToEnteredFrom();
  void goToReference();

 private:
  std::optional<std::size_t> selectedResetIndex() const;
  std::optional<std::size_t> selectedMobResetIndex() const;
  eleutheria::Zone* currentZone();
  QString flagNames(std::uint64_t bits) const;
  QString entityLabel(char kind, std::int64_t vnum) const;
  /** Short/name per commenti .zon; se assente: "xxx". */
  QString commentEntityName(char kind, std::int64_t vnum) const;
  std::string makeResetCommentM(std::int64_t mob_vnum, std::int64_t room_vnum) const;
  std::string makeResetCommentO(std::int64_t obj_vnum, std::int64_t room_vnum) const;
  std::string makeResetCommentE(std::int64_t obj_vnum, int wear_pos) const;
  std::string makeResetCommentG(std::int64_t obj_vnum) const;
  std::size_t insertAfterMobChain(eleutheria::Zone& zone, std::size_t mob_index) const;

  const eleutheria::Catalog& catalog_;
  eleutheria::Area* area_ = nullptr;
  const eleutheria::WorkspaceEntityIndex* workspace_index_ = nullptr;
  QString current_area_name_;
  std::int64_t loaded_vnum_ = -1;
  /** True durante load(): evita che textChanged/toggled marchino l'area dirty. */
  bool loading_ = false;

  QTabWidget* tabs_ = nullptr;
  QLineEdit* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* flags_summary_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  QLineEdit* zone_hint_ = nullptr;
  FlagEditor* flags_ = nullptr;
  QComboBox* sector_ = nullptr;
  QTableWidget* exits_ = nullptr;
  ExtraDescListEditor* extras_ = nullptr;

  QCheckBox* teleport_enable_ = nullptr;
  QLineEdit* tele_time_ = nullptr;
  QLineEdit* tele_targ_ = nullptr;
  QLineEdit* tele_mask_ = nullptr;
  QLineEdit* tele_cnt_ = nullptr;
  QCheckBox* river_enable_ = nullptr;
  QLineEdit* river_speed_ = nullptr;
  QLineEdit* river_dir_ = nullptr;
  QCheckBox* tunnel_enable_ = nullptr;
  QLineEdit* moblim_ = nullptr;
  QCheckBox* bright_enable_ = nullptr;
  LineLimitedPlainTextEdit* bright_night_ = nullptr;
  LineLimitedPlainTextEdit* bright_day_ = nullptr;

  QListWidget* entered_from_list_ = nullptr;
  QListWidget* contents_list_ = nullptr;
  QPushButton* goto_entered_btn_ = nullptr;
  QPushButton* goto_ref_btn_ = nullptr;
  QPushButton* add_mob_btn_ = nullptr;
  QPushButton* add_obj_btn_ = nullptr;
  QPushButton* equip_btn_ = nullptr;
  QPushButton* give_btn_ = nullptr;
  QPushButton* remove_btn_ = nullptr;
  QPushButton* edit_limits_btn_ = nullptr;
};

class MobEditor : public QWidget {
  Q_OBJECT
 public:
  explicit MobEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void load(const eleutheria::Mobile& mob);
  /** Parte da previous (merge-on-save): campi non in UI restano intatti. */
  eleutheria::Mobile save(const eleutheria::Mobile* previous = nullptr) const;
  void clear();
  void setMaxLineLength(int max_length);

 signals:
  void edited();

 private:
  void updateExpHint();
  void updateDamageEstimate();
  void updateTypeDependentUi();

  const eleutheria::Catalog& catalog_;
  bool loading_ = false;
  QTabWidget* tabs_ = nullptr;
  QLineEdit* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* short_ = nullptr;
  LineLimitedPlainTextEdit* long_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  FlagEditor* act_ = nullptr;
  FlagEditor* affect_ = nullptr;
  QLineEdit* alignment_ = nullptr;
  QComboBox* letter_ = nullptr;
  QLineEdit* mult_att_ = nullptr;
  QLineEdit* level_ = nullptr;
  QLineEdit* thac0_ = nullptr;
  QLineEdit* armor_ = nullptr;
  QLineEdit* hit_bonus_ = nullptr;
  QLineEdit* hit_dice_ = nullptr;
  QLineEdit* dam_dice_ = nullptr;
  QLineEdit* gold_ = nullptr;
  QLineEdit* exp_ = nullptr;
  QLabel* exp_hint_ = nullptr;
  QLabel* damage_estimate_ = nullptr;
  QGroupBox* damage_sim_box_ = nullptr;
  QLineEdit* race_ = nullptr;
  QCheckBox* spellpower_enable_ = nullptr;
  QLineEdit* spellpower_ = nullptr;
  QComboBox* position_ = nullptr;
  QComboBox* default_pos_ = nullptr;
  QComboBox* sex_ = nullptr;
  QCheckBox* immunities_enable_ = nullptr;
  FlagEditor* immune_ = nullptr;
  FlagEditor* m_immune_ = nullptr;
  FlagEditor* susc_ = nullptr;
  LineLimitedPlainTextEdit* sounds_ = nullptr;
  LineLimitedPlainTextEdit* distant_sounds_ = nullptr;
  QLabel* hit_bonus_label_ = nullptr;
  QLabel* hit_dice_label_ = nullptr;
};

class ObjEditor : public QWidget {
  Q_OBJECT
 public:
  explicit ObjEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void load(const eleutheria::GameObject& obj);
  /** Parte da previous (merge-on-save): campi non in UI restano intatti. */
  eleutheria::GameObject save(const eleutheria::GameObject* previous = nullptr) const;
  void clear();
  void setMaxLineLength(int max_length);

 signals:
  void edited();

 private:
  QComboBox* makeApplyCombo(QWidget* parent) const;
  void setAffectRow(int row, int location, int modifier);
  void notifyEdited();

  const eleutheria::Catalog& catalog_;
  bool loading_ = false;
  QTabWidget* tabs_ = nullptr;
  QLineEdit* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* short_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  LineLimitedPlainTextEdit* action_ = nullptr;
  QComboBox* type_ = nullptr;
  FlagEditor* extra_ = nullptr;
  FlagEditor* extra2_ = nullptr;
  FlagEditor* wear_ = nullptr;
  QLineEdit* v0_ = nullptr;
  QLineEdit* v1_ = nullptr;
  QLineEdit* v2_ = nullptr;
  QLineEdit* v3_ = nullptr;
  QLabel* v0_label_ = nullptr;
  QLabel* v1_label_ = nullptr;
  QLabel* v2_label_ = nullptr;
  QLabel* v3_label_ = nullptr;
  QLineEdit* weight_ = nullptr;
  QLineEdit* cost_ = nullptr;
  QLineEdit* rent_ = nullptr;
  QTableWidget* affects_ = nullptr;
  ExtraDescListEditor* extras_ = nullptr;
  QCheckBox* forbidden_enable_ = nullptr;
  LineLimitedPlainTextEdit* forbidden_char_ = nullptr;
  LineLimitedPlainTextEdit* forbidden_room_ = nullptr;
};

class ZoneEditor : public QWidget {
  Q_OBJECT
 public:
  explicit ZoneEditor(QWidget* parent = nullptr);
  void load(const eleutheria::Zone& zone);
  eleutheria::Zone save() const;
  void clear();

 signals:
  void edited();

 private:
  bool loading_ = false;
  QLineEdit* number_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* top_ = nullptr;
  QLineEdit* lifespan_ = nullptr;
  QLineEdit* reset_mode_ = nullptr;
  QTableWidget* commands_ = nullptr;
};

class SpeEditor : public QWidget {
  Q_OBJECT
 public:
  explicit SpeEditor(QWidget* parent = nullptr);
  void setArea(eleutheria::Area* area);
  void setWorkspaceIndex(const eleutheria::WorkspaceEntityIndex* index,
                         const QString& current_area_name = {});
  void load(const std::vector<eleutheria::SpecialAssignment>& specials);
  std::vector<eleutheria::SpecialAssignment> save() const;
  void clear();

 signals:
  void edited();

 private:
  void addRow();
  void browseVnumForSelected();
  void removeSelected();

  eleutheria::Area* area_ = nullptr;
  const eleutheria::WorkspaceEntityIndex* workspace_index_ = nullptr;
  QString current_area_name_;
  bool loading_ = false;
  QTableWidget* table_ = nullptr;
  QPushButton* add_btn_ = nullptr;
  QPushButton* browse_btn_ = nullptr;
  QPushButton* remove_btn_ = nullptr;
};
