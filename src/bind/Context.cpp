#include "bind.h"

#include "plugin/SolLuaDocument.h"
#include "plugin/SolLuaDataModel.h"

#include <memory>


namespace Rml::SolLua
{

	namespace document
	{
		/// <summary>
		/// Return a SolLuaDocument.
		/// </summary>
		auto getDocumentBypass(Rml::Context& self, int idx)
		{
			auto document = self.GetDocument(idx);
			auto result = rmlui_dynamic_cast<SolLuaDocument*>(document);
			return result;
		}

		/// <summary>
		/// Return a SolLuaDocument.
		/// </summary>
		auto getDocumentBypassString(Rml::Context& self, const Rml::String& name)
		{
			auto document = self.GetDocument(name);
			return rmlui_dynamic_cast<SolLuaDocument*>(document);
		}

		/// <summary>
		/// Helper function to fill the indexed table with data.
		/// </summary>
		auto getDocument(Rml::Context& self)
		{
			std::function<SolLuaDocument* (int)> result = [&self](int idx) -> auto { return getDocumentBypass(self, idx); };
			return result;
		}
	}

	namespace datamodel
	{
		/// <summary>
		/// Bind a sol::table into the data model.
		/// </summary>
		/// <param name="data">The data model container.</param>
		/// <param name="table">The table to bind.</param>
		void bindTable(SolLuaDataModel* data, sol::table& table)
		{
			for (auto& [key, value] : table)
			{
				auto skey = key.as<std::string>();
				auto it = data->ObjectList.insert_or_assign(skey, value);

				if (value.get_type() == sol::type::function)
				{
					data->Constructor.BindEventCallback(skey,
						[skey, cb = sol::protected_function{ value }, state = sol::state_view{ table.lua_state() }](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& varlist)
						{
							if (cb.valid())
							{
								std::vector<sol::object> args;
								for (const auto& variant : varlist)
								{
									args.push_back(makeObjectFromVariant(&variant, state));
								}
								auto pfr = cb(event, sol::as_args(args));
								if (!pfr.valid())
									ErrorHandler(cb.lua_state(), std::move(pfr));
							}
						});
				}
				else
				{
					data->Constructor.BindCustomDataVariable(skey, Rml::DataVariable(data->ObjectDef.get(), &(it.first->second)));
				}
			}
		}

		/// <summary>
		/// Opens a Lua data model.
		/// </summary>
		/// <param name="self">The context that called this function.</param>
		/// <param name="name">The name of the data model.</param>
		/// <param name="model">The table to bind as the data model.</param>
		/// <param name="s">Lua state.</param>
		/// <returns>A unique pointer to a Sol Lua Data Model.</returns>
		std::unique_ptr<SolLuaDataModel> openDataModel(Rml::Context& self, const Rml::String& name, sol::object model, sol::this_state s)
		{
			sol::state_view lua{ s };

			// Create data model.
			auto constructor = self.CreateDataModel(name);
			auto data = std::make_unique<SolLuaDataModel>(lua);

			// Already created?  Get existing.
			if (!constructor)
			{
				constructor = self.GetDataModel(name);
				if (!constructor)
					return data;
			}

			data->Constructor = constructor;
			data->Handle = constructor.GetModelHandle();
			data->ObjectDef = std::make_unique<SolLuaObjectDef>(data.get());

			// Only bind table.
			if (model.get_type() == sol::type::table)
			{
				data->Table = model.as<sol::table>();
				datamodel::bindTable(data.get(), data->Table);
			}

			return data;
		}
	}

	namespace element
	{
		auto getElementAtPoint1(Rml::Context& self, Rml::Vector2f point)
		{
			return self.GetElementAtPoint(point);
		}

		auto getElementAtPoint2(Rml::Context& self, Rml::Vector2f point, Rml::Element& ignore)
		{
			return self.GetElementAtPoint(point, &ignore);
		}
	}

