// Copyright (C) 2019, Robin Klimonow. All rights reserved.

#pragma once

#include "binary_io/string_sink.h"
#include "connection.h"

namespace mmo
{
	template <class P>
	class SendSink : public io::StringSink
	{
	public:

		typedef AbstractConnection<P> MyConnection;

	public:

		explicit SendSink(MyConnection &connection)
			: io::StringSink(connection.getSendBuffer())
			, m_connection(connection)
		{
		}

		virtual void flush()
		{
			m_connection.flush();
		}

	private:

		MyConnection &m_connection;
	};
}
