#include "dnssubdomaindialog.h"
#include "ui_dnssubdomaindialog.h"

#include "names/main.h"
#include "platformstyle.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QClipboard>

DNSSubDomainDialog::DNSSubDomainDialog(const PlatformStyle *platformStyle,
        const QString &_name, QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    ui(new Ui::DNSSubDomainDialog),
    platformStyle(platformStyle),
    name(_name)
{
    ui->setupUi(this);
}

DNSSubDomainDialog::~DNSSubDomainDialog()
{
    delete ui;
}

const QString DNSSubDomainDialog::fmtDotBit(const QString name)
{
    return QString(name.mid(2) + ".bit");
}

void DNSSubDomainDialog::accept()
{
    // TODO: figure out if we even need walletmodel
    if (!walletModel)
        return;
    // TODO: validate size & spec
    QDialog::accept();
}

void DNSSubDomainDialog::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
}

const QString DNSSubDomainDialog::getReturnData()
{
    return ui->editSubDomain->text();
}