	/// <summary>
	/// Binds the Rml::Context class to Lua.
	/// </summary>
	/// <param name="lua">The Lua state to bind into.</param>
	void bind_context(sol::state_view& lua)
	{
		auto usertype = lua.new_usertype<Rml::Context>("Context", sol::no_constructor);
		usertype[sol::meta_function::to_string] = pointer_to_string<Rml::Context>("sol.Context");
		// M
		usertype["AddEventListener"] = &Rml::Context::AddEventListener;
		usertype["CreateDocument"] = [](Rml::Context& self) { return self.CreateDocument(); };
		usertype["LoadDocument"] = [](Rml::Context& self, const Rml::String& document) {
			auto doc = self.LoadDocument(document);
			return rmlui_dynamic_cast<SolLuaDocument*>(doc);
		};
		usertype["GetDocument"] = &document::getDocumentBypassString;
		usertype["Render"] = &Rml::Context::Render;
		usertype["UnloadAllDocuments"] = &Rml::Context::UnloadAllDocuments;
		usertype["UnloadDocument"] = &Rml::Context::UnloadDocument;
		usertype["Update"] = &Rml::Context::Update;
		usertype["OpenDataModel"] = &datamodel::openDataModel;
		usertype["ProcessMouseMove"] = &Rml::Context::ProcessMouseMove;
		usertype["ProcessMouseButtonDown"] = &Rml::Context::ProcessMouseButtonDown;
		usertype["ProcessMouseButtonUp"] = &Rml::Context::ProcessMouseButtonUp;
#if RmlUi_VERSION_MAJOR >= 5 && RmlUi_VERSION_MINOR >= 1
		usertype["ProcessMouseWheel"] = sol::overload(
			static_cast<bool(Rml::Context::*)(float, int)>(&Rml::Context::ProcessMouseWheel),
			static_cast<bool(Rml::Context::*)(Vector2f, int)>(&Rml::Context::ProcessMouseWheel)
		);
#else
		usertype["ProcessMouseWheel"] = &Rml::Context::ProcessMouseWheel;
#endif
		usertype["ProcessMouseLeave"] = &Rml::Context::ProcessMouseLeave;
		usertype["IsMouseInteracting"] = &Rml::Context::IsMouseInteracting;
		usertype["ProcessKeyDown"] = &Rml::Context::ProcessKeyDown;
		usertype["ProcessKeyUp"] = &Rml::Context::ProcessKeyUp;
		usertype["ProcessTextInput"] = sol::resolve<bool(const Rml::String&)>(&Rml::Context::ProcessTextInput);
		//--
		usertype["EnableMouseCursor"] = &Rml::Context::EnableMouseCursor;
		usertype["ActivateTheme"] = &Rml::Context::ActivateTheme;
		usertype["IsThemeActive"] = &Rml::Context::IsThemeActive;
		usertype["GetElementAtPoint"] = sol::overload(&element::getElementAtPoint1, &element::getElementAtPoint2);
		usertype["PullDocumentToFront"] = &Rml::Context::PullDocumentToFront;
		usertype["PushDocumentToBack"] = &Rml::Context::PushDocumentToBack;
		usertype["UnfocusDocument"] = &Rml::Context::UnfocusDocument;
		// RemoveEventListener

		// G+S
		usertype["dimensions"] = sol::property(&Rml::Context::GetDimensions, &Rml::Context::SetDimensions);
		usertype["dp_ratio"] = sol::property(&Rml::Context::GetDensityIndependentPixelRatio, &Rml::Context::SetDensityIndependentPixelRatio);
		//--
		usertype["clip_region"] = sol::property(&Rml::Context::GetActiveClipRegion, &Rml::Context::SetActiveClipRegion);

		// G
		usertype["documents"] = sol::readonly_property(&getIndexedTable<SolLuaDocument, Rml::Context, &document::getDocument, &Rml::Context::GetNumDocuments>);
		usertype["focus_element"] = sol::readonly_property(&Rml::Context::GetFocusElement);
		usertype["hover_element"] = sol::readonly_property(&Rml::Context::GetHoverElement);
		usertype["name"] = sol::readonly_property(&Rml::Context::GetName);
		usertype["root_element"] = sol::readonly_property(&Rml::Context::GetRootElement);
	}

} // end namespace Rml::SolLua
