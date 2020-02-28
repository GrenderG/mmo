// Copyright (C) 2019, Robin Klimonow. All rights reserved.

#pragma once

#include "game_protocol.h"
#include "network/connection.h"
#include "network/send_sink.h"

namespace mmo
{
	namespace game
	{
		typedef mmo::Connection<Protocol> Connection;
		typedef mmo::IConnectionListener<Protocol> IConnectionListener;
		typedef mmo::SendSink<Protocol> SendSink;
	}
}
