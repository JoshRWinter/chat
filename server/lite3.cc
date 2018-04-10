#include <cstdlib>
#include <iostream>

#include "lite3.hpp"

using namespace std::string_literals;

// *******************************
// *******************************
// *******************************
// sqlite3 c++ interface
// *******************************
// *******************************
// *******************************

// default constructor
lite3::connection::connection()
{
	conn = NULL;
}

// db file path constructor
lite3::connection::connection(const std::string &filepath)
{
	open(filepath);
}

// move constructor
lite3::connection::connection(connection &&other)
{
	conn = other.conn;
	other.conn = NULL;
}

// destructor
lite3::connection::~connection()
{
	close();
}

// move assignment
lite3::connection &lite3::connection::operator=(connection &&other)
{
	conn = other.conn;
	other.conn = NULL;

	return *this;
}

void lite3::connection::open(const std::string &filepath)
{
	if(sqlite3_open(filepath.c_str(), &conn) != SQLITE_OK)
	{
		exception e(sqlite3_errmsg(conn));
		sqlite3_close(conn);

		throw e;
	}
}

void lite3::connection::close()
{
	if(conn != NULL)
	{
		if(sqlite3_close(conn) != SQLITE_OK)
		{
			std::cerr << "the database connection could not be closed" << std::endl;
			std::abort();
		}

		conn = NULL;
	}
}

void lite3::connection::execute(const std::string &exec)
{
	statement s(*this, exec);
	if(s.execute())
		throw exception("results were returned from static execute");
}

void lite3::connection::begin()
{
	execute("BEGIN TRANSACTION");
}

void lite3::connection::rollback()
{
	execute("ROLLBACK");
}

void lite3::connection::commit()
{
	execute("COMMIT");
}

// *******************************
// *******************************
// *******************************
// sqlite3 statement interface
// *******************************
// *******************************
// *******************************

// constructor
lite3::statement::statement(connection &db, const std::string &q)
	: parent(db)
	, query(q)
{
	if(sqlite3_prepare_v2(db.conn, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
		throw statement_exception(query, "statement: "s + sqlite3_errmsg(parent.conn));
}

// move constructor
lite3::statement::statement(statement &&other)
	: parent(other.parent)
	, query(std::move(other.query))
{
	stmt = other.stmt;
	other.stmt = NULL;
}

// destructor
lite3::statement::~statement()
{
	if(sqlite3_finalize(stmt) != SQLITE_OK)
	{
		std::cerr << "the statement could not be finalized" << std::endl;
		std::abort();
	}
}

// execute the query specified in the constructor
// call this function repeatedly until it returns false, indicating no more available rows.
bool lite3::statement::execute()
{
	const int result = sqlite3_step(stmt);

	if(result != SQLITE_ROW && result != SQLITE_DONE)
		throw statement_exception(query, "statement: "s + sqlite3_errmsg(parent.conn));

	return result == SQLITE_ROW;
}

void lite3::statement::bind(int column, const void *blob, int size)
{
	if(sqlite3_bind_blob(stmt, column, blob, size, SQLITE_TRANSIENT) != SQLITE_OK)
		throw statement_exception(query, "statment: blob bind: "s + sqlite3_errmsg(parent.conn));
}

void lite3::statement::bind(int column, double real)
{
	if(sqlite3_bind_double(stmt, column, real) != SQLITE_OK)
		throw statement_exception(query, "statement: double bind: "s + sqlite3_errmsg(parent.conn));
}

void lite3::statement::bind(int column, std::int32_t integer)
{
	if(sqlite3_bind_int(stmt, column, integer) != SQLITE_OK)
		throw statement_exception(query, "statement: int bind: "s + sqlite3_errmsg(parent.conn));
}

void lite3::statement::bind(int column, std::int64_t integer)
{
	if(sqlite3_bind_int64(stmt, column, integer) != SQLITE_OK)
		throw statement_exception(query, "statement: long int bind: "s + sqlite3_errmsg(parent.conn));
}

void lite3::statement::bind(int column, std::nullptr_t)
{
	if(sqlite3_bind_null(stmt, column) != SQLITE_OK)
		throw statement_exception(query, "statement: null bind: "s + sqlite3_errmsg(parent.conn));
}

void lite3::statement::bind(int column, const std::string &str)
{
	if(sqlite3_bind_text(stmt, column, str.c_str(), -1, SQLITE_TRANSIENT))
		throw statement_exception(query, "statement: string bind: "s + sqlite3_errmsg(parent.conn));
}

// get a void* blob from column <column>
const void *lite3::statement::blob(int column)
{
	return sqlite3_column_blob(stmt, column);
}

// get blob size in bytes from column <column>
int lite3::statement::blob_size(int column)
{
	return sqlite3_column_bytes(stmt, column);
}

// get a double blob from column <column>
double lite3::statement::real(int column)
{
	return sqlite3_column_double(stmt, column);
}

// get a int32 blob from column <column>
std::int32_t lite3::statement::integer(int column)
{
	return sqlite3_column_int(stmt, column);
}

// get a int64 blob from column <column>
std::int64_t lite3::statement::long_integer(int column)
{
	return sqlite3_column_int64(stmt, column);
}

// get a string blob from column <column>
std::string lite3::statement::str(int column)
{
	return (char*)sqlite3_column_text(stmt, column);
}
