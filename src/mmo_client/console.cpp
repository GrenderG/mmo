// Copyright (C) 2019, Robin Klimonow. All rights reserved.

#include "console.h"
#include "console_commands.h"
#include "event_loop.h"
#include "screen.h"

#include "log/default_log_levels.h"
#include "graphics/graphics_device.h"
#include "frame_ui/font.h"
#include "frame_ui/geometry_buffer.h"

#include <mutex>

namespace mmo
{
	std::map<std::string, Console::ConsoleCommand, StrCaseIComp> Console::s_consoleCommands;

	/// A event that listens for KeyDown events from the event loop to handle console input.
	static scoped_connection s_consoleKeyDownEvent;

	// Used to determine whether the console window needs to be rendered right now
	/// Whether the console window should be rendered right now.
	static bool s_consoleVisible = false;
	/// The current height of the console window in pixels, counted from the top edge.
	static int32 s_consoleWindowHeight = 210;
	static int32 s_lastViewportWidth = 0;
	static int32 s_lastViewportHeight = 0;

	// Used for rendering the console window background
	static ScreenLayerIt s_consoleLayer;
	static VertexBufferPtr s_consoleVertBuf;
	static IndexBufferPtr s_consoleIndBuf;

	static FontPtr s_consoleFont;
	static std::unique_ptr<GeometryBuffer> s_consoleTextGeom;
	static bool s_consoleTextDirty = true;
	static std::list<std::string> s_consoleLog;

	static std::mutex s_consoleLogMutex;
	static scoped_connection s_consoleLogConn;

	void Console::Initialize(const std::filesystem::path& configFile)
	{
		// Ensure the folder is created
		std::filesystem::create_directories(configFile.parent_path());

		// Register some default console commands
		RegisterCommand("ver", console_commands::ConsoleCommand_Ver, ConsoleCommandCategory::Default, "Displays the client version.");
		RegisterCommand("run", console_commands::ConsoleCommand_Run, ConsoleCommandCategory::Default, "Runs a console script.");

		// Load the config file
		console_commands::ConsoleCommand_Run("run", configFile.string());

		// Console is hidden by default
		s_consoleVisible = false;
		s_consoleWindowHeight = 210;

		// Initialize the graphics api
		auto& device = GraphicsDevice::CreateD3D11();
		device.SetWindowTitle("MMORPG");

		// Query the viewport size
		device.GetViewport(nullptr, nullptr, &s_lastViewportWidth, &s_lastViewportHeight, nullptr, nullptr);

		// Create the vertex buffer for the console background
		const POS_COL_VERTEX vertices[] = {
			{ { 0.0f, 0.0f, 0.0f }, 0xc0000000 },
			{ { s_lastViewportWidth, 0.0f, 0.0f }, 0xc0000000 },
			{ { s_lastViewportWidth, s_consoleWindowHeight, 0.0f }, 0xc0000000 },
			{ { 0.0f, s_consoleWindowHeight, 0.0f }, 0xc0000000 }
		};

		// Setup vertices
		s_consoleVertBuf = device.CreateVertexBuffer(4, sizeof(POS_COL_VERTEX), true, vertices);

		// Setup indices
		const uint16 indices[] = { 0, 1, 2, 2, 3, 0 };
		s_consoleIndBuf = device.CreateIndexBuffer(6, IndexBufferSize::Index_16, indices);

		// Load the console font
		s_consoleFont = std::make_shared<Font>();
		VERIFY(s_consoleFont->Initialize("Fonts/ARIALN.TTF", 12.0f, 0.0f));

		// Create a geometry buffer for the console output text
		s_consoleTextGeom = std::make_unique<GeometryBuffer>();
		s_consoleTextDirty = true;
		s_consoleLog.clear();

		// Initialize the screen system
		Screen::Initialize();

		// Assign console log signal
		s_consoleLogConn = mmo::g_DefaultLog.signal().connect([](const mmo::LogEntry & entry) {
			std::scoped_lock lock{ s_consoleLogMutex };

			s_consoleLog.push_front(entry.message);
			if (s_consoleLog.size() > 50)
			{
				s_consoleLog.pop_back();
			}

			s_consoleTextDirty = true;
		});

		// Add the console layer
		s_consoleLayer = Screen::AddLayer(&Console::Paint, 100.0f, ScreenLayerFlags::IdentityTransform);

		// Watch for the console key event
		s_consoleKeyDownEvent = EventLoop::KeyDown.connect(&Console::KeyDown);
	}
	
	void Console::Destroy()
	{
		// Disconnect the key events
		s_consoleKeyDownEvent.disconnect();

		// Remove the console layer
		Screen::RemoveLayer(s_consoleLayer);

		// Destroy the screen system
		Screen::Destroy();
		
		// Close connection
		s_consoleLogConn.disconnect();

		// Delete console text geometry
		s_consoleTextGeom.reset();

		// Delete console font object
		s_consoleFont.reset();

		// Reset vertex and index buffer
		s_consoleIndBuf.reset();
		s_consoleVertBuf.reset();
		s_consoleLog.clear();

		// Destroy the graphics device
		GraphicsDevice::Destroy();

		// Remove default console commands
		UnregisterCommand("run");
		UnregisterCommand("ver");
	}

