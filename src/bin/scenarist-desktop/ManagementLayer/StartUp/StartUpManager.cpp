/*
* Copyright (C) 2014 Dimka Novikov, to@dimkanovikov.pro
* Copyright (C) 2016 Alexey Polushkin, armijo38@yandex.ru
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* Full license: http://dimkanovikov.pro/license/GPLv3
*/

#include "StartUpManager.h"

#include <ManagementLayer/Project/ProjectsManager.h>

#include <UserInterfaceLayer/StartUp/StartUpView.h>
#include <UserInterfaceLayer/Application/CrashReportDialog.h>
#include <UserInterfaceLayer/Application/UpdateDialog.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <3rd_party/Helpers/PasswordStorage.h>
#include <3rd_party/Helpers/TextEditHelper.h>
#include <3rd_party/Widgets/QLightBoxWidget/qlightboxmessage.h>

#include <NetworkRequest.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMenu>
#include <QMutableMapIterator>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStandardPaths>
#include <QTimer>
#include <QXmlStreamReader>

using ManagementLayer::StartUpManager;
using ManagementLayer::ProjectsManager;
using DataStorageLayer::StorageFacade;
using DataStorageLayer::SettingsStorage;
using UserInterface::StartUpView;
using UserInterface::CrashReportDialog;
using UserInterface::UpdateDialog;

namespace {
    QUrl UPDATE_URL = QString("https://kitscenarist.ru/api/app/updates/");
}


StartUpManager::StartUpManager(QObject *_parent, QWidget* _parentWidget) :
    QObject(_parent),
    m_view(new StartUpView(_parentWidget))
{
    initConnections();
}

QWidget* StartUpManager::view() const
{
    return m_view;
}

void StartUpManager::checkCrashReports()
{
    const QString sendedSuffix = "sended";
    const QString ignoredSuffix = "ignored";

    //
    // Настроим отлавливание ошибок
    //
    QString appDataFolderPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QString crashReportsFolderPath = appDataFolderPath + QDir::separator() + "CrashReports";
    QVector<QString> unhandledReportsPaths;
    for (const QFileInfo& crashReportFileInfo : QDir(crashReportsFolderPath).entryInfoList(QDir::Files)) {
        if (crashReportFileInfo.suffix() != sendedSuffix
            && crashReportFileInfo.suffix() != ignoredSuffix) {
            unhandledReportsPaths.append(crashReportFileInfo.filePath());
        }
    }

    //
    // Если есть необработанные отчёты, показать диалог
    //
    if (!unhandledReportsPaths.isEmpty()) {
        CrashReportDialog dialog(m_view);

        //
        // Возьмем email из хранилища (тот же, что и для авторизации)
        //
        const QString email =
                StorageFacade::settingsStorage()->value(
                    "application/email",
                    SettingsStorage::ApplicationSettings);
        if (!email.isEmpty()) {
            dialog.setEmail(email);
        }

        bool handled = true;
        if (dialog.exec() == CrashReportDialog::Accepted) {
            dialog.showProgress();
            for (const auto& reportPath : unhandledReportsPaths) {
                //
                // Отправляем
                //
                NetworkRequest loader;
                loader.setRequestMethod(NetworkRequestMethod::Post);
                loader.addRequestAttribute("version", QApplication::applicationVersion());
                loader.addRequestAttribute("email", dialog.email());
                loader.addRequestAttribute("message", dialog.message());
                loader.addRequestAttributeFile("report", reportPath);
                loader.loadSync("https://kitscenarist.ru/api/app/feedback/");
            }

            //
            // Сохраняем email, если ранее не было никакого
            //
            if (email.isEmpty()) {
                StorageFacade::settingsStorage()->setValue(
                            "application/email",
                            dialog.email(),
                            SettingsStorage::ApplicationSettings);
            }
        } else {
            handled = false;
        }

        //
        // Помечаем отчёты, чтобы в слдующий раз на них не обращать внимания
        //
        for (auto& reportPath : unhandledReportsPaths) {
            QFile::rename(reportPath, reportPath +  "." + (handled ? sendedSuffix : ignoredSuffix));
        }
    }
}

