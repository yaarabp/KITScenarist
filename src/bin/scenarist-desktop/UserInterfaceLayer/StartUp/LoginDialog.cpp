#include "LoginDialog.h"
#include "ui_LoginDialog.h"

#include "QRegExpValidator"
#include <3rd_party/Widgets/TabBarExpanded/TabBarExpanded.h>
#include <3rd_party/Widgets/PasswordLineEdit/PasswordLineEdit.h>

using UserInterface::LoginDialog;

LoginDialog::LoginDialog(QWidget* _parent) :
	QLightBoxDialog(_parent),
    ui(new Ui::LoginDialog),
    m_tabs(new TabBarExpanded(this)),
    loginPasswordEdit(new PasswordLineEdit(this)),
    signUpPasswordEdit(new PasswordLineEdit(this))
{
	ui->setupUi(this);

	initView();
	initConnections();


    isVerify = false;
}

LoginDialog::~LoginDialog()
{
	delete ui;
}

QString LoginDialog::loginEmail() const
{
    return ui->loginEmail->text();
}

QString LoginDialog::signUpEmail() const
{
    return ui->signUpEmail->text();
}

QString LoginDialog::loginPassword() const
{
    return loginPasswordEdit->text();
}

QString LoginDialog::signUpPassword() const
{
    return signUpPasswordEdit->text();
}

void LoginDialog::setLoginError(const QString& _error)
{
    ui->loginError->setStyleSheet("QLabel { color : red; }");
    ui->loginError->setText(_error);
    ui->loginError->show();
    unblock();
}

void LoginDialog::setSignUpError(const QString &_error)
{
    ui->signUpError->setText(_error);
    ui->signUpError->show();
    unblock();
}

void LoginDialog::setVerificationError(const QString &_error)
{
    ui->verificationError->setStyleSheet("QLabel { color : red; }");
    ui->verificationError->setText(_error);
    ui->verificationError->show();
    unblock();
}

QString LoginDialog::signUpType() const
{
    if(ui->startButton->isChecked()) {
        return tr("start");
    } else if(ui->basicButton->isChecked()) {
        return tr("basic");
    } else if(ui->maximalButton->isChecked()) {
        return tr("maximal");
    } else return tr("unexpected");
}

QString LoginDialog::verificationCode() const
{
    return ui->verificationCode->text();
}

void LoginDialog::checkCode()
{
    QRegExpValidator validator(QRegExp("[0-9]{5}"));
    QString s = ui->verificationCode->text();
    int pos = 0;
    if(validator.validate(s, pos) == QValidator::Acceptable) {
        block();
        emit verify();
    }
}

void LoginDialog::showVerification()
{
    isVerify = true;
    ui->verificationError->setStyleSheet("QLabel { color : green; }");
    ui->verificationError->setText(tr("your e-mail \"%1\" was sent a letter "
                                      "with a confirmation code").arg(ui->signUpEmail->text()));
    ui->verificationError->show();
    ui->stackedWidget->setCurrentWidget(ui->verificationPage);

    unblock();
}

void LoginDialog::showRestore()
{
    ui->loginError->setStyleSheet("QLabel { color : green; }");
    ui->loginError->setText(tr("your e-mail \"%1\" was sent a letter "
                                      "with a password").arg(ui->loginEmail->text()));
    ui->loginError->show();

    unblock();
}

void LoginDialog::setLoginPage()
{
    isVerify = false;
    m_tabs->setCurrentIndex(0);
}

void LoginDialog::clear()
{
    ui->loginEmail->clear();
    loginPasswordEdit->clear();
    ui->signUpEmail->clear();
    signUpPasswordEdit->clear();
    ui->verificationError->clear();

    m_tabs->setCurrentIndex(0);
    ui->basicButton->setChecked(true);
    switchWidget();

    isVerify = false;

    ui->loginError->clear();
    ui->signUpError->clear();
    ui->verificationError->clear();
}

QWidget* LoginDialog::focusedOnExec() const
{
    return ui->loginEmail;
}

void LoginDialog::initView()
{
    ui->loginError->hide();
    ui->loginButtons->addButton(tr("Login"), QDialogButtonBox::AcceptRole);

    ui->signUpError->hide();
    ui->signUpButtons->addButton(tr("Sign Up"), QDialogButtonBox::AcceptRole);

    ui->verificationError->hide();

    //
    // Красивые чекбоксы
    //
    m_tabs->addTab(tr("Login"));
    m_tabs->addTab(tr("Sign Up"));
    m_tabs->setProperty("inTopPanel", true);
    ui->stackLayout->insertWidget(0, m_tabs);

    //
    // Красивые строки ввода пароля
    //
    ui->loginMainLayout->insertWidget(4, loginPasswordEdit);
    ui->loginPasswordLabel->setBuddy(loginPasswordEdit);

    ui->signUpMainLayout->insertWidget(4, signUpPasswordEdit);
    ui->signUpPasswordLabel->setBuddy(signUpPasswordEdit);

    QLightBoxDialog::initView();
}

void LoginDialog::initConnections()
{
    connect(ui->loginButtons, SIGNAL(accepted()), this, SLOT(block()));
    connect(ui->loginButtons, SIGNAL(accepted()), this, SIGNAL(login()));

    connect(ui->signUpButtons, SIGNAL(accepted()), this, SLOT(block()));
    connect(ui->signUpButtons, SIGNAL(accepted()), this, SIGNAL(signUp()));

    connect(ui->loginButtons, SIGNAL(rejected()), this, SLOT(hide()));
    connect(ui->signUpButtons, SIGNAL(rejected()), this, SLOT(hide()));
    connect(ui->ButtonsVerification, SIGNAL(rejected()), this, SLOT(cancelVerify()));
    connect(ui->ButtonsVerification, SIGNAL(rejected()), this, SLOT(hide()));

    connect(m_tabs, SIGNAL(currentChanged(int)), this, SLOT(switchWidget()));
    connect(ui->restorePassword, SIGNAL(clicked(bool)), this, SIGNAL(restore()));

    connect(ui->verificationCode, SIGNAL(textChanged(QString)), this, SLOT(checkCode()));

	QLightBoxDialog::initConnections();
}

void LoginDialog::block()
{
    m_tabs->setEnabled(false);

    ui->stackedWidget->setEnabled(false);

    showProgress();
}

void LoginDialog::unblock()
{
    m_tabs->setEnabled(true);

    ui->stackedWidget->setEnabled(true);

    hideProgress();
}

void LoginDialog::cancelVerify()
{
    isVerify = false;
    switchWidget();
}

void LoginDialog::switchWidget()
{
    if(m_tabs->currentIndex() == 0) {
        ui->stackedWidget->setCurrentWidget(ui->loginPage);
    }
    else if(isVerify) {
        ui->stackedWidget->setCurrentWidget(ui->verificationPage);
    }
    else {
        ui->stackedWidget->setCurrentWidget(ui->signUpPage);
    }
}
