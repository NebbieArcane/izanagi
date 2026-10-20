#pragma once

#include <QDialog>

class QLineEdit;
class QListWidget;
class QLabel;

namespace nebbie::qt {

class SpecialProcCatalogDialog : public QDialog {
    Q_OBJECT

public:
    explicit SpecialProcCatalogDialog(QWidget* parent = nullptr);

    /** True if the user saved changes (catalog should be re-applied by caller). */
    bool saved() const { return saved_; }

    QString serverRoot() const;
    QStringList extraNames() const;

private slots:
    void browseServerRoot();
    void reloadStatus();
    void addExtraName();
    void removeExtraName();
    void acceptAndSave();

private:
    void loadFromCurrentCatalog();

    QLineEdit* server_root_ = nullptr;
    QLabel* status_label_ = nullptr;
    QListWidget* extra_list_ = nullptr;
    QLineEdit* extra_name_ = nullptr;
    bool saved_ = false;
};

} // namespace nebbie::qt
