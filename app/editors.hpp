#pragma once

#include "eleutheria/catalog.hpp"
#include "eleutheria/types.hpp"

#include <QWidget>
#include <optional>

class QLineEdit;
class QSpinBox;
class LineLimitedPlainTextEdit;
class QComboBox;
class QDoubleSpinBox;
class QTableWidget;
class QListWidget;
class QPushButton;
class QTabWidget;
class QLabel;
class FlagEditor;

class RoomEditor : public QWidget {
  Q_OBJECT
 public:
  explicit RoomEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void setArea(eleutheria::Area* area);
  void load(const eleutheria::Room& room);
  eleutheria::Room save() const;
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
  void goToEnteredFrom();
  void goToReference();

 private:
  std::optional<std::size_t> selectedResetIndex() const;
  std::optional<std::size_t> selectedMobResetIndex() const;
  eleutheria::Zone* currentZone();
  QString flagNames(std::uint64_t bits) const;
  QString entityLabel(char kind, std::int64_t vnum) const;
  std::size_t insertAfterMobChain(eleutheria::Zone& zone, std::size_t mob_index) const;

  const eleutheria::Catalog& catalog_;
  eleutheria::Area* area_ = nullptr;
  std::int64_t loaded_vnum_ = -1;

  QTabWidget* tabs_ = nullptr;
  QSpinBox* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* flags_summary_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  QSpinBox* zone_hint_ = nullptr;
  FlagEditor* flags_ = nullptr;
  QComboBox* sector_ = nullptr;
  QTableWidget* exits_ = nullptr;
  LineLimitedPlainTextEdit* extras_ = nullptr;

  QListWidget* entered_from_list_ = nullptr;
  QListWidget* contents_list_ = nullptr;
  QPushButton* goto_entered_btn_ = nullptr;
  QPushButton* goto_ref_btn_ = nullptr;
  QPushButton* add_mob_btn_ = nullptr;
  QPushButton* add_obj_btn_ = nullptr;
  QPushButton* equip_btn_ = nullptr;
  QPushButton* give_btn_ = nullptr;
  QPushButton* remove_btn_ = nullptr;
};

class MobEditor : public QWidget {
  Q_OBJECT
 public:
  explicit MobEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void load(const eleutheria::Mobile& mob);
  eleutheria::Mobile save() const;
  void clear();
  void setMaxLineLength(int max_length);

 signals:
  void edited();

 private:
  const eleutheria::Catalog& catalog_;
  QSpinBox* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* short_ = nullptr;
  LineLimitedPlainTextEdit* long_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  FlagEditor* act_ = nullptr;
  FlagEditor* affect_ = nullptr;
  QSpinBox* alignment_ = nullptr;
  QComboBox* letter_ = nullptr;
  QDoubleSpinBox* mult_att_ = nullptr;
  QSpinBox* level_ = nullptr;
  QSpinBox* thac0_ = nullptr;
  QSpinBox* armor_ = nullptr;
  QSpinBox* hit_bonus_ = nullptr;
  QLineEdit* dam_dice_ = nullptr;
  QSpinBox* gold_ = nullptr;
  QSpinBox* exp_ = nullptr;
  QSpinBox* race_ = nullptr;
  QComboBox* position_ = nullptr;
  QComboBox* default_pos_ = nullptr;
  QComboBox* sex_ = nullptr;
  LineLimitedPlainTextEdit* sounds_ = nullptr;
};

class ObjEditor : public QWidget {
  Q_OBJECT
 public:
  explicit ObjEditor(const eleutheria::Catalog& catalog, QWidget* parent = nullptr);
  void load(const eleutheria::GameObject& obj);
  eleutheria::GameObject save() const;
  void clear();
  void setMaxLineLength(int max_length);

 signals:
  void edited();

 private:
  QComboBox* makeApplyCombo(QWidget* parent) const;
  void setAffectRow(int row, int location, int modifier);
  void notifyEdited();

  const eleutheria::Catalog& catalog_;
  QTabWidget* tabs_ = nullptr;
  QSpinBox* vnum_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* short_ = nullptr;
  LineLimitedPlainTextEdit* description_ = nullptr;
  LineLimitedPlainTextEdit* action_ = nullptr;
  QComboBox* type_ = nullptr;
  FlagEditor* extra_ = nullptr;
  FlagEditor* wear_ = nullptr;
  QSpinBox* v0_ = nullptr;
  QSpinBox* v1_ = nullptr;
  QSpinBox* v2_ = nullptr;
  QSpinBox* v3_ = nullptr;
  QLabel* v0_label_ = nullptr;
  QLabel* v1_label_ = nullptr;
  QLabel* v2_label_ = nullptr;
  QLabel* v3_label_ = nullptr;
  QSpinBox* weight_ = nullptr;
  QSpinBox* cost_ = nullptr;
  QSpinBox* rent_ = nullptr;
  QTableWidget* affects_ = nullptr;
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
  QSpinBox* number_ = nullptr;
  QLineEdit* name_ = nullptr;
  QSpinBox* top_ = nullptr;
  QSpinBox* lifespan_ = nullptr;
  QSpinBox* reset_mode_ = nullptr;
  QTableWidget* commands_ = nullptr;
};
