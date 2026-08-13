#ifndef DLGCUSTOMPATTERNS_H
#define DLGCUSTOMPATTERNS_H

#include <QDialog>
#include "settings.h"
#include "src/models/tablemodelcustomnamingpatterns.h"
#include <memory>

namespace Ui {
class DlgCustomPatterns;
}

class DlgCustomPatterns : public QDialog
{
    Q_OBJECT
public:
    explicit DlgCustomPatterns(QWidget *parent = nullptr);
    ~DlgCustomPatterns() override;

private slots:
    void evaluateRegEx();
    void btnCloseClicked();
    void tableViewPatternsClicked(const QModelIndex &index);
    void btnAddClicked();
    void btnDeleteClicked();
    void btnApplyChangesClicked();

private:
    CustomPattern *getSelectedPattern();
    std::unique_ptr<Ui::DlgCustomPatterns> ui;
    TableModelCustomNamingPatterns m_patternsModel;
    Settings m_settings;
};

#endif // DLGCUSTOMPATTERNS_H
