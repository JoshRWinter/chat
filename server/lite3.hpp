#ifndef SQLITE_3_CPP
#define SQLITE_3_CPP

#include <exception>
#include <string>
#include <cstdint>
#include <cstddef>

#include "sqlite3.h"

namespace lite3{

	class exception : public std::exception
	{
	public:
		exception(const std::string &msg)
			: message(msg) {}

		virtual const char *what() const noexcept
		{
			return message.c_str();
		}

	private:
		const std::string message;
	};

	class statement_exception : public exception
	{
	public:
		statement_exception(const std::string &q, const std::string &msg)
			: exception(msg)
			, m_query(q) {}

		virtual const char *query() const noexcept
		{
			return m_query.c_str();
		}

	private:
		const std::string m_query;
	};

	class statement;

	class connection
	{
		friend statement;
	public:
		connection();
		connection(const std::string&);
		connection(const connection&) = delete;
		connection(connection&&);
		~connection();

		void operator=(const connection&) = delete;
		connection& operator=(connection&&);

		void open(const std::string&);
		void execute(const std::string&);
		void close();
		void begin();
		void rollback();
		void commit();

	private:
		sqlite3 *conn;
	};

	class statement
	{
	public:
		statement(connection&, const std::string&);
		statement(const statement&) = delete;
		statement(statement&&);
		~statement();

		void operator=(const statement&) = delete;
		statement &operator=(statement&&) = delete;

		bool execute();

		void bind(int, const void*, int);
		void bind(int, double);
		void bind(int, std::int32_t);
		void bind(int, std::int64_t);
		void bind(int, std::nullptr_t);
		void bind(int, const std::string&);

		const void *blob(int);
		int blob_size(int);
		double real(int);
		std::int32_t integer(int);
		std::int64_t long_integer(int);
		std::string str(int);


	private:
		sqlite3_stmt *stmt;
		connection &parent;
		const std::string query;
	};
}

#endif // SQLITE_3_CPP
