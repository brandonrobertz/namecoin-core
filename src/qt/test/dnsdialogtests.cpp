#include "dnsdialogtests.h"

#include "qt/optionsmodel.h"
#include "qt/platformstyle.h"
#include "qt/walletmodel.h"
#include "rpc/server.h"
#include "test/test_bitcoin.h"
#include "wallet/test/wallet_test_fixture.h"
#include "wallet/wallet.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QListView>
#include <QDialogButtonBox>

// NOTE: This gets created by the WalletTestingSetup class
// from the regular bitcoin wallet tests
extern CWallet* pwalletMain;

namespace
{

void TestDNSSpecSerialization()
{
    // Utilize the normal testsuite setup (we have no fixtures in Qt tests
    // so we have to do it like this).
    WalletTestingSetup testSetup(CBaseChainParams::REGTEST);

    // The Qt/wallet testing manifolds don't appear to instantiate the wallets
    // correctly for multi-wallet bitcoin so this is a hack in place until that
    // happens
    vpwallets.insert(vpwallets.begin(), pwalletMain);

    bool firstRun;
    pwalletMain->LoadWallet(firstRun);

    // Set up wallet and chain with 105 blocks (5 mature blocks for spending).
    GenerateCoins(105);
    CWalletDB(pwalletMain->GetDBHandle()).LoadWallet(pwalletMain);
    RegisterWalletRPCCommands(tableRPC);

    // Create widgets for interacting with the names UI
    std::unique_ptr<const PlatformStyle> platformStyle(PlatformStyle::instantiate("other"));
    OptionsModel optionsModel;
    WalletModel walletModel(platformStyle.get(), pwalletMain, &optionsModel);

}

}

void DNSDialogTests::dnsDialogTests()
{
    TestDNSSpecSerialization();
}
