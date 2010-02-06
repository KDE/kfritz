/*
 * KFritz
 *
 * Copyright (C) 2008 Joachim Wilke <vdr@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "KFritzWindow.h"

#include <KApplication>
#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KApplication>
#include <KAction>
#include <KLocale>
#include <KActionCollection>
#include <KAboutData>
#include <KFindDialog>
#include <KStandardAction>
#include <KService>
#include <KStatusBar> //TODO: add status messages to status bar
#include <KConfigSkeleton>
#include <KConfigDialog>
#include <KNotifyConfigWidget>
#include <KPasswordDialog>
#include <QTextCodec>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStackedLayout>

#include <Config.h>
#include <Tools.h>
#include <CallList.h>

#include "KSettings.h"
#include "KSettingsFonbooks.h"
#include "KSettingsFritzBox.h"
#include "Log.h"

KFritzWindow::KFritzWindow()
{
	appName     = KGlobal::mainComponent().aboutData()->appName();
	programName = KGlobal::mainComponent().aboutData()->programName();

	connect(this, SIGNAL(signalNotification(QString, QString, bool)), this, SLOT(slotNotification(QString, QString, bool)));

	initIndicator();
	updateMissedCallsIndicator();

	logArea = new KTextEdit(this);
	logArea->setReadOnly(true);

	fritz::Config::SetupLogging(LogStream::getLogStream(LogBuf::DEBUG)->setLogWidget(logArea),
							    LogStream::getLogStream(LogBuf::INFO)->setLogWidget(logArea),
					            LogStream::getLogStream(LogBuf::ERROR)->setLogWidget(logArea));

	bool savetoWallet = false;
	bool requestPassword = true;

	KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), winId());
	if (wallet) {
		if (wallet->hasFolder(appName)) {
			wallet->setFolder(appName);
			if (wallet->hasEntry(KSettings::hostname())) {
				if (wallet->readPassword(KSettings::hostname(), fbPassword) == 0) {
					DBG("Got password data from KWallet.");
						requestPassword = false;
				}
			} else {
				DBG("No password for this host available.");
			}
		} else {
			DBG("No relevant data in KWallet yet.");
		}
	} else {
		INF("No access to KWallet.");
	}

	if (requestPassword)
		savetoWallet = showPasswordDialog(fbPassword, wallet != NULL);

	libFritzInit = new LibFritzInit(fbPassword, this);

	connect(libFritzInit, SIGNAL(invalidPassword()), this, SLOT(reenterPassword()));

	if (wallet && savetoWallet)
		saveToWallet(wallet);

	setupActions();

	tabWidget = new KTabWidget();

	// init fonbooks, add to tabWidget

	fritz::FonbookManager *fm = fritz::FonbookManager::GetFonbookManager();
	fritz::Fonbooks *books = fm->GetFonbooks();
	for (size_t pos=0; pos<books->size(); pos++) {
		if (!(*books)[pos]->isDisplayable())
			continue;

		KFonbookModel *modelFonbook = new KFonbookModel((*books)[pos]->GetTechId().c_str());
		connect(libFritzInit, SIGNAL(ready(bool)), modelFonbook, SLOT(libReady(bool)));

		QAdaptTreeView *treeFonbook = new QAdaptTreeView(this);
		treeFonbook->setAlternatingRowColors(true);
		treeFonbook->setItemsExpandable(false);
		treeFonbook->setSortingEnabled(true);
		treeFonbook->setModel(modelFonbook);
		treeFonbook->sortByColumn(0, Qt::AscendingOrder); //sort by Name

		treeFonbooks.push_back(treeFonbook);
		//TODO: show only configured ones
		tabWidget->addTab(treeFonbook,  KIcon("x-office-address-book"), 	i18n((*books)[pos]->GetTitle().c_str()));
}

	// init call list, add to tabWidget

	KCalllistModel *modelCalllist;
	modelCalllist = new KCalllistModel();
	connect(libFritzInit, SIGNAL(ready(bool)), modelCalllist, SLOT(libReady(bool)));
	connect(modelCalllist, SIGNAL(updated()), this, SLOT(updateMissedCallsIndicator()));

	QAdaptTreeView *treeCallList = new QAdaptTreeView(this);
	treeCallList->setAlternatingRowColors(true);
	treeCallList->setItemsExpandable(false);
	treeCallList->setSortingEnabled(true);

	treeCallList->setModel(modelCalllist);
	treeCallList->sortByColumn(1, Qt::DescendingOrder); //sort by Date

	tabWidget->addTab(treeCallList, KIcon("application-x-gnumeric"), 	i18n("Call history"));

	// add log to tabWidget, if configured

	showLog(KSettings::showLog());

	setCentralWidget(tabWidget);

	setupGUI();
}

KFritzWindow::~KFritzWindow()
{
	// move logging to console
	fritz::Config::SetupLogging(&std::clog, &std::cout, &std::cerr);

	fritz::FonbookManager::DeleteFonbookManager();
	fritz::CallList::DeleteCallList();

	delete libFritzInit;
}

void KFritzWindow::HandleCall(bool outgoing, int connId __attribute__((unused)), std::string remoteNumber, std::string remoteName, fritz::FonbookEntry::eType type, std::string localParty __attribute__((unused)), std::string medium __attribute__((unused)), std::string mediumName)
{
	QTextCodec *inputCodec  = QTextCodec::codecForName(fritz::CharSetConv::SystemCharacterTable() ? fritz::CharSetConv::SystemCharacterTable() : "UTF-8");
	QString qRemoteName    = inputCodec->toUnicode(remoteName.c_str());
	QString qTypeName      = KFonbookModel::getTypeName(type);

	if (qTypeName.size() > 0)
		qRemoteName += " (" + qTypeName + ")";

	//QString qLocalParty = inputCodec->toUnicode(localParty.c_str());
	QString qMediumName    = inputCodec->toUnicode(mediumName.c_str());
	QString qMessage;
	if (outgoing)
		qMessage=i18n("Outgoing call to <b>%1</b><br/>using %2",   qRemoteName.size() ? qRemoteName : remoteNumber.c_str(),                                    qMediumName);
	else
		qMessage=i18n("Incoming call from <b>%1</b><br/>using %2", qRemoteName.size() ? qRemoteName : remoteNumber.size() ? remoteNumber.c_str() : i18n("unknown"),  qMediumName);

	emit signalNotification(outgoing ? "outgoingCall" : "incomingCall", qMessage, true);
}

void KFritzWindow::HandleConnect(int connId __attribute__((unused)))
{
	if (notification)
		notification->close();
	emit signalNotification("callConnected", "Call connected.", false);
}

void KFritzWindow::HandleDisconnect(int connId __attribute__((unused)), std::string duration)
{
	if (notification)
		notification->close();
	std::stringstream ss(duration);
	int seconds;
	ss >> seconds;

	QString qMessage = i18n("Call disconnected (%1:%2).", seconds/60, seconds%60);
	emit signalNotification("callDisconnected", qMessage, false);
}

void KFritzWindow::slotNotification(QString event, QString qMessage, bool persistent) {
	notification = new KNotification (event, this, persistent ? KNotification::Persistent : KNotification::CloseOnTimeout);
	KIcon ico(event == "incomingCall" ? "incoming-call" :
			  event == "outgoingCall" ? "outgoing-call" :
					                    "internet-telephony");
	notification->setTitle(programName);
	notification->setPixmap(ico.pixmap(64, 64));
	notification->setText(qMessage);
	notification->sendEvent();
	connect(notification, SIGNAL(closed()), this, SLOT(notificationClosed()));
}

void KFritzWindow::notificationClosed() {
	notification = NULL;
}

void KFritzWindow::showSettings() {
	KConfigDialog *confDialog = new KConfigDialog(this, "settings", KSettings::self());

	QWidget *frameFritzBox = new KSettingsFritzBox(this);
    confDialog->addPage(frameFritzBox, i18n("Fritz!Box"), "modem", i18n("Configure connection to Fritz!Box"));

    QWidget *frameFonbooks = new KSettingsFonbooks(this);
	confDialog->addPage(frameFonbooks, i18n("Phone books"), "x-office-address-book", i18n("Select phone books to use"));

	connect(confDialog, SIGNAL(settingsChanged(const QString &)), this, SLOT(updateConfiguration(const QString &)));

	confDialog->show();
}

void KFritzWindow::showNotificationSettings() {
    KNotifyConfigWidget::configure(this);
}

void KFritzWindow::updateConfiguration(const QString &dialogName __attribute__((unused))) {
	libFritzInit->setPassword(fbPassword);
	libFritzInit->start();
}

void KFritzWindow::reenterPassword() {
	KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::LocalWallet(), 0);

	bool keepPassword = showPasswordDialog(fbPassword, wallet != NULL);

	if (wallet && keepPassword)
		saveToWallet(wallet);
	updateConfiguration();
}

void KFritzWindow::saveToWallet(KWallet::Wallet *wallet) {
	DBG("Trying to save password...");
	if (wallet->hasFolder(appName) || wallet->createFolder(appName)) {
		wallet->setFolder(appName);
		if (wallet->writePassword(KSettings::hostname(), fbPassword) == 0) {
			INF("Saved password to KWallet.");
		}
	} else {
		ERR("Error accessing KWallet.")
	}
}

bool KFritzWindow::showPasswordDialog(QString &password, bool offerSaving) {
	KPasswordDialog pwd(this, offerSaving ? KPasswordDialog::ShowKeepPassword : KPasswordDialog::NoFlags);
	pwd.setPrompt(i18n("Enter your Fritz!Box password"));
	pwd.exec();
	password = pwd.password();
	return pwd.keepPassword();
}

void KFritzWindow::setupActions() {
	KAction* aShowSettings = new KAction(this);
	aShowSettings->setText(i18n("Configure KFritz..."));
	aShowSettings->setIcon(KIcon("preferences-other"));
	actionCollection()->addAction("showSettings", aShowSettings);
	connect(aShowSettings, SIGNAL(triggered(bool)), this, SLOT(showSettings()));

	KAction* aShowNotifySettings = new KAction(this);
	aShowNotifySettings->setText(i18n("Configure Notifications..."));
	aShowNotifySettings->setIcon(KIcon("preferences-desktop-notification"));
	actionCollection()->addAction("showNotifySettings", aShowNotifySettings);
	connect(aShowNotifySettings, SIGNAL(triggered(bool)), this, SLOT(showNotificationSettings()));

	KAction *aShowLog = new KAction(this);
	aShowLog->setText(i18n("Show log"));
	aShowLog->setIcon(KIcon("text-x-log"));
	aShowLog->setCheckable(true);
	aShowLog->setChecked(KSettings::showLog());
	actionCollection()->addAction("showLog", aShowLog);
	connect(aShowLog, SIGNAL(triggered(bool)), this, SLOT(showLog(bool)));

	KStandardAction::quit(kapp, SLOT(quit()), actionCollection());

//	KStandardAction::find(this, SLOT(find()), actionCollection());
//	KStandardAction::findNext(this, SLOT(findNext()), actionCollection());
//	KStandardAction::findPrev(this, SLOT(findPrev()), actionCollection());
}

void KFritzWindow::initIndicator() {
	QIndicate::Server *iServer = NULL;
	missedCallsIndicator = NULL;
#ifdef INDICATEQT_FOUND
	iServer = QIndicate::Server::defaultInstance();
	iServer->setType("message.irc");
	KService::Ptr service = KService::serviceByDesktopName(appName);
	if (service)
		iServer->setDesktopFile(service->entryPath());
	iServer->show();

	missedCallsIndicator = new QIndicate::Indicator(iServer);
	missedCallsIndicator->setNameProperty("Missed calls");
	missedCallsIndicator->setDrawAttentionProperty(true);

	connect(iServer, SIGNAL(serverDisplay()), this, SLOT(showMainWindow()));
	connect(missedCallsIndicator, SIGNAL(display(QIndicate::Indicator*)), this, SLOT(showMissedCalls(QIndicate::Indicator*)));
#endif
}

bool KFritzWindow::queryClose() {
	// don't quit, just minimize to systray
	hide();
	return false;
}


void KFritzWindow::updateMissedCallsIndicator() {
	fritz::CallList *callList = fritz::CallList::getCallList(false);
	size_t missedCallCount = 0;
	if (callList) {
		for (size_t pos = 0; pos < callList->GetSize(fritz::CallEntry::MISSED); pos++)
			if (KSettings::lastKnownMissedCall() < callList->RetrieveEntry(fritz::CallEntry::MISSED, pos)->timestamp)
				missedCallCount++;
	}
#ifdef INDICATEQT_FOUND
	missedCallsIndicator->setCountProperty(missedCallCount);
	if (missedCallCount == 0)
		missedCallsIndicator->hide();
	else
		missedCallsIndicator->show();
#endif
}

void KFritzWindow::find() {
	QStringList findStrings;
	KFindDialog dlg(this, 0, findStrings, true, false);
	if (dlg.exec() != QDialog::Accepted)
		return;

}

void KFritzWindow::findNext() {

}

void KFritzWindow::findPrev() {

}

void KFritzWindow::showMainWindow() {
	this->show();
}

void KFritzWindow::showMissedCalls(QIndicate::Indicator* indicator __attribute__((unused))) {
	tabWidget->setCurrentIndex(1);
	this->show();
	if (missedCallsIndicator)
		missedCallsIndicator->hide();

	fritz::CallList *callList = fritz::CallList::getCallList(false);
	KSettings::setLastKnownMissedCall(callList->LastMissedCall());
	KSettings::self()->writeConfig();
	updateMissedCallsIndicator();
}

void KFritzWindow::showLog(bool b) {
	if (b) {
		tabWidget->addTab(logArea, KIcon("text-x-log"), i18n("Log"));
		logArea->show();
	} else {
		tabWidget->removePage(logArea);
		logArea->hide();
	}
	KSettings::setShowLog(b);
	KSettings::self()->writeConfig();
}
