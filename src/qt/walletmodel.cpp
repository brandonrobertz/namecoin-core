// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmodel.h"

#include "addresstablemodel.h"
#include "consensus/validation.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "nametablemodel.h"
#include "optionsmodel.h"
#include "paymentserver.h"
#include "recentrequeststablemodel.h"
#include "sendcoinsdialog.h"
#include "transactiontablemodel.h"

#include "base58.h"
#include "chain.h"
#include "keystore.h"
#include "names/common.h"
#include "rpc/server.h"
#include "rpc/client.h"
#include "validation.h"
#include "net.h" // for g_connman
#include "policy/fees.h"
#include "policy/rbf.h"
#include "sync.h"
#include "ui_interface.h"
#include "util.h" // for GetBoolArg
#include "wallet/coincontrol.h"
#include "wallet/feebumper.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h" // for BackupWallet

#include <stdint.h>

#include <QDebug>
#include <QMessageBox>
#include <QSet>
#include <QTimer>

#include <boost/foreach.hpp>
#include <map>
#include <univalue.h>

WalletModel::WalletModel(const PlatformStyle *platformStyle, CWallet *_wallet, OptionsModel *_optionsModel, QWidget *parent) :
    QWidget(parent), wallet(_wallet), optionsModel(_optionsModel), addressTableModel(0),
    transactionTableModel(0),
    nameTableModel(0),
    recentRequestsTableModel(0),
    cachedBalance(0), cachedUnconfirmedBalance(0), cachedImmatureBalance(0),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0)
{
    fHaveWatchOnly = wallet->HaveWatchOnly();
    fForceCheckBalanceChanged = false;

    addressTableModel = new AddressTableModel(wallet, this);
    transactionTableModel = new TransactionTableModel(platformStyle, wallet, this);
    nameTableModel = new NameTableModel(platformStyle, wallet, this);
    recentRequestsTableModel = new RecentRequestsTableModel(wallet, this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollBalanceChanged()));
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

CAmount WalletModel::getBalance(const CCoinControl *coinControl) const
{
    if (coinControl)
    {
        return wallet->GetAvailableBalance(coinControl);
    }

    return wallet->GetBalance();
}

CAmount WalletModel::getUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedBalance();
}

CAmount WalletModel::getImmatureBalance() const
{
    return wallet->GetImmatureBalance();
}

bool WalletModel::haveWatchOnly() const
{
    return fHaveWatchOnly;
}

CAmount WalletModel::getWatchBalance() const
{
    return wallet->GetWatchOnlyBalance();
}

CAmount WalletModel::getWatchUnconfirmedBalance() const
{
    return wallet->GetUnconfirmedWatchOnlyBalance();
}

CAmount WalletModel::getWatchImmatureBalance() const
{
    return wallet->GetImmatureWatchOnlyBalance();
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if (cachedEncryptionStatus != newEncryptionStatus)
        Q_EMIT encryptionStatusChanged(newEncryptionStatus);
}

void WalletModel::pollBalanceChanged()
{
    // Get required locks upfront. This avoids the GUI from getting stuck on
    // periodical polls if the core is holding the locks for a longer time -
    // for example, during a wallet rescan.
    TRY_LOCK(cs_main, lockMain);
    if (!lockMain)
        return;

    if (fForceCheckBalanceChanged || chainActive.Height() != cachedNumBlocks)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = chainActive.Height();

        checkBalanceChanged();
        if (transactionTableModel)
            transactionTableModel->updateConfirmations();

        std::vector<std::string> successfulNames = sendPendingNameFirstUpdates();
        std::cout << "Successful names " << successfulNames.size() << '\n';
        for(std::string name : successfulNames)
            wallet->ErasePendingNameFirstUpdate(name);

    }
}

