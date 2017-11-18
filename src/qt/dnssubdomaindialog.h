#ifndef DNSSUBDOMAINDIALOG_H
#define DNSSUBDOMAINDIALOG_H

#include "platformstyle.h"

#include <QDialog>

namespace Ui {
    class DNSSubDomainDialog;
}

class WalletModel;

/** Dialog for editing an address and associated information.
 */
class DNSSubDomainDialog: public QDialog
{
    Q_OBJECT

public:

    explicit DNSSubDomainDialog(const PlatformStyle *platformStyle,
            const QString &_name, QWidget *parent = nullptr);
    ~DNSSubDomainDialog();

    void setModel(WalletModel *walletModel);
    const QString fmtDotBit(const QString name);
    const QString getReturnData();

// public Q_SLOTS:
//     void accept() override;

private:
    Ui::DNSSubDomainDialog *ui;
    const PlatformStyle *platformStyle;
    WalletModel *walletModel;
    const QString name;
};

#endif // DNSSUBDOMAINDIALOG_H
