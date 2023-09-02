#include "bind.h"

#include "plugin/SolLuaEventListener.h"

#include <unordered_map>


namespace Rml::SolLua
{

	namespace functions
	{
		template <class TSelf>
		constexpr bool hasAttribute(TSelf& self, const std::string& name)
		{
			return self.HasAttribute(name);
		}
		#define HASATTRGETTER(S, N) [](S& self) { return self.HasAttribute(N); }

		template <class TSelf, typename T>
		T getAttributeWithDefault(TSelf& self, const std::string& name, T def)
		{
			auto attr = self.template GetAttribute<T>(name, def);
			return attr;
		}
		#define GETATTRGETTER(S, N, D) [](S& self) { return functions::getAttributeWithDefault(self, N, D); }

		template <class TSelf, class T>
		constexpr void setAttribute(TSelf& self, const std::string& name, const T& value)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, bool>)
			{
				if (value)
					self.SetAttribute(name, true);
				else self.RemoveAttribute(name);
			}
			else
			{
				self.SetAttribute(name, value);
			}
		}
		#define SETATTR(S, N, D) [](S& self, const D& value) { functions::setAttribute(self, N, value); }
	}

	namespace submit
	{
		void submit(Rml::ElementForm& self)
		{
			self.Submit();
		}

		void submitName(Rml::ElementForm& self, const std::string& name)
		{
			self.Submit(name);
		}

		void submitNameValue(Rml::ElementForm& self, const std::string& name, const std::string& value)
		{
			self.Submit(name, value);
		}
	}

	void bind_element_form(sol::state_view& lua)
	{

		lua.new_usertype<Rml::ElementForm>("ElementForm", sol::no_constructor,
			// M
			"Submit", sol::overload(&submit::submit, &submit::submitName, &submit::submitNameValue),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		lua.new_usertype<Rml::ElementFormControl>("ElementFormControl", sol::no_constructor,
			// G+S
			"disabled", sol::property(&Rml::ElementFormControl::IsDisabled, &Rml::ElementFormControl::SetDisabled),
			"name", sol::property(&Rml::ElementFormControl::GetName, &Rml::ElementFormControl::SetName),
			"value", sol::property(&Rml::ElementFormControl::GetValue, &Rml::ElementFormControl::SetValue),

			// G
			//--
			"submitted", sol::readonly_property(&Rml::ElementFormControl::IsSubmitted),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		lua.new_usertype<Rml::ElementFormControlInput>("ElementFormControlInput", sol::no_constructor,
			// G+S
			"checked", sol::property(HASATTRGETTER(Rml::ElementFormControlInput, "checked"), SETATTR(Rml::ElementFormControlInput, "checked", bool)),
			"maxlength", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "maxlength", -1), SETATTR(Rml::ElementFormControlInput, "maxlength", int)),
			"size", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "size", 20), SETATTR(Rml::ElementFormControlInput, "size", int)),
			"max", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "max", 100), SETATTR(Rml::ElementFormControlInput, "max", int)),
			"min", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "min", 0), SETATTR(Rml::ElementFormControlInput, "min", int)),
			"step", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "step", 1), SETATTR(Rml::ElementFormControlInput, "step", int)),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////

		lua.new_usertype<Rml::ElementFormControlSelect>("ElementFormControlSelect", sol::no_constructor,
			// M
			"Add", sol::overload(
				[](Rml::ElementFormControlSelect& self, Rml::ElementPtr& element, sol::optional<int> before)
				{
					return self.Add(std::move(element), from_lua_index(before.value_or(0)));
				},
				[](
					Rml::ElementFormControlSelect& self,
					const Rml::String& rml,
					const Rml::String& value,
					std::optional<int> before)
				{
					return self.Add(rml, value, from_lua_index(before.value_or(0)));
				}
			),
			"Remove", [](Rml::ElementFormControlSelect& self, int index)
			{
				self.Remove(from_lua_index(index));
			},
			"RemoveAll", &Rml::ElementFormControlSelect::RemoveAll,

			// G+S
			"selection", sol::property(
				[](const Rml::ElementFormControlSelect& self) -> int
				{
					return to_lua_index(self.GetSelection());
				},
				[](Rml::ElementFormControlSelect& self, int v)
				{
					self.SetSelection(from_lua_index(v));
				}
			),

			// G
			"options", sol::property([](Rml::ElementFormControlSelect& self, sol::this_state state) -> sol::table
			{
				sol::state_view lua(state);
				auto result = lua.create_table();
				int i = 0;
				while (auto element = self.GetOption(i++))
				{
					auto luaElement = lua.create_table();
					luaElement["element"] = element;
					luaElement["value"] = element->GetAttribute("value", std::string{});
					result.add(luaElement);
				}
				return result;
			}),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////

		lua.new_usertype<Rml::ElementFormControlDataSelect>("ElementFormControlDataSelect", sol::no_constructor,
			// M
			"SetDataSource", &Rml::ElementFormControlDataSelect::SetDataSource,

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControlSelect, Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////

		lua.new_usertype<Rml::ElementFormControlTextArea>("ElementFormControlTextArea", sol::no_constructor,
			// G+S
			"cols", sol::property(&Rml::ElementFormControlTextArea::GetNumColumns, &Rml::ElementFormControlTextArea::SetNumColumns),
			"maxlength", sol::property(&Rml::ElementFormControlTextArea::GetMaxLength, &Rml::ElementFormControlTextArea::SetMaxLength),
			"rows", sol::property(&Rml::ElementFormControlTextArea::GetNumRows, &Rml::ElementFormControlTextArea::SetNumRows),
			"wordwrap", sol::property(&Rml::ElementFormControlTextArea::SetWordWrap, &Rml::ElementFormControlTextArea::GetWordWrap),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

	}

} // end namespace Rml::SolLua
