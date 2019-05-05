#include "configuration.h"

#include "simple_file_format/sff_write.h"
#include "simple_file_format/sff_read_tree.h"
#include "simple_file_format/sff_load_file.h"
#include "base/constants.h"
#include "log/default_log_levels.h"

#include <filesystem>
#include <fstream>
#include <limits>

namespace mmo
{
	const uint32 Configuration::LoginConfigVersion = 0x01;

	Configuration::Configuration()
		: playerPort(mmo::constants::DefaultLoginPlayerPort)
        , realmPort(mmo::constants::DefaultLoginRealmPort)
		, maxPlayers((std::numeric_limits<decltype(maxPlayers)>::max)())
		, maxRealms(constants::MaxRealmCount)
		, mysqlPort(mmo::constants::DefaultMySQLPort)
		, mysqlHost("127.0.0.1")
		, mysqlUser("mmo")
		, mysqlPassword("")
		, mysqlDatabase("mmo_login")
		, isLogActive(true)
		, logFileName("logs/login")
		, isLogFileBuffering(false)
		, webPort(8090)
		, webSSLPort(8091)
		, webUser("mmo-web")
		, webPassword("test")
	{
	}

	bool Configuration::load(const String &fileName)
	{
		typedef String::const_iterator Iterator;
		typedef sff::read::tree::Table<Iterator> Table;

		Table global;
		std::string fileContent;

		std::ifstream file(fileName, std::ios::binary);
		if (!file)
		{
			if (save(fileName))
			{
				ILOG("Saved default settings as " << fileName);
			}
			else
			{
				ELOG("Could not save default settings as " << fileName);
			}

			return false;
		}

		try
		{
			sff::loadTableFromFile(global, fileContent, file);

			// Read config version
			uint32 fileVersion = 0;
			if (!global.tryGetInteger("version", fileVersion) ||
				fileVersion != LoginConfigVersion)
			{
				file.close();

				if (save(fileName + ".updated"))
				{
					ILOG("Saved updated settings with default values as " << fileName << ".updated");
					ILOG("Please insert values from the old setting file manually and rename the file.");
				}
				else
				{
					ELOG("Could not save updated default settings as " << fileName << ".updated");
				}

				return false;
			}

			if (const Table *const mysqlDatabaseTable = global.getTable("mysqlDatabase"))
			{
				mysqlPort = mysqlDatabaseTable->getInteger("port", mysqlPort);
				mysqlHost = mysqlDatabaseTable->getString("host", mysqlHost);
				mysqlUser = mysqlDatabaseTable->getString("user", mysqlUser);
				mysqlPassword = mysqlDatabaseTable->getString("password", mysqlPassword);
				mysqlDatabase = mysqlDatabaseTable->getString("database", mysqlDatabase);
			}

			if (const Table *const mysqlDatabaseTable = global.getTable("webServer"))
			{
				webPort = mysqlDatabaseTable->getInteger("port", webPort);
				webSSLPort = mysqlDatabaseTable->getInteger("ssl_port", webSSLPort);
				webUser = mysqlDatabaseTable->getString("user", webUser);
				webPassword = mysqlDatabaseTable->getString("password", webPassword);
			}

			if (const Table *const playerManager = global.getTable("playerManager"))
			{
				playerPort = playerManager->getInteger("port", playerPort);
				maxPlayers = playerManager->getInteger("maxCount", maxPlayers);
			}

			if (const Table *const realmManager = global.getTable("realmManager"))
			{
				realmPort = realmManager->getInteger("port", realmPort);
				maxRealms = realmManager->getInteger("maxCount", maxRealms);
				if (maxRealms > constants::MaxRealmCount)
				{
					WLOG("Warning: Game client only supports up to " 
						<< constants::MaxRealmCount << " realms, but you requested support for " 
						<< maxRealms << " realms. Max realm count will be clamped.");

					maxRealms = constants::MaxRealmCount;
				}
			}

			if (const Table *const log = global.getTable("log"))
			{
				isLogActive = log->getInteger("active", static_cast<unsigned>(isLogActive)) != 0;
				logFileName = log->getString("fileName", logFileName);
				isLogFileBuffering = log->getInteger("buffering", static_cast<unsigned>(isLogFileBuffering)) != 0;
			}
		}
		catch (const sff::read::ParseException<Iterator> &e)
		{
			const auto line = std::count<Iterator>(fileContent.begin(), e.position.begin, '\n');
			ELOG("Error in config: " << e.what());
			ELOG("Line " << (line + 1) << ": " << e.position.str());
			return false;
		}

		return true;
	}

	bool Configuration::save(const String &fileName)
	{
		// Try to create directories
		try
		{
			std::filesystem::create_directories(
				std::filesystem::path(fileName).remove_filename());
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			ELOG("Failed to create log directories: " << e.what() << "\n\tPath 1: " << e.path1() << "\n\tPath 2: " << e.path2());
		}

		std::ofstream file(fileName.c_str());
		if (!file)
		{
			return false;
		}

		typedef char Char;
		sff::write::File<Char> global(file, sff::write::MultiLine);

		// Save file version
		global.addKey("version", LoginConfigVersion);
		global.writer.newLine();

		{
			sff::write::Table<Char> mysqlDatabaseTable(global, "mysqlDatabase", sff::write::MultiLine);
			mysqlDatabaseTable.addKey("port", mysqlPort);
			mysqlDatabaseTable.addKey("host", mysqlHost);
			mysqlDatabaseTable.addKey("user", mysqlUser);
			mysqlDatabaseTable.addKey("password", mysqlPassword);
			mysqlDatabaseTable.addKey("database", mysqlDatabase);
			mysqlDatabaseTable.Finish();
		}

		global.writer.newLine();

		{
			sff::write::Table<Char> mysqlDatabaseTable(global, "webServer", sff::write::MultiLine);
			mysqlDatabaseTable.addKey("port", webPort);
			mysqlDatabaseTable.addKey("ssl_port", webSSLPort);
			mysqlDatabaseTable.addKey("user", webUser);
			mysqlDatabaseTable.addKey("password", webPassword);
			mysqlDatabaseTable.Finish();
		}

		global.writer.newLine();

		{
			sff::write::Table<Char> playerManager(global, "playerManager", sff::write::MultiLine);
			playerManager.addKey("port", playerPort);
			playerManager.addKey("maxCount", maxPlayers);
			playerManager.Finish();
		}

		global.writer.newLine();

		{
			sff::write::Table<Char> realmManager(global, "realmManager", sff::write::MultiLine);
			realmManager.addKey("port", realmPort);
			realmManager.addKey("maxCount", maxRealms);
			realmManager.Finish();
		}
		
		global.writer.newLine();

		{
			sff::write::Table<Char> log(global, "log", sff::write::MultiLine);
			log.addKey("active", static_cast<unsigned>(isLogActive));
			log.addKey("fileName", logFileName);
			log.addKey("buffering", isLogFileBuffering);
			log.Finish();
		}

		return true;
	}
}
