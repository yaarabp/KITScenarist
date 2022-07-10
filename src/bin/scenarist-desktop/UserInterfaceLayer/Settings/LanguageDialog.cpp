#include "LanguageDialog.h"
#include "ui_LanguageDialog.h"

using UserInterface::LanguageDialog;


LanguageDialog::LanguageDialog(QWidget *parent, int _language) :
    QLightBoxDialog(parent),
    m_ui(new Ui::LanguageDialog)
{
    m_ui->setupUi(this);

    QRadioButton* languageButton = nullptr;
    switch (_language) {
        case -1: languageButton = m_ui->system; break;
        case 0: languageButton = m_ui->russian; break;
        case 1: languageButton = m_ui->spanish; break;
        case 2: languageButton = m_ui->english; break;
        case 3: languageButton = m_ui->french; break;
        case 4: languageButton = m_ui->kazakh; break;
        case 5: languageButton = m_ui->ukrainian; break;
        case 6: languageButton = m_ui->german; break;
        case 7: languageButton = m_ui->portugues; break;
        case 8: languageButton = m_ui->farsi; break;
        case 9: languageButton = m_ui->chinese; break;
        case 10: languageButton = m_ui->hebrew; break;
        case 11: languageButton = m_ui->polish; break;
        case 12: languageButton = m_ui->turkish; break;
        case 13: languageButton = m_ui->hungarian; break;
        case 14: languageButton = m_ui->italian; break;
        case 15: languageButton = m_ui->azerbaijani; break;
        case 16: languageButton = m_ui->telugu; break;
        case 17: languageButton = m_ui->portuguesBrasil; break;
        case 18: languageButton = m_ui->slovenian; break;
        case 19: languageButton = m_ui->swedish; break;
        case 20: languageButton = m_ui->nederlands; break;
        case 21: languageButton = m_ui->serbian; break;
        case 22: languageButton = m_ui->arabic; break;
        case 23: languageButton = m_ui->greek; break;
        case 24: languageButton = m_ui->galician; break;
        case 25: languageButton = m_ui->danish; break;
        case 26: languageButton = m_ui->belarusian; break;
        case 27: languageButton = m_ui->cambodian; break;
        case 28: languageButton = m_ui->croatian; break;
        default: break;
    }

    if (languageButton != nullptr) {
        languageButton->setChecked(true);
    }
}

LanguageDialog::~LanguageDialog()
{
    delete m_ui;
}

int LanguageDialog::language() const
{
    int result = -1;
    if (m_ui->system->isChecked()) {
        result = -1;
    } else if (m_ui->russian->isChecked()) {
        result = 0;
    } else if (m_ui->spanish->isChecked()) {
        result = 1;
    } else if (m_ui->english->isChecked()) {
        result = 2;
    } else if (m_ui->french->isChecked()) {
        result = 3;
    } else if (m_ui->kazakh->isChecked()) {
        result = 4;
    } else if (m_ui->ukrainian->isChecked()) {
        result = 5;
    } else if (m_ui->german->isChecked()) {
        result = 6;
    } else if (m_ui->portugues->isChecked()) {
        result = 7;
    } else if (m_ui->farsi->isChecked()) {
        result = 8;
    } else if (m_ui->chinese->isChecked()) {
        result = 9;
    } else if (m_ui->hebrew->isChecked()) {
        result = 10;
    } else if (m_ui->polish->isChecked()) {
        result = 11;
    } else if (m_ui->turkish->isChecked()) {
        result = 12;
    } else if (m_ui->hungarian->isChecked()) {
        result = 13;
    } else if (m_ui->italian->isChecked()) {
        result = 14;
    } else if (m_ui->azerbaijani->isChecked()) {
        result = 15;
    } else if (m_ui->telugu->isChecked()) {
        result = 16;
    } else if (m_ui->portuguesBrasil->isChecked()) {
        result = 17;
    } else if (m_ui->slovenian->isChecked()) {
        result = 18;
    } else if (m_ui->swedish->isChecked()) {
        result = 19;
    } else if (m_ui->nederlands->isChecked()) {
        result = 20;
    } else if (m_ui->serbian->isChecked()) {
        result = 21;
    } else if (m_ui->arabic->isChecked()) {
        result = 22;
    } else if (m_ui->greek->isChecked()) {
        result = 23;
    } else if (m_ui->galician->isChecked()) {
        result = 24;
    } else if (m_ui->danish->isChecked()) {
        result = 25;
    } else if (m_ui->belarusian->isChecked()) {
        result = 26;
    } else if (m_ui->cambodian->isChecked()) {
        result = 27;
    } else if (m_ui->croatian->isChecked()) {
        result = 28;
    }

    return result;
}

void LanguageDialog::initView()
{
}

void LanguageDialog::initConnections()
{
    connect(m_ui->buttons, SIGNAL(accepted()), this, SLOT(accept()));
}