void WalletModel::checkBalanceChanged()
{
    CAmount newBalance = getBalance();
    CAmount newUnconfirmedBalance = getUnconfirmedBalance();
    CAmount newImmatureBalance = getImmatureBalance();
    CAmount newWatchOnlyBalance = 0;
    CAmount newWatchUnconfBalance = 0;
    CAmount newWatchImmatureBalance = 0;
    if (haveWatchOnly())
    {
        newWatchOnlyBalance = getWatchBalance();
        newWatchUnconfBalance = getWatchUnconfirmedBalance();
        newWatchImmatureBalance = getWatchImmatureBalance();
    }

    if (cachedBalance != newBalance || cachedUnconfirmedBalance != newUnconfirmedBalance || cachedImmatureBalance != newImmatureBalance ||
        cachedWatchOnlyBalance != newWatchOnlyBalance || cachedWatchUnconfBalance != newWatchUnconfBalance || cachedWatchImmatureBalance != newWatchImmatureBalance)
    {
        cachedBalance = newBalance;
        cachedUnconfirmedBalance = newUnconfirmedBalance;
        cachedImmatureBalance = newImmatureBalance;
        cachedWatchOnlyBalance = newWatchOnlyBalance;
        cachedWatchUnconfBalance = newWatchUnconfBalance;
        cachedWatchImmatureBalance = newWatchImmatureBalance;
        Q_EMIT balanceChanged(newBalance, newUnconfirmedBalance, newImmatureBalance,
                            newWatchOnlyBalance, newWatchUnconfBalance, newWatchImmatureBalance);
    }
}

void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if (addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
    return IsValidDestinationString(address.toStdString());
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl)
{
    CAmount total = 0;
    bool fSubtractFeeFromAmount = false;
    QList<SendCoinsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if (recipients.empty())
    {
        return OK;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    for (const SendCoinsRecipient &rcp : recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;

        if (rcp.paymentRequest.IsInitialized())
        {   // PaymentRequest...
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int i = 0; i < details.outputs_size(); i++)
            {
                const payments::Output& out = details.outputs(i);
                if (out.amount() <= 0) continue;
                subtotal += out.amount();
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                CAmount nAmount = out.amount();
                CRecipient recipient = {scriptPubKey, nAmount, rcp.fSubtractFeeFromAmount};
                vecSend.push_back(recipient);
            }
            if (subtotal <= 0)
            {
                return InvalidAmount;
            }
            total += subtotal;
        }
        else
        {   // User-entered bitcoin address / amount:
            if (!validateAddress(rcp.address))
            {
                return InvalidAddress;
            }
            if (rcp.amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey = GetScriptForDestination(DecodeDestination(rcp.address.toStdString()));
            CRecipient recipient = {scriptPubKey, rcp.amount, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total += rcp.amount;
        }
    }
    if (setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmount nBalance = getBalance(&coinControl);

    if (total > nBalance)
    {
        return AmountExceedsBalance;
    }

    {
        LOCK2(cs_main, wallet->cs_wallet);

        transaction.newPossibleKeyChange(wallet);

        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        std::string strFailReason;

        CWalletTx *newTx = transaction.getTransaction();
        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        bool fCreated = wallet->CreateTransaction(vecSend, NULL, *newTx, *keyChange, nFeeRequired, nChangePosRet, strFailReason, coinControl);
        transaction.setTransactionFee(nFeeRequired);
        if (fSubtractFeeFromAmount && fCreated)
            transaction.reassignAmounts(nChangePosRet);

        if (!fCreated)
        {
            if (!fSubtractFeeFromAmount && (total + nFeeRequired) > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason),
                         CClientUIInterface::MSG_ERROR);
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at maxTxFee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired > maxTxFee)
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    {
        LOCK2(cs_main, wallet->cs_wallet);
        CWalletTx *newTx = transaction.getTransaction();

        for (const SendCoinsRecipient &rcp : transaction.getRecipients())
        {
            if (rcp.paymentRequest.IsInitialized())
            {
                // Make sure any payment requests involved are still valid.
                if (PaymentServer::verifyExpired(rcp.paymentRequest.getDetails())) {
                    return PaymentRequestExpired;
                }

                // Store PaymentRequests in wtx.vOrderForm in wallet.
                std::string key("PaymentRequest");
                std::string value;
                rcp.paymentRequest.SerializeToString(&value);
                newTx->vOrderForm.push_back(make_pair(key, value));
            }
            else if (!rcp.message.isEmpty()) // Message from normal bitcoin:URI (bitcoin:123...?message=example)
                newTx->vOrderForm.push_back(make_pair("Message", rcp.message.toStdString()));
        }

        CReserveKey *keyChange = transaction.getPossibleKeyChange();
        CValidationState state;
        if (!wallet->CommitTransaction(*newTx, *keyChange, g_connman.get(), state))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(state.GetRejectReason()));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx->tx;
        transaction_array.append(&(ssTx[0]), ssTx.size());
    }

    // Add addresses / update labels that we've sent to the address book,
    // and emit coinsSent signal for each recipient
    for (const SendCoinsRecipient &rcp : transaction.getRecipients())
    {
        // Don't touch the address book when we have a payment request
        if (!rcp.paymentRequest.IsInitialized())
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = DecodeDestination(strAddress);
            std::string strLabel = rcp.label.toStdString();
            {
                LOCK(wallet->cs_wallet);

                std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(dest);

                // Check if we have a new address or an updated label
                if (mi == wallet->mapAddressBook.end())
                {
                    wallet->SetAddressBook(dest, strLabel, "send");
                }
                else if (mi->second.name != strLabel)
                {
                    wallet->SetAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }
            }
        }
        Q_EMIT coinsSent(wallet, rcp, transaction_array);
    }
    checkBalanceChanged(); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

    return SendCoinsReturn(OK);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}


NameTableModel *WalletModel::getNameTableModel()
{
      return nameTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if (!wallet->IsCrypted())
    {
        return Unencrypted;
    }
    else if (wallet->IsLocked())
    {
        return Locked;
    }
    else
    {
        return Unlocked;
    }
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if (encrypted)
    {
        // Encrypt
        return wallet->EncryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase)
{
    if (locked)
    {
        // Lock
        return wallet->Lock();
    }
    else
    {
        // Unlock
        return wallet->Unlock(passPhrase);
    }
}

bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    bool retval;
    {
        LOCK(wallet->cs_wallet);
        wallet->Lock(); // Make sure wallet is locked before attempting pass change
        retval = wallet->ChangeWalletPassphrase(oldPass, newPass);
    }
    return retval;
}

bool WalletModel::backupWallet(const QString &filename)
{
    return wallet->BackupWallet(filename.toLocal8Bit().data());
}

// Handlers for core signals
static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel, CCryptoKeyStore *wallet)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel, CWallet *wallet,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(EncodeDestination(address));
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
}

