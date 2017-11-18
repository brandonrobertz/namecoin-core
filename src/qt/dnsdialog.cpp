#include "dnsdialog.h"
#include "ui_dnsdialog.h"

#include "names/main.h"
#include "platformstyle.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QClipboard>

DNSDialog::DNSDialog(const PlatformStyle *platformStyle,
        const QString &_name, const QString &_data,
        QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    ui(new Ui::DNSDialog),
    platformStyle(platformStyle),
    name(_name),
    data(_data)
{
    ui->setupUi(this);
    // TODO: add all subdomains here, select tld by default
    ui->comboDomain->addItem(fmtDotBit(name));
    ui->comboDomain->addItem(Ui::AddSubdomain);
}

DNSDialog::~DNSDialog()
{
    delete ui;
}

const QString DNSDialog::fmtDotBit(const QString name)
{
    return QString(name.mid(2) + ".bit");
}

void DNSDialog::accept()
{
    // TODO: figure out if we even need walletmodel
    if (!walletModel)
        return;
    // TODO: validate size & spec
    QDialog::accept();
}

void DNSDialog::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
}
