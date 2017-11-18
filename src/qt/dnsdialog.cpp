#include "dnsdialog.h"
#include "ui_dnsdialog.h"

#include "dnssubdomaindialog.h"
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
    connect(ui->comboDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(launchSubDomainDialog()));
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

void DNSDialog::launchSubDomainDialog()
{
    if(ui->comboDomain->currentText() != Ui::AddSubdomain)
        return;

    DNSSubDomainDialog dlg(platformStyle, name, this);
    if (dlg.exec() != QDialog::Accepted)
        ui->comboDomain->setCurrentIndex(0);

    std::cout << "NEW " << dlg.getReturnData().toStdString() << '\n';

    int index = ui->comboDomain->findData(dlg.getReturnData());
    if (index != -1)
        ui->comboDomain->setCurrentIndex(index);
}
