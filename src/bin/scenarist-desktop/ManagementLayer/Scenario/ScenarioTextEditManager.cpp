#include "ScenarioTextEditManager.h"

#include <BusinessLayer/ScenarioDocument/ScenarioDocument.h>
#include <BusinessLayer/ScenarioDocument/ScenarioModelItem.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>
#include <BusinessLayer/ScenarioDocument/ScenarioTextDocument.h>

#include <3rd_party/Widgets/QLightBoxWidget/qlightboxinputdialog.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <UserInterfaceLayer/Scenario/ScenarioTextEdit/ScenarioTextEditWidget.h>

using ManagementLayer::ScenarioTextEditManager;
using BusinessLogic::ScenarioDocument;
using UserInterface::ScenarioTextEditWidget;

namespace {
    /**
     * @brief Перевести тип блока из представления модели в представление стиля блока
     */
    static int mapItemTypeFromModelToBlock(int _itemType) {
        int mappedType = BusinessLogic::ScenarioBlockStyle::SceneHeading;
        if (_itemType == BusinessLogic::ScenarioModelItem::Folder) {
            mappedType = BusinessLogic::ScenarioBlockStyle::FolderHeader;
        }

        return mappedType;
    }
}


ScenarioTextEditManager::ScenarioTextEditManager(QObject* _parent, QWidget* _parentWidget) :
    QObject(_parent),
    m_view(new ScenarioTextEditWidget(_parentWidget))
{
    initView();
    initConnections();
    reloadTextEditSettings();
}

QWidget* ScenarioTextEditManager::toolbar() const
{
    return m_view->toolbar();
}

QWidget* ScenarioTextEditManager::view() const
{
    return m_view;
}

void ScenarioTextEditManager::setScenarioDocument(BusinessLogic::ScenarioTextDocument* _document, bool _isDraft)
{
    if (m_view->scenarioDocument() != _document) {
        m_view->setScenarioDocument(_document, _isDraft);
        reloadTextEditSettings();
    }
}

void ScenarioTextEditManager::setDuration(const QString& _duration)
{
    m_view->setDuration(_duration);
}

void ScenarioTextEditManager::setCountersInfo(const QStringList& _counters)
{
    m_view->setCountersInfo(_counters);
}

void ScenarioTextEditManager::setCursorPosition(int _position)
{
    m_view->setCursorPosition(_position);
}

