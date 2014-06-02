#include "Database.h"

#include <QStringList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QTextCodec>
#include <QApplication>

using namespace DatabaseLayer;


void Database::setCurrentFile(const QString& _databaseFileName)
{
	//
	// Если использовалась база данных, то удалим старое соединение
	//
	// FIXME: если удалить соединение до тех пор пока не выполнились все запросы,
	//		  то они не смогут корректно завершиться, как отследить все ли запросы
	//		  выполнились я пока не придумал...
	//		  выполняются они в отдельном потоке см. SqlExecutingQueuedThread
	//
//	if (QSqlDatabase::contains(CONNECTION_NAME)) {
//		QSqlDatabase::removeDatabase(CONNECTION_NAME);
//	}

	//
	// Установим текущее имя базы данных
	//
	if (DATABASE_NAME != _databaseFileName) {
		DATABASE_NAME = _databaseFileName;
		CONNECTION_NAME = "local_database [" + DATABASE_NAME + "]";
	}

	//
	// Откроем базу данных, или создадим новую
	//
	instanse();
}

QString Database::currentFile()
{
	return instanse().databaseName();
}

QSqlDatabase Database::instanse()
{
	QSqlDatabase database;

	if (!QSqlDatabase::contains(CONNECTION_NAME)) {
		open(database, CONNECTION_NAME, DATABASE_NAME);
	} else {
		database = QSqlDatabase::database(CONNECTION_NAME);
	}

	return database;
}

QString Database::CONNECTION_NAME = "local_database";
QString Database::SQL_DRIVER      = "QSQLITE";
QString Database::DATABASE_NAME   = ":memory:";

void Database::open(QSqlDatabase& _database, const QString& _connectionName, const QString& _databaseName)
{
	_database = QSqlDatabase::addDatabase(SQL_DRIVER, _connectionName);
	_database.setDatabaseName(_databaseName);
	_database.open();

	Database::States states = checkState(_database);

	if (!states.testFlag(SchemeFlag))
		createTables(_database);
	if (!states.testFlag(IndexesFlag))
		createIndexes(_database);
	if (!states.testFlag(EnumsFlag))
		createEnums(_database);
	if (states.testFlag(OldVersionFlag))
		updateDatabase(_database);
}

// Проверка состояния базы данных
// например:
// - БД отсутствует
// - БД пуста
// - БД имеет старую версию
// - БД имеет последнюю версию
// - и т.д.
Database::States Database::checkState(QSqlDatabase& _database)
{
	QSqlQuery q_checker(_database);
	Database::States states = Database::EmptyFlag;

	//
	// Созданы ли таблицы
	//
	if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'table' ") &&
		q_checker.next() &&
		q_checker.record().value("size").toInt()) {
		//
		// Все остальные проверки имеют смысл, только если проходит данная проверка
		//
		states = states | Database::SchemeFlag;

		//
		// Созданы ли индексы
		//
		if (q_checker.exec("SELECT COUNT(*) as size FROM sqlite_master WHERE type = 'index' ") &&
			q_checker.next() &&
			q_checker.record().value("size").toInt()) {
			states = states | Database::IndexesFlag;
		}

		//
		// Проверка версии
		//
		if (q_checker.exec("SELECT value as version FROM system_variables WHERE variable = 'application-version' ") &&
			q_checker.next() &&
			q_checker.record().value("version").toString() != qApp->applicationVersion()) {
			states = states | Database::OldVersionFlag;
		}
	}

	return states;
}