void StartUpManager::checkNewVersion()
{
    NetworkRequest loader;
    loader.setRequestMethod(NetworkRequestMethod::Post);

    //
    // Сформируем uuid для приложения, по которому будем идентифицировать данного пользователя
    //
    QString uuid
            = DataStorageLayer::StorageFacade::settingsStorage()->value(
                  "application/uuid", DataStorageLayer::SettingsStorage::ApplicationSettings);
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                "application/uuid", uuid, DataStorageLayer::SettingsStorage::ApplicationSettings);

    //
    // Построим ссылку, чтобы учитывать запрос на проверку обновлений
    //

    loader.addRequestAttribute("system_type",
#ifdef Q_OS_WIN
                "windows"
#elif defined Q_OS_LINUX
                "linux"
#elif defined Q_OS_MAC
                "mac"
#else
                QSysInfo::kernelType()
#endif
                );

    loader.addRequestAttribute("system_name", QSysInfo::prettyProductName().toUtf8().toPercentEncoding());
    loader.addRequestAttribute("uuid", uuid);
    loader.addRequestAttribute("application_version", QApplication::applicationVersion());

    QByteArray response = loader.loadSync(UPDATE_URL);

    if (!response.isEmpty()) {
        QXmlStreamReader responseReader(response);

        const int currentLang =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "application/language",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt();
        QString needLang;
        if (currentLang == 0
                || (currentLang == -1
                    && QLocale().language() == QLocale::Russian)) {
            needLang = "ru";
        } else {
            needLang = "en";
        }

        //
        // Распарсим ответ. Нам нужна версия, ее описание, шаблон на скачивание и является ли бетой
        //
        int funded = 0;
        while (!responseReader.atEnd()) {
            responseReader.readNext();
            if (responseReader.name().toString() == "update"
                    && responseReader.tokenType() == QXmlStreamReader::StartElement) {
                QXmlStreamAttributes attributes = responseReader.attributes();
                m_updateVersion = attributes.value("version").toString();
                m_updateFileTemplate = attributes.value("file_template").toString();
                m_updateIsBeta = attributes.value("is_beta").toString() == "true"; // :)
                if (attributes.hasAttribute("funded")) {
                    funded = attributes.value("funded").toInt();
                }
                responseReader.readNext();
            } else if (responseReader.name().toString() == "description"
                    && responseReader.tokenType() == QXmlStreamReader::StartElement) {
                QString lang = responseReader.attributes().value("language").toString();
                if (lang == needLang) {
                    //
                    // Либо русская локаль и русский текст, либо нерусская локаль и нерусский текст
                    //
                    responseReader.readNext();
                    responseReader.readNext();
                    m_updateDescription = TextEditHelper::fromHtmlEscaped(responseReader.text().toString());
                }
                responseReader.readNext();
            }
        }

        m_view->setCrowdfundingVisible(funded > 0, funded);

        //
        // Загрузим версию, либо которая установлена, либо которую пропустили
        //
        QString prevVersion =
                    DataStorageLayer::StorageFacade::settingsStorage()->value(
                        "application/latest_version",
                        DataStorageLayer::SettingsStorage::ApplicationSettings);

        if (m_updateVersion != QApplication::applicationVersion()
            && m_updateVersion != prevVersion
            && !QLightBoxWidget::hasOpenedWidgets()) {
            //
            // Есть новая версия, которая не совпадает с нашей. Покажем диалог
            //
            showUpdateDialogImpl();
        }


        if (QApplication::applicationVersion() != m_updateVersion) {
            //
            // Если оказались здесь, значит либо отменили установку, либо пропустили обновление.
            // Будем показывать пользователю кнопку-ссылку на обновление
            //
            emit updatePublished(m_updateVersion);
        }
    } else {
        m_view->setCrowdfundingVisible(true, 0);
    }
}

bool StartUpManager::isOnLocalProjectsTab() const
{
    return m_view->isOnLocalProjectsTab();
}

void StartUpManager::setRecentProjects(QAbstractItemModel* _model)
{
    m_view->setRecentProjects(_model);
}

void StartUpManager::setRecentProjectName(int _index, const QString& _name)
{
    m_view->setRecentProjectName(_index, _name);
}

void StartUpManager::setRemoteProjectsVisible(bool _visible)
{
    m_view->setRemoteProjectsVisible(_visible);
}

void StartUpManager::setRemoteProjects(QAbstractItemModel* _model)
{
    m_view->setRemoteProjects(_model);
}

void StartUpManager::setRemoteProjectName(int _index, const QString& _name)
{
    m_view->setRemoteProjectName(_index, _name);
}

void StartUpManager::showUpdateDialog()
{
    showUpdateDialogImpl();
}

#ifdef Q_OS_MAC
void StartUpManager::buildEditMenu(QMenu* _menu)
{
    auto* undo = _menu->addAction(tr("Undo"));
    undo->setShortcut(QKeySequence::Undo);
    undo->setEnabled(false);
    auto* redo = _menu->addAction(tr("Redo"));
    redo->setShortcut(QKeySequence::Redo);
    redo->setEnabled(false);
}
#endif

