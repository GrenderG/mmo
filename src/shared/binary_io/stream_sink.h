// Copyright (C) 2019, Robin Klimonow. All rights reserved.

#pragma once

#include "sink.h"
#include <ostream>

namespace io
{
	class StreamSink : public ISink
	{
	public:

		typedef std::ostream Stream;


		explicit StreamSink(Stream &dest)
			: m_dest(dest)
		{
		}

		virtual std::size_t write(const char *src, std::size_t size) override
		{
			m_dest.write(src, static_cast<std::streamsize>(size));
			return size; //hack
		}

		virtual std::size_t overwrite(std::size_t position, const char *src, std::size_t size) override
		{
			const auto previous = m_dest.tellp();
			m_dest.seekp(static_cast<std::streamoff>(position));
			const auto result = write(src, size);
			m_dest.seekp(previous);
			return result;
		}

		virtual std::size_t position() override
		{
			return static_cast<std::size_t>(m_dest.tellp());
		}

		virtual void flush() override
		{
			m_dest.flush();
		}

	private:

		Stream &m_dest;
	};
}