static void NotifyTransactionChanged(WalletModel *walletmodel, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(wallet);
    Q_UNUSED(hash);
    Q_UNUSED(status);
    QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyStatusChanged.connect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.connect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.connect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyStatusChanged.disconnect(boost::bind(&NotifyKeyStoreStatusChanged, this, _1));
    wallet->NotifyAddressBookChanged.disconnect(boost::bind(NotifyAddressBookChanged, this, _1, _2, _3, _4, _5, _6));
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
    wallet->NotifyWatchonlyChanged.disconnect(boost::bind(NotifyWatchonlyChanged, this, _1));
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    bool was_locked = getEncryptionStatus() == Locked;
    if (was_locked)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = getEncryptionStatus() != Locked;

    return UnlockContext(this, valid, was_locked);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, bool _relock):
        wallet(_wallet),
        valid(_valid),
        relock(_relock)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if (valid && relock)
    {
        wallet->setWalletLocked(true);
    }
}

void WalletModel::UnlockContext::CopyFrom(const UnlockContext& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return wallet->GetPubKey(address, vchPubKeyOut);
}

bool WalletModel::IsSpendable(const CTxDestination& dest) const
{
    return IsMine(*wallet, dest) & ISMINE_SPENDABLE;
}

bool WalletModel::getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const
{
    return wallet->GetKey(address, vchPrivKeyOut);
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
    LOCK2(cs_main, wallet->cs_wallet);
    for (const COutPoint& outpoint : vOutpoints)
    {
        auto it = wallet->mapWallet.find(outpoint.hash);
        if (it == wallet->mapWallet.end()) continue;
        int nDepth = it->second.GetDepthInMainChain();
        if (nDepth < 0) continue;
        COutput out(&it->second, outpoint.n, nDepth, true /* spendable */, true /* solvable */, true /* safe */);
        vOutputs.push_back(out);
    }
}

bool WalletModel::isSpent(const COutPoint& outpoint) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsSpent(outpoint.hash, outpoint.n);
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void WalletModel::listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
    for (auto& group : wallet->ListCoins()) {
        auto& resultGroup = mapCoins[QString::fromStdString(EncodeDestination(group.first))];
        for (auto& coin : group.second) {
            resultGroup.emplace_back(std::move(coin));
        }
    }
}

bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->IsLockedCoin(hash, n);
}

void WalletModel::lockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->LockCoin(output);
}

void WalletModel::unlockCoin(COutPoint& output)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->UnlockCoin(output);
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    LOCK2(cs_main, wallet->cs_wallet);
    wallet->ListLockedCoins(vOutpts);
}

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    vReceiveRequests = wallet->GetDestValues("rr"); // receive request
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = DecodeDestination(sAddress);

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    LOCK(wallet->cs_wallet);
    if (sRequest.empty())
        return wallet->EraseDestData(dest, key);
    else
        return wallet->AddDestData(dest, key, sRequest);
}

bool WalletModel::transactionCanBeAbandoned(uint256 hash) const
{
    return wallet->TransactionCanBeAbandoned(hash);
}

bool WalletModel::abandonTransaction(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    return wallet->AbandonTransaction(hash);
}

bool WalletModel::transactionCanBeBumped(uint256 hash) const
{
    LOCK2(cs_main, wallet->cs_wallet);
    const CWalletTx *wtx = wallet->GetWalletTx(hash);
    return wtx && SignalsOptInRBF(*wtx) && !wtx->mapValue.count("replaced_by_txid");
}

bool WalletModel::bumpFee(uint256 hash)
{
    std::unique_ptr<CFeeBumper> feeBump;
    {
        CCoinControl coin_control;
        coin_control.signalRbf = true;
        LOCK2(cs_main, wallet->cs_wallet);
        feeBump.reset(new CFeeBumper(wallet, hash, coin_control, 0));
    }
    if (feeBump->getResult() != BumpFeeResult::OK)
    {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
            (feeBump->getErrors().size() ? QString::fromStdString(feeBump->getErrors()[0]) : "") +")");
         return false;
    }

    // allow a user based fee verification
    QString questionString = tr("Do you want to increase the fee?");
    questionString.append("<br />");
    CAmount oldFee = feeBump->getOldFee();
    CAmount newFee = feeBump->getNewFee();
    questionString.append("<table style=\"text-align: left;\">");
    questionString.append("<tr><td>");
    questionString.append(tr("Current fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), oldFee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("Increase:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), newFee - oldFee));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("New fee:"));
    questionString.append("</td><td>");
    questionString.append(BitcoinUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), newFee));
    questionString.append("</td></tr></table>");
    SendConfirmationDialog confirmationDialog(tr("Confirm fee bump"), questionString);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    // cancel sign&broadcast if users doesn't want to bump the fee
    if (retval != QMessageBox::Yes) {
        return false;
    }

    WalletModel::UnlockContext ctx(requestUnlock());
    if (!ctx.isValid())
    {
        return false;
    }

    // sign bumped transaction
    bool res = false;
    {
        LOCK2(cs_main, wallet->cs_wallet);
        res = feeBump->signTransaction(wallet);
    }
    if (!res) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    {
        LOCK2(cs_main, wallet->cs_wallet);
        res = feeBump->commit(wallet);
    }
    if (!res) {
        QMessageBox::critical(0, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(feeBump->getErrors()[0])+")");
         return false;
    }
    return true;
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

bool WalletModel::hdEnabled() const
{
    return wallet->IsHDEnabled();
}

int WalletModel::getDefaultConfirmTarget() const
{
    return nTxConfirmTarget;
}

bool WalletModel::getDefaultWalletRbf() const
{
    return fWalletRbf;
}

bool WalletModel::nameAvailable(const QString &name)
{
    UniValue res, isExpired;

    const std::string strName = name.toStdString();

    UniValue params (UniValue::VOBJ);
    params.pushKV ("name", strName);

    JSONRPCRequest jsonRequest;
    jsonRequest.strMethod = "name_show";
    jsonRequest.params = params;
    jsonRequest.fHelp = false;

    try {
        res = tableRPC.execute(jsonRequest);

    } catch (const UniValue& e) {
        // Make sure we have the correct error response and not something else
        UniValue message = find_value( e, "message");
        std::string errorStr = message.get_str();

        if (errorStr.find("name not found") != std::string::npos)
            return true;

        LogPrintf ("unexpected nameAvailable response: %s\n", errorStr.c_str());
        return false;
    }

    isExpired = find_value( res, "expired");
    if (isExpired.get_bool())
      return true;

    return false;
}