void ScenarioTextEditManager::reloadTextEditSettings()
{
    m_view->setUsePageView(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/page-view",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setShowScenesNumbers(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/show-scenes-numbers",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setShowDialoguesNumbers(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/show-dialogues-numbers",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setHighlightBlocks(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/highlight-blocks",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setHighlightCurrentLine(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/highlight-current-line",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setAutoReplacing(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/capitalize-first-word",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt(),
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/correct-double-capitals",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt(),
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/replace-three-dots",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt(),
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/smart-quotes",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setUseSpellChecker(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/spell-checking",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());
    m_view->setSpellCheckLanguage(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/spell-checking-language",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());

    //
    // Цветовая схема
    //
    const bool useDarkTheme =
            DataStorageLayer::StorageFacade::settingsStorage()->value(
                "application/use-dark-theme",
                DataStorageLayer::SettingsStorage::ApplicationSettings)
            .toInt();
    const QString colorSuffix = useDarkTheme ? "-dark" : "";
    m_view->setTextEditColors(
                QColor(
                    DataStorageLayer::StorageFacade::settingsStorage()->value(
                        "scenario-editor/text-color" + colorSuffix,
                        DataStorageLayer::SettingsStorage::ApplicationSettings)
                    ),
                QColor(
                    DataStorageLayer::StorageFacade::settingsStorage()->value(
                        "scenario-editor/background-color" + colorSuffix,
                        DataStorageLayer::SettingsStorage::ApplicationSettings)
                    )
                );

    m_view->setTextEditZoomRange(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/zoom-range",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toDouble());

    m_view->setShowAutocompletionInEmptyBlocks(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/show-suggestions-in-empty-blocks-2",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());

    m_view->setAutocompleteNextCharacterForDialogue(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/show-suggestions-in-empty-blocks",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt());

    //
    // Настраиваем коррекции текста
    //
    BusinessLogic::ScenarioTextDocument* script = m_view->scenarioDocument();
    if (script != nullptr) {
        const bool continueDialogues =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/auto-continue-dialogue",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt();
        const bool correctTextOnPageBreaks =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    "scenario-editor/auto-corrections-on-page-breaks",
                    DataStorageLayer::SettingsStorage::ApplicationSettings)
                .toInt();
        script->setCorrectionOptions(continueDialogues, correctTextOnPageBreaks);
    }


    m_view->updateStylesElements();
    m_view->updateShortcuts();
    m_view->updateToolBar();
}

int ScenarioTextEditManager::cursorPosition() const
{
    return m_view->cursorPosition();
}

void ScenarioTextEditManager::setAdditionalCursors(const QMap<QString, int>& _cursors)
{
    m_view->setAdditionalCursors(_cursors);
}

void ScenarioTextEditManager::setCommentOnly(bool _isCommentOnly)
{
    m_view->setCommentOnly(_isCommentOnly);
}

void ScenarioTextEditManager::scrollToAdditionalCursor(int _additionalCursorIndex)
{
    m_view->scrollToAdditionalCursor(_additionalCursorIndex);
}

void ScenarioTextEditManager::scrollToPosition(int _position)
{
    m_view->setCursorPosition(_position, false, false);
}

void ScenarioTextEditManager::setScriptHeader(const QString& _header)
{
    m_view->setScriptHeader(_header);
}

void ScenarioTextEditManager::setScriptFooter(const QString& _footer)
{
    m_view->setScriptFooter(_footer);
}

void ScenarioTextEditManager::setSceneNumbersPrefix(const QString& _prefix)
{
    m_view->setSceneNumbersPrefix(_prefix);
}

#ifdef Q_OS_MAC
void ScenarioTextEditManager::buildEditMenu(QMenu* _menu)
{
    m_view->buildEditMenu(_menu);
}
#endif

void ScenarioTextEditManager::addScenarioItem(int _position, int _type, const QString& _name,
    const QString& _header, const QString& _description, const QColor& _color)
{
    //
    // Переводим тип элемента
    //
    const int mappedType = ::mapItemTypeFromModelToBlock(_type);
    m_view->addItem(_position, mappedType, _name, _header, _description, _color);
}

void ScenarioTextEditManager::editScenarioItem(int _startPosition, int _type, const QString& _name,
    const QString& _header, const QString& _colors)
{
    //
    // Переводим тип элемента
    //
    const int mappedType = ::mapItemTypeFromModelToBlock(_type);
    m_view->editItem(_startPosition, mappedType, _name, _header, _colors);
}

void ScenarioTextEditManager::removeScenarioText(int _from, int _to)
{
    m_view->removeText(_from, _to);
}

void ScenarioTextEditManager::changeItemType(int _position, int _type)
{
    //
    // Переводим тип элемента
    //
    const int mappedType = ::mapItemTypeFromModelToBlock(_type);
    m_view->setCursorPosition(_position);
    m_view->setCurrentBlockType(mappedType);
}

void ScenarioTextEditManager::setZenMode(bool _isZen)
{
    m_view->setZenMode(_isZen);
}

void ScenarioTextEditManager::aboutTextEditZoomRangeChanged(qreal _zoomRange)
{
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                "scenario-editor/zoom-range",
                QString::number(_zoomRange),
                DataStorageLayer::SettingsStorage::ApplicationSettings);
}

void ScenarioTextEditManager::renameSceneNumber(const QString& _oldSceneNumber, int _position)
{
    QString newSceneNumber = QLightBoxInputDialog::getText(m_view, tr("Enter new scene number"), tr("New scene number"), _oldSceneNumber);
    if (!newSceneNumber.isEmpty()) {
        emit renameSceneNumberRequested(newSceneNumber, _position);
    }
}

void ScenarioTextEditManager::initView()
{

}

void ScenarioTextEditManager::initConnections()
{
    connect(m_view, &ScenarioTextEditWidget::undoRequest, this, &ScenarioTextEditManager::undoRequest);
    connect(m_view, &ScenarioTextEditWidget::redoRequest, this, &ScenarioTextEditManager::redoRequest);
    connect(m_view, &ScenarioTextEditWidget::textModeChanged, this, &ScenarioTextEditManager::textModeChanged);
    connect(m_view, &ScenarioTextEditWidget::textChanged, this, &ScenarioTextEditManager::textChanged);
    connect(m_view, &ScenarioTextEditWidget::cursorPositionChanged, this, &ScenarioTextEditManager::cursorPositionChanged);
    connect(m_view, &ScenarioTextEditWidget::zoomRangeChanged, this, &ScenarioTextEditManager::aboutTextEditZoomRangeChanged);
    connect(m_view, &ScenarioTextEditWidget::quitFromZenMode, this, &ScenarioTextEditManager::quitFromZenMode);
    connect(m_view, &ScenarioTextEditWidget::addBookmarkRequested, this, &ScenarioTextEditManager::addBookmarkRequested);
    connect(m_view, &ScenarioTextEditWidget::removeBookmarkRequested, this, &ScenarioTextEditManager::removeBookmarkRequested);
    connect(m_view, &ScenarioTextEditWidget::renameSceneNumberRequested, this, &ScenarioTextEditManager::renameSceneNumber);
}