	inline void Console::RegisterCommand(const std::string & command, ConsoleCommandHandler handler, ConsoleCommandCategory category, const std::string & help)
	{
		// Don't do anything if this console command is already registered
		auto it = s_consoleCommands.find(command);
		if (it != s_consoleCommands.end())
		{
			return;
		}

		// Build command structure and add it
		ConsoleCommand cmd;
		cmd.category = category;
		cmd.help = std::move(help);
		cmd.handler = std::move(handler);
		s_consoleCommands.emplace(command, cmd);
	}

	inline void Console::UnregisterCommand(const std::string & command)
	{
		// Remove the respective iterator
		auto it = s_consoleCommands.find(command);
		if (it != s_consoleCommands.end())
		{
			s_consoleCommands.erase(it);
		}
	}

	void Console::ExecuteCommand(std::string commandLine)
	{
		// Will hold the command name
		std::string command;
		std::string arguments;

		// Find the first space and use it to get the command
		auto space = commandLine.find(' ');
		if (space == commandLine.npos)
		{
			command = commandLine;
		}
		else
		{
			command = commandLine.substr(0, space);
			arguments = commandLine.substr(space + 1);
		}

		// If somehow the command is empty, just stop here without saying anything.
		if (command.empty())
		{
			return;
		}

		// Check if such argument exists
		auto it = s_consoleCommands.find(command);
		if (it == s_consoleCommands.end())
		{
			ELOG("Unknown console command \"" << command << "\"");
			return;
		}

		// Now execute the console commands handler if there is any
		if (it->second.handler)
		{
			it->second.handler(command, arguments);
		}
	}

	bool Console::KeyDown(int key)
	{
		// Console key will toggle the console visibility
		if (key == 0xC0 || key == 0xDC)
		{
			// Show the console window again
			s_consoleVisible = !s_consoleVisible;
			if (s_consoleVisible && s_consoleWindowHeight <= 0)
			{
				s_consoleWindowHeight = 200;
			}

			return false;
		}

		return true;
	}

	bool Console::KeyUp(int key)
	{
		return true;
	}

	void Console::Paint()
	{
		// Nothing to render here eventually
		if (!s_consoleVisible)
			return;

		// Get the current graphics device
		auto& gx = GraphicsDevice::Get();

		// Create console text geometry
		if (s_consoleTextDirty)
		{
			s_consoleTextGeom->Reset();
			
			// Calculate start point
			mmo::Point startPoint{ 0.0f, 0.0f };

			for (auto it = s_consoleLog.rbegin(); it != s_consoleLog.rend(); it++)
			{
				// Draw line of text
				s_consoleFont->DrawText(*it, startPoint, *s_consoleTextGeom);

				// Reduce line by one
				startPoint.y += s_consoleFont->GetHeight();

				// Stop it here
				if (startPoint.y > s_consoleWindowHeight)
				{
					break;
				}
			}
			
			s_consoleTextDirty = false;
		}

		// Obtain viewport info
		int32 s_vpWidth = 0, s_vpHeight = 0;
		gx.GetViewport(nullptr, nullptr, &s_vpWidth, &s_vpHeight, nullptr, nullptr);

		// Check for changes in viewport size, in which case we would need to update the contents of our vertex buffer
		if (s_vpWidth != s_lastViewportWidth || s_vpHeight != s_lastViewportHeight)
		{
			s_lastViewportWidth = s_vpWidth;
			s_lastViewportHeight = s_vpHeight;

			// Create the vertex buffer for the console background
			const POS_COL_VERTEX vertices[] = {
				{ { 0.0f, 0.0f, 0.0f }, 0xc0000000 },
				{ { s_lastViewportWidth, 0.0f, 0.0f }, 0xc0000000 },
				{ { s_lastViewportWidth, s_consoleWindowHeight, 0.0f }, 0xc0000000 },
				{ { 0.0f, s_consoleWindowHeight, 0.0f }, 0xc0000000 }
			};

			// Update vertex buffer data
			CScopedGxBufferLock<POS_COL_VERTEX> lock { *s_consoleVertBuf };
			*lock[0] = vertices[0];
			*lock[1] = vertices[1];
			*lock[2] = vertices[2];
			*lock[3] = vertices[3];

			DLOG("Updated console vertex buffer");
		}

		// Set up a clipping rect
		gx.SetClipRect(0, 0, s_lastViewportWidth, s_consoleWindowHeight);

		// Update transform
		gx.SetTransformMatrix(TransformType::Projection, Matrix4::MakeOrthographic(0.0f, s_vpWidth, s_vpHeight, 0.0f, 0.0f, 100.0f));

		// Prepare drawing mode
		gx.SetVertexFormat(VertexFormat::PosColor);
		gx.SetTopologyType(TopologyType::TriangleList);
		gx.SetBlendMode(BlendMode::Alpha);

		// Set buffers
		s_consoleVertBuf->Set();
		s_consoleIndBuf->Set();

		// Draw buffer content
		gx.DrawIndexed();

		// Draw text
		s_consoleTextGeom->Draw();

		// Clear the clip rect again
		gx.ResetClipRect();
	}
}
