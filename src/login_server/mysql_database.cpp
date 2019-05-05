#include "mysql_database.h"
#include "mysql_wrapper/mysql_row.h"
#include "mysql_wrapper/mysql_select.h"
#include "mysql_wrapper/mysql_statement.h"
#include "log/default_log_levels.h"

namespace mmo
{
	MySQLDatabase::MySQLDatabase(const mysql::DatabaseInfo &connectionInfo)
		: m_connectionInfo(connectionInfo)
	{
	}

	bool MySQLDatabase::load()
	{
		if (!m_connection.Connect(m_connectionInfo))
		{
			ELOG("Could not connect to the login database");
			ELOG(m_connection.GetErrorMessage());
			return false;
		}

		// Mark all realms as offline
		ILOG("Connected to MySQL at " <<
			m_connectionInfo.host << ":" <<
			m_connectionInfo.port);

		return true;
	}

	std::optional<AccountData> MySQLDatabase::getAccountDataByName(const std::string & name)
	{
		mysql::Select select(m_connection, "SELECT id,username,s,v FROM account WHERE username='" + m_connection.EscapeString(name) + "' LIMIT 1");
		if (select.Success())
		{
			mysql::Row row(select);
			if (row)
			{
				// Account exists: Get data
				AccountData data;
				row.GetField(0, data.id);
				row.GetField(1, data.name);
				row.GetField(2, data.s);
				row.GetField(3, data.v);
				return data;
			}
		}
		else
		{
			// There was an error
			PrintDatabaseError();
		}

		return {};
	}

	std::optional<RealmAuthData> MySQLDatabase::getRealmAuthData(uint32 realmId)
	{
		mysql::Select select(m_connection, "SELECT internalName,password FROM realm WHERE id = " + std::to_string(realmId) + " LIMIT 1");
		if (select.Success())
		{
			mysql::Row row(select);
			if (row)
			{
				RealmAuthData data;
				row.GetField(0, data.name);
				row.GetField(1, data.password);
				return data;
			}
		}
		else
		{
			// There was an error
			PrintDatabaseError();
		}

		return {};
	}

	std::optional<std::pair<uint64, std::string>> MySQLDatabase::getAccountSessionKey(const std::string& accountName)
	{
		mysql::Select select(m_connection, "SELECT id,k FROM account WHERE username = '" + m_connection.EscapeString(accountName) + "' LIMIT 1");
		if (select.Success())
		{
			mysql::Row row(select);
			if (row)
			{
				return std::make_pair<uint64, std::string>(std::atoll(row.GetField(0)), std::string(row.GetField(1)));
			}
		}
		else
		{
			// There was an error
			PrintDatabaseError();
		}

		return {};
	}

	void MySQLDatabase::setAccountSessionKey(uint64 accountId, const std::string& sessionKey)
	{
		m_connection.Execute("UPDATE account SET k = '" + m_connection.EscapeString(sessionKey) + "' WHERE id = " + std::to_string(accountId));
	}
	
	void MySQLDatabase::PrintDatabaseError()
	{
		ELOG("Login database error: " << m_connection.GetErrorMessage());
	}
}