NameNewReturn WalletModel::nameNew(const QString &name)
{
    std::cout << "WalletModel::nameNew\n";

    std::string strName = name.toStdString ();
    std::string data;

    std::vector<UniValue> values;

    UniValue params(UniValue::VOBJ);
    params.pushKV("name", strName);

    JSONRPCRequest jsonRequest;
    jsonRequest.strMethod = "name_new";
    jsonRequest.params = params;
    jsonRequest.fHelp = false;

    NameNewReturn retval;
    UniValue res;
    try {
        std::cout << "name_new\n";
        res = tableRPC.execute(jsonRequest);
    } catch (const UniValue& e) {
        UniValue message = find_value( e, "message");
        std::string errorStr = message.get_str();
        LogPrintf ("nameNew error: %s\n", errorStr.c_str());
        retval.err_msg = errorStr;
        retval.ok = false;
        return retval;
    }

    values = res.getValues();
    const auto txid = values[0];
    const auto  rand = values[1];

    retval.ok = true;
    retval.hex = txid.get_str(); // txid
    retval.rand = rand.get_str();

    return retval;
}

const std::string WalletModel::completePendingNameFirstUpdate(std::string &name, std::string &rand, std::string &txid, std::string &data, std::string &toaddress)
{
    std::string errorStr;

    std::cout << "VALUE " << data << '\n';

    UniValue params(UniValue::VOBJ);
    params.pushKV ("name", name);
    params.pushKV ("rand", rand);
    params.pushKV ("tx", txid);
    params.pushKV ("value", data);
    if (!toaddress.empty())
        params.pushKV ("toaddress", toaddress);

    JSONRPCRequest jsonRequest;
    jsonRequest.strMethod = "name_firstupdate";
    jsonRequest.params = params;
    jsonRequest.fHelp = false;

    LogPrintf("executing name_firstupdate name=%s rand=%s tx=%s value=%s\n",
        name.c_str(), rand.c_str(), txid.c_str(), data.c_str());

    UniValue res;
    try {
        res = tableRPC.execute(jsonRequest);
    }
    catch (const UniValue& e) {
        UniValue message = find_value( e, "message");
        errorStr = message.get_str();
        LogPrintf ("name_firstupdate error: %s\n", errorStr.c_str());
    }

    return errorStr;
}