void StartUpManager::showUpdateDialogImpl()
{
    UpdateDialog dialog(m_view);

    bool isSupported = true;

    connect(&dialog, &UpdateDialog::skipUpdate, [this] {
        StorageFacade::settingsStorage()->setValue(
                    "application/latest_version",
                    m_updateVersion,
                    SettingsStorage::ApplicationSettings);
    });
    connect(&dialog, &UpdateDialog::downloadUpdate, [this] {
        downloadUpdate(m_updateFileTemplate);
    });

    connect(this, &StartUpManager::downloadProgressForUpdate, &dialog, &UpdateDialog::setProgressValue);
    connect(this, &StartUpManager::downloadFinishedForUpdate, &dialog, &UpdateDialog::downloadFinished);
    connect(this, &StartUpManager::errorDownloadForUpdate, &dialog, &UpdateDialog::showDownloadError);

#ifdef Q_OS_LINUX
    isSupported = false;
#endif

    //
    // Для версий основанных на старых сборках отключаем автоматическую установку обновлений
    //
    if (QApplication::applicationVersion().contains("granny")) {
        isSupported = false;
    }

    //
    // Покажем окно с обновлением
    //
    bool needToShowUpdate = true;
    QString updateDialogText = m_updateDescription;
    do {
        const int showResult =
                dialog.showUpdate(m_updateVersion, updateDialogText, m_updateIsBeta, isSupported);
        if (showResult == UpdateDialog::Accepted) {
            //
            // Нажали "Установить"
            //
#ifdef Q_OS_WIN
            if (QProcess::startDetached(m_updateFile)) {
#else
            if (QFile::exists(m_updateFile) && QDesktopServices::openUrl(QUrl::fromLocalFile(m_updateFile))) {
#endif
                QApplication::quit();
                return;
            } else {
                updateDialogText = tr("<p>Can't install update. There are some problems with downloaded file.</p>"
                                      "<p>You can try to reload update or load it manually "
                                      "from <a href=\"%1\" style=\"color:#2b78da;\">official website</a>.</p>")
                                   .arg(makeUpdateUrl(m_updateFileTemplate));
            }
        } else {
            needToShowUpdate = false;
        }
    } while (needToShowUpdate);

    //
    // Отменили или пропустили. Остановим загрузку
    //
    emit stopDownloadForUpdate();
}

QString StartUpManager::makeUpdateUrl(const QString& _fileTemplate)
{
    //
    // URL до новой версии в соответствии с ОС, архитектурой и языком
    //
#ifdef Q_OS_WIN
    QString updateUrl = QString("windows/%1.exe").arg(_fileTemplate);
#elif defined Q_OS_LINUX
    #ifdef Q_PROCESSOR_X86_32
        QString arch = "_i386";
    #else
        QString arch = "_amd64";
    #endif

    QString updateUrl = QString("linux/%1%2.deb").arg(_fileTemplate, arch);
#elif defined Q_OS_MAC
    QString updateUrl = QString("mac/%1.dmg").arg(_fileTemplate);
#endif

    //
    // Полная ссылка
    //
    const QString prefixUrl = "https://kitscenarist.ru/downloads/";
    return prefixUrl + updateUrl;
}

void StartUpManager::downloadUpdate(const QString& _fileTemplate)
{
    NetworkRequest loader;

    connect(&loader, &NetworkRequest::downloadProgress, this, &StartUpManager::downloadProgressForUpdate);
    connect(this, &StartUpManager::stopDownloadForUpdate, &loader, &NetworkRequest::stop);

    loader.setRequestMethod(NetworkRequestMethod::Post);
    loader.clearRequestAttributes();

    //
    // Загружаем установщик
    //
    const QUrl updateInfoUrl(makeUpdateUrl(_fileTemplate));
    QByteArray response = loader.loadSync(updateInfoUrl);
    if (response.isEmpty()) {
        emit errorDownloadForUpdate(updateInfoUrl.toString());
        return;
    }

    //
    // Сохраняем установщик в файл
    //
    const QString tempDirPath = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    m_updateFile = tempDirPath + QDir::separator() + updateInfoUrl.fileName();
    QFile tempFile(m_updateFile);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write(response);
        tempFile.close();
        emit downloadFinishedForUpdate();
    }
}

void StartUpManager::initConnections()
{
    connect(m_view, &StartUpView::createProjectClicked, this, &StartUpManager::createProjectRequested);
    connect(m_view, &StartUpView::openProjectClicked, this, &StartUpManager::openProjectRequested);
    connect(m_view, &StartUpView::helpClicked, this, &StartUpManager::helpRequested);
    connect(m_view, &StartUpView::crowdfundingJoinClicked, this, &StartUpManager::crowdfindingJoinRequested);
    connect(m_view, &StartUpView::updateRequested, this, &StartUpManager::showUpdateDialogImpl);

    connect(m_view, &StartUpView::openRecentProjectClicked, this, &StartUpManager::openRecentProjectRequested);
    connect(m_view, &StartUpView::hideRecentProjectRequested, this, &StartUpManager::hideRecentProjectRequested);
    connect(m_view, &StartUpView::moveToCloudRecentProjectRequested, this, &StartUpManager::moveToCloudRecentProjectRequested);
    connect(m_view, &StartUpView::openRemoteProjectClicked, this, &StartUpManager::openRemoteProjectRequested);
    connect(m_view, &StartUpView::editRemoteProjectRequested, this, &StartUpManager::editRemoteProjectRequested);
    connect(m_view, &StartUpView::removeRemoteProjectRequested, this, &StartUpManager::removeRemoteProjectRequested);
    connect(m_view, &StartUpView::shareRemoteProjectRequested, this, &StartUpManager::shareRemoteProjectRequested);
    connect(m_view, &StartUpView::unshareRemoteProjectRequested, this, &StartUpManager::unshareRemoteProjectRequested);
    connect(m_view, &StartUpView::refreshProjects, this, &StartUpManager::refreshProjectsRequested);
}
