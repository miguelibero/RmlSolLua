#include "SolLuaDocument.h"

#include <RmlUi/Core/Stream.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/FileInterface.h>


namespace Rml::SolLua
{

	sol::protected_function_result ErrorHandler(lua_State*, sol::protected_function_result pfr)
	{
		if (!pfr.valid())
		{
			sol::error err = pfr;
			Rml::Log::Message(Rml::Log::LT_ERROR, "[LUA][ERROR] %s", err.what());
		}
		return pfr;
	}

	//-----------------------------------------------------

	SolLuaDocument::SolLuaDocument(sol::state_view state, const Rml::String& tag, const Rml::String& lua_env_identifier)
		: m_state(state), ElementDocument(tag), m_environment(state, sol::create, state.globals()), m_lua_env_identifier(lua_env_identifier)
	{
	}

	void SolLuaDocument::LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line)
	{
		auto* context = GetContext();

		Rml::String buffer{ "--" };
		buffer.append("[");
		buffer.append(context->GetName());
		buffer.append("][");
		buffer.append(GetSourceURL());
		buffer.append("]:");
		buffer.append(Rml::ToString(source_line));
		buffer.append("\n");
		buffer.append(content);

		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		m_state.safe_script(buffer, m_environment, ErrorHandler);
	}

	void SolLuaDocument::LoadExternalScript(const String& source_path)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		// use the file interface to get the contents of the script
		FileInterface* file_interface = GetFileInterface();
		FileHandle handle = file_interface->Open(source_path);
		if (handle == 0)
		{
			Log::Message(Log::LT_WARNING, "LoadFile: Unable to open file: %s", source_path.c_str());
			return;
		}

		size_t size = file_interface->Length(handle);
		if (size == 0)
		{
			Log::Message(Log::LT_WARNING, "LoadFile: File is 0 bytes in size: %s", source_path.c_str());
			return;
		}
		UniquePtr<char[]> file_contents(new char[size]);
		file_interface->Read(file_contents.get(), size, handle);
		file_interface->Close(handle);

		Rml::String chunkname = "@" + source_path;
		std::string_view source(file_contents.get(), size);
		m_state.safe_script(source, m_environment, ErrorHandler, chunkname);
	}

	sol::protected_function_result SolLuaDocument::RunLuaScript(const Rml::String& script)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		return m_state.safe_script(script, m_environment, ErrorHandler);
	}

} // namespace Rml::SolLua