std::vector<std::string> WalletModel::sendPendingNameFirstUpdates()
{
    std::cout << "Iterating\n";
    std::vector<std::string> successfulNames;

    LOCK(wallet->cs_wallet);
    MapNameNewReturn pendingNameFirstUpdate = wallet->pendingNameFirstUpdate;
    for (MapNameNewReturn::iterator i = pendingNameFirstUpdate.begin();
         i != pendingNameFirstUpdate.end(); i++)
    {
        LogPrintf("I=%i\n",  std::distance( pendingNameFirstUpdate.begin(), i));

        JSONRPCRequest jsonRequest;
        UniValue params1(UniValue::VOBJ);
        UniValue res1, val;
        // hold the error returned from name_firstupdate, via
        // completePendingNameFirstUpdate, or empty on success
        // this will drive the error-handling popup
        std::string completedResult;

        std::string name = i->first;
        std::string txid = i->second.hex;
        std::string rand = i->second.rand;
        std::string data = i->second.data;
        std::string toaddress = i->second.toaddress;

        std::cout << "***** NAME " << name << '\n';
        std::cout << "***** DATA " << data << '\n';

        params1.pushKV ("txid", txid);
        jsonRequest.strMethod = "gettransaction";
        jsonRequest.params = params1;
        jsonRequest.fHelp = false;

        // if we're here, the names doesn't exist
        // should we remove it from the DB?
        try {
            res1 = tableRPC.execute(jsonRequest);
        }
        catch (const UniValue& e) {
            UniValue message = find_value( e, "message");
            std::string errorStr = message.get_str();
            LogPrintf ("gettransaction error for name %s: %s\n",
                       name.c_str(), errorStr.c_str());
            continue;
        }

        val = find_value (res1, "confirmations");
        if (!val.isNum ())
        {
            LogPrintf ("No confirmations for name %s\n", name.c_str());
            continue;
        }

        const int confirms = val.get_int ();
        LogPrintf ("Pending Name FirstUpdate Confirms: %d\n", confirms);

        if (confirms < 12)
        {
            continue;
        }

        if (getEncryptionStatus() == Locked)
        {
            if (QMessageBox::Yes != QMessageBox::question(this,
                  tr("Confirm wallet unlock"),
                  tr("Namecoin Core is about to finalize your name registration for name <b>%1</b>, by sending name_firstupdate. If your wallet is locked, you will be prompted to unlock it. Pressing cancel will delay your name registration by one block, at which point you will be prompted again. Would you like to proceed?").arg(QString::fromStdString(name)),
                  QMessageBox::Yes|QMessageBox::Cancel,
                  QMessageBox::Cancel))
            {
                LogPrintf ("User cancelled wallet unlock pre-name_firstupdate. Waiting 1 block.\n");
                return successfulNames;
            }

            LogPrintf ("Attempting wallet unlock ...\n");
            WalletModel::UnlockContext ctx(this->requestUnlock ());
            if (!ctx.isValid ())
            {
                return successfulNames;
            }
            else
            {
                // NOTE: the reason we're doing this here and not at the same
                // time as the one beneath it is because when ctx is destroyed
                // (subsequently after leaving this block) the wallet is locked.
                completedResult = this->completePendingNameFirstUpdate(name, rand, txid, data, toaddress);
            }
        }
        else
        {
            completedResult = this->completePendingNameFirstUpdate(name, rand, txid, data, toaddress);
        }

        // if we got an error on name_firstupdate. prompt user for what to do
        if (!completedResult.empty())
        {
            std::cout << "NOT EMPTY error response\n";
            QString errorMsg = tr("Namecoin Core has encountered an error while attempting to complete your name registration for name <b>%1</b>. The name_firstupdate operation caused the following error to occurr:<br><br>%2<br><br>Would you like to cancel the pending name registration?")
                .arg(QString::fromStdString(name))
                .arg(QString::fromStdString(completedResult));
            // if they didnt hit yes, move onto next pending op, otherwise
            // the pending transaction will be deleted in the subsequent block
            if (QMessageBox::Yes != QMessageBox::question(this, tr("Name registration error"),
                                                          errorMsg,
                                                          QMessageBox::Yes|QMessageBox::No,
                                                          QMessageBox::No))
            {
                // NOTE: removing this fixes the current bug, could be a lock issue though
                continue;
            }
        }

        successfulNames.push_back(name);
        nameTableModel->updateEntry(QString::fromStdString(name), QString::fromStdString(data), chainActive.Height() + 1, CT_UPDATED);
    }
    return successfulNames;
}

QString WalletModel::nameUpdate(const QString &name, const QString &data, const QString &transferToAddress)
{
    std::string strName = name.toStdString ();
    std::string strData = data.toStdString ();
    std::string strTransferToAddress = transferToAddress.toStdString ();

    UniValue params(UniValue::VOBJ);
    params.pushKV ("name", strName);
    params.pushKV ("value", strData);

    JSONRPCRequest jsonRequest;
    jsonRequest.strMethod = "name_update";
    jsonRequest.params = params;
    jsonRequest.fHelp = false;

    if (strTransferToAddress != "")
        params.pushKV ("toaddress", strTransferToAddress);

    UniValue res;
    try {
        res = tableRPC.execute(jsonRequest);
    }
    catch (const UniValue& e) {
        UniValue message = find_value( e, "message");
        std::string errorStr = message.get_str();
        LogPrintf ("name_update error: %s\n", errorStr.c_str());
        return QString::fromStdString(errorStr);
    }
    return tr ("");
}

bool WalletModel::writePendingNameFirstUpdate(std::string &name, std::string &rand, std::string &txid, std::string &data, std::string &toaddress)
{
    return wallet->WritePendingNameFirstUpdate(name, rand, txid, data, toaddress);
}

bool WalletModel::pendingNameFirstUpdateExists(std::string &name)
{
    return wallet->PendingNameFirstUpdateExists(name);
}

NameNewReturn WalletModel::getPendingNameFirstUpdate(std::string &name)
{
    return wallet->GetPendingNameFirstUpdate(name);
}

