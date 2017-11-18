#include "configurenamedialog.h"
#include "ui_configurenamedialog.h"

#include "addressbookpage.h"
#include "dnsdialog.h"
#include "guiutil.h"
#include "names/main.h"
#include "platformstyle.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QClipboard>

ConfigureNameDialog::ConfigureNameDialog(const PlatformStyle *platformStyle,
                                         const QString &_name, const QString &data,
                                         bool _firstUpdate, QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    ui(new Ui::ConfigureNameDialog),
    platformStyle(platformStyle),
    name(_name),
    firstUpdate(_firstUpdate)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    ui->transferToLayout->setSpacing(4);
#endif

    GUIUtil::setupAddressWidget(ui->transferTo, this);

    ui->labelName->setText(name);
    ui->dataEdit->setText(data);

    returnData = data;

    if (name.startsWith("d/"))
        ui->labelDomain->setText(name.mid(2) + ".bit");
    else
        ui->labelDomain->setText(tr("(not a domain name)"));

    if (firstUpdate)
    {
        ui->labelTransferTo->hide();
        ui->labelTransferToHint->hide();
        ui->transferTo->hide();
        ui->addressBookButton->hide();
        ui->pasteButton->hide();
        ui->labelSubmitHint->setText(
            tr("name_firstupdate transaction will be queued and broadcasted when corresponding name_new is %1 blocks old")
            .arg(MIN_FIRSTUPDATE_DEPTH));
    }
    else
    {
        ui->labelSubmitHint->setText(tr("name_update transaction will be issued immediately"));
        setWindowTitle(tr("Update Name"));
    }
}


ConfigureNameDialog::~ConfigureNameDialog()
{
    delete ui;
}

void ConfigureNameDialog::accept()
{
    if (!walletModel)
        return;

    QString addr;
    if (!firstUpdate)
    {
        if (!ui->transferTo->text().isEmpty() && !ui->transferTo->hasAcceptableInput())
        {
            ui->transferTo->setValid(false);
            return;
        }

        addr = ui->transferTo->text();

        if (addr != "" && !walletModel->validateAddress(addr))
        {
            ui->transferTo->setValid(false);
            return;
        }

    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
        return;

    returnData = ui->dataEdit->text();
    if (!firstUpdate)
        returnTransferTo = ui->transferTo->text();

    QDialog::accept();
}

void ConfigureNameDialog::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
}

void ConfigureNameDialog::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->transferTo->setText(QApplication::clipboard()->text());
}

void ConfigureNameDialog::on_addressBookButton_clicked()
{
    if (!walletModel)
        return;

    AddressBookPage dlg(
        // platformStyle
        platformStyle,
        // mode
        AddressBookPage::ForSelection,
        // tab
        AddressBookPage::SendingTab,
        // *parent
        this);
    dlg.setModel(walletModel->getAddressTableModel());
    if (dlg.exec())
        ui->transferTo->setText(dlg.getReturnValue());
}

void ConfigureNameDialog::on_btnDNSEditor_clicked()
{
    // follow AddressbookPage and paste results into text if accepted
    const QString nameData = ui->dataEdit->text();
    DNSDialog dlg(platformStyle, name, nameData, this);
    dlg.exec();
}
