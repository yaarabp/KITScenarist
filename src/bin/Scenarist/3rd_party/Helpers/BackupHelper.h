#ifndef BACKUPHELPER_H
#define BACKUPHELPER_H

#include <QString>


/**
 * @brief Класс для организации создания резервных копий
 */
class BackupHelper
{
public:
	BackupHelper();

	/**
	 * @brief Включить/выключить
	 */
	void setIsActive(bool _isActive);

	/**
	 * @brief Установить папку для сохранения резервных копий
	 */
	void setBackupDir(const QString& _dir);

	/**
	 * @brief Создать резервную копию файла
	 */
	void saveBackup(const QString& _filePath);

private:
	/**
	 * @brief Включён/выключен
	 */
	bool m_isActive;

	/**
	 * @brief Папка, в которую сохранять резервные копии
	 */
	QString m_backupDir;
};

#endif // BACKUPHELPER_H