void Database::createTables(QSqlDatabase& _database)
{
	QSqlQuery q_creator(_database);
	_database.transaction();

	// Таблица системных переменных
	q_creator.exec("CREATE TABLE system_variables "
				   "( "
				   "variable TEXT PRIMARY KEY ON CONFLICT REPLACE, "
				   "value TEXT NOT NULL "
				   "); "
				   );

	// Таблица "Место"
	q_creator.exec("CREATE TABLE places "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Локация"
	q_creator.exec("CREATE TABLE locations "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "name TEXT UNIQUE NOT NULL, "
				   "description TEXT DEFAULT(NULL) "
				   "); "
				   );

	// Таблица "Фотографии локаций"
	q_creator.exec("CREATE TABLE locations_photo "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "fk_location_id INTEGER NOT NULL, "
				   "photo BLOB NOT NULL, "
				   "sort_order INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	// Таблица "Сценарний день"
	q_creator.exec("CREATE TABLE scenary_days "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Время"
	q_creator.exec("CREATE TABLE times "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "name TEXT UNIQUE NOT NULL "
				   "); "
				   );

	// Таблица "Персонажи"
	q_creator.exec("CREATE TABLE characters "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
				   "name TEXT UNIQUE NOT NULL, "
				   "real_name TEXT DEFAULT(NULL), "
				   "description TEXT DEFAULT(NULL) "
				   "); "
				   );

	// Таблица "Фотографии персонажей"
	q_creator.exec("CREATE TABLE characters_photo "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "fk_character_id INTEGER NOT NULL, "
				   "photo BLOB NOT NULL, "
				   "sort_order INTEGER NOT NULL DEFAULT(0) "
				   ")"
				   );

	// Таблица "Текст сценария"
	q_creator.exec("CREATE TABLE scenario "
				   "( "
				   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
				   "name TEXT DEFAULT(NULL), "
				   "synopsis TEXT DEFAULT(NULL), "
				   "text TEXT NOT NULL, "
				   "is_fixed INTEGER NOT NULL DEFAULT(0), " // фиксация версии, дата и комментарий
				   "fix_date TEXT DEFAULT(NULL), "
				   "fix_comment TEXT DEFAULT(NULL) "
				   ")"
				   );

	_database.commit();
}

void Database::createIndexes(QSqlDatabase& _database)
{
	Q_UNUSED(_database);
}

void Database::createEnums(QSqlDatabase& _database)
{
	QSqlQuery q_creator(_database);
	_database.transaction();

	// Версия программы
	{
		q_creator.exec(
					QString("INSERT INTO system_variables VALUES ('application-version', '%1')")
					.arg(qApp->applicationVersion())
					);
	}

	// Справочник мест
	{
#ifdef USE_RUSSIAN_DATABASE_ENUMS
		q_creator.exec(
					QString("INSERT INTO places (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("ИНТ"))
					);
		q_creator.exec(
					QString("INSERT INTO places (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("НАТ"))
					);
#else
		q_creator.exec("INSERT INTO places (id, name) VALUES (null, 'INT');");
		q_creator.exec("INSERT INTO places (id, name) VALUES (null, 'EXT');");
#endif
	}

	// Справочник времени
	{
#ifdef USE_RUSSIAN_DATABASE_ENUMS
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("УТРО"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("ДЕНЬ"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("ВЕЧЕР"))
					);
		q_creator.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("НОЧЬ"))
					);
#else
		q_creator.exec("INSERT INTO times (id, name) VALUES (null, 'MORNING');");
		q_creator.exec("INSERT INTO times (id, name) VALUES (null, 'DAY');");
		q_creator.exec("INSERT INTO times (id, name) VALUES (null, 'EVENING');");
		q_creator.exec("INSERT INTO times (id, name) VALUES (null, 'NIGHT');");
#endif
	}

	_database.commit();
}

void Database::updateDatabase(QSqlDatabase& _database)
{
	QSqlQuery q_checker(_database);

	//
	// Определим версию базы данных
	//
	q_checker.exec("SELECT value as version FROM system_variables WHERE variable = 'application-version' ");
	q_checker.next();
	QString databaseVersion = q_checker.record().value("version").toString();
	int versionMajor = databaseVersion.split(".").value(0, "0").toInt();
	int versionMinor = databaseVersion.split(".").value(1, "0").toInt();
	int versionBuild = databaseVersion.split(".").value(2, "1").toInt();

	//
	// Вызываются необходимые процедуры обновления БД в зависимости от её версии
	//
	switch (versionMajor) {
		default:
		case 0: {

			switch (versionMinor) {
				default:
				case 0: {

					switch (versionBuild) {
						default:
						case 1: {
							updateDatabaseTo_0_0_2(_database);
						}
						case 2:
						case 3: {
							updateDatabaseTo_0_0_4(_database);
						}

						case 4: {
							updateDatabaseTo_0_0_5(_database);
						}
					}

				}

				case 1: {

					switch (versionBuild) {
						default:
						case 0: {
							updateDatabaseTo_0_1_0(_database);
						}
					}
				}
			}

		}
	}

	//
	// Обновляется версия программы
	//
	q_checker.exec(
				QString("INSERT INTO system_variables VALUES ('application-version', '%1')")
				.arg(qApp->applicationVersion())
				);
}

void Database::updateDatabaseTo_0_0_2(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
#ifdef USE_RUSSIAN_DATABASE_ENUMS
		q_updater.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("УТРО"))
					);
		q_updater.exec(
					QString("INSERT INTO times (id, name) VALUES (null, '%1');")
					.arg(QString::fromUtf8("ВЕЧЕР"))
					);
#else
		q_updater.exec("INSERT INTO times (id, name) VALUES (null, 'MORNING');");
		q_updater.exec("INSERT INTO times (id, name) VALUES (null, 'EVENING');");
#endif
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_0_4(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
#ifdef USE_RUSSIAN_DATABASE_ENUMS
		q_updater.exec(
					QString("UPDATE times SET id = 1 WHERE name = '%1';")
					.arg(QString::fromUtf8("УТРО"))
					);
		q_updater.exec(
					QString("UPDATE times SET id = 2 WHERE name = '%1';")
					.arg(QString::fromUtf8("ДЕНЬ"))
					);
		q_updater.exec(
					QString("UPDATE times SET id = 3 WHERE name = '%1';")
					.arg(QString::fromUtf8("ВЕЧЕР"))
					);
		q_updater.exec(
					QString("UPDATE times SET id = 4 WHERE name = '%1';")
					.arg(QString::fromUtf8("НОЧЬ"))
					);
#else
		q_updater.exec("UPDATE times SET id = 1 WHERE name = 'MORNING';");
		q_updater.exec("UPDATE times SET id = 2 WHERE name = 'DAY';");
		q_updater.exec("UPDATE times SET id = 3 WHERE name = 'EVENING';");
		q_updater.exec("UPDATE times SET id = 4 WHERE name = 'NIGHT';");
#endif
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_0_5(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Обновление таблицы локаций
		//
		q_updater.exec("ALTER TABLE locations ADD COLUMN description TEXT DEFAULT(NULL)");

		// Таблица "Фотографии локаций"
		q_updater.exec("CREATE TABLE locations_photo "
					   "( "
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "fk_location_id INTEGER NOT NULL, "
					   "photo BLOB NOT NULL, "
					   "sort_order INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );

		//
		// Обновление таблицы персонажей
		//
		q_updater.exec("ALTER TABLE characters ADD COLUMN real_name TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE characters ADD COLUMN description TEXT DEFAULT(NULL)");

		// Таблица "Фотографии персонажей"
		q_updater.exec("CREATE TABLE characters_photo "
					   "( "
					   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
					   "fk_character_id INTEGER NOT NULL, "
					   "photo BLOB NOT NULL, "
					   "sort_order INTEGER NOT NULL DEFAULT(0) "
					   ")"
					   );
	}

	_database.commit();
}

void Database::updateDatabaseTo_0_1_0(QSqlDatabase& _database)
{
	QSqlQuery q_updater(_database);

	_database.transaction();

	{
		//
		// Обновление таблицы сценария
		//
		q_updater.exec("ALTER TABLE scenario ADD COLUMN name TEXT DEFAULT(NULL)");
		q_updater.exec("ALTER TABLE scenario ADD COLUMN synopsis TEXT DEFAULT(NULL)");
	}

	_database.commit();
}