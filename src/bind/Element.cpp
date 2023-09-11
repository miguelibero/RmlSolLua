#include "bind.h"

#include "plugin/SolLuaDocument.h"
#include "plugin/SolLuaEventListener.h"

#include <unordered_map>


namespace Rml::SolLua
{

	namespace functions
	{
		void addEventListener(Rml::Element& self, const Rml::String& event, sol::protected_function func, const bool in_capture_phase = false)
		{
			auto e = new SolLuaEventListener{ func, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, false);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s, const bool in_capture_phase)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		auto getAttribute(Rml::Element& self, const Rml::String& name, sol::this_state s)
		{
			sol::state_view state{ s };

			auto attr = self.GetAttribute(name);
			return makeObjectFromVariant(attr, s);
		}

		auto getElementsByTagName(Rml::Element& self, const Rml::String& tag)
		{
			Rml::ElementList result;
			self.GetElementsByTagName(result, tag);
			return sol::as_table(result);
		}

		auto getElementsByClassName(Rml::Element& self, const Rml::String& class_name)
		{
			Rml::ElementList result;
			self.GetElementsByClassName(result, class_name);
			return sol::as_table(result);
		}

		auto getAttributes(Rml::Element& self, sol::this_state s)
		{
			SolObjectMap result;

			const auto& attributes = self.GetAttributes();
			for (auto& [key, value] : attributes)
			{
				auto object = makeObjectFromVariant(&value, s);
				result.insert(std::make_pair(key, object));
			}

			return result;
		}

		auto getOwnerDocument(Rml::Element& self)
		{
			auto document = self.GetOwnerDocument();
			auto soldocument = dynamic_cast<SolLuaDocument*>(document);
			return soldocument;
		}

		auto getQuerySelectorAll(Rml::Element& self, const Rml::String& selector)
		{
			Rml::ElementList result;
			self.QuerySelectorAll(result, selector);
			return sol::as_table(result);
		}
	}

	namespace child
	{
		auto getMaxChildren(Rml::Element& self)
		{
			std::function<int()> result = std::bind(&Rml::Element::GetNumChildren, &self, true);
			return result;
		}
	}

	namespace style
	{
		struct StyleProxyIter
		{
			StyleProxyIter(Rml::PropertiesIteratorView&& self) : Iterator(std::move(self)) {}
			Rml::PropertiesIteratorView Iterator;
		};

		auto nextPair(sol::user<StyleProxyIter&> iter_state, sol::this_state s)
		{
			StyleProxyIter& iter = iter_state;
			if (iter.Iterator.AtEnd())
				return std::make_tuple(sol::object(sol::lua_nil), sol::object(sol::lua_nil));

			auto result = std::make_tuple(sol::object(s, sol::in_place, iter.Iterator.GetName()), sol::object(s, sol::in_place, iter.Iterator.GetProperty().ToString()));
			++iter.Iterator;

			return result;
		}

		struct StyleProxy
		{
			StyleProxy(Rml::Element& element) : m_element(element) {}

			std::string Get(const std::string& name)
			{
				auto prop = m_element.GetProperty(name);
				if (prop == nullptr) return {};
				return prop->ToString();
			}

			void Set(const std::string& name, const std::string& value)
			{
				m_element.SetProperty(name, value);
			}

			auto Pairs()
			{
				StyleProxyIter iter{ std::move(m_element.IterateLocalProperties()) };
				return std::make_tuple(&nextPair, std::move(iter), sol::lua_nil);
			}

		private:
			Rml::Element& m_element;
		};

		auto getElementStyleProxy(Rml::Element& self)
		{
			return StyleProxy{ self };
		}
	}

	void bind_element(sol::state_view& lua)
	{
		lua.new_usertype<Rml::EventListener>("EventListener", sol::no_constructor,
			// M
			"OnAttach", &Rml::EventListener::OnAttach,
			"OnDetach", &Rml::EventListener::OnDetach,
			"ProcessEvent", &Rml::EventListener::ProcessEvent
		);

		///////////////////////////

		lua.new_usertype<style::StyleProxy>("StyleProxy", sol::no_constructor,
			sol::meta_function::index, &style::StyleProxy::Get,
			sol::meta_function::new_index, &style::StyleProxy::Set,
			sol::meta_function::pairs, &style::StyleProxy::Pairs
		);

		auto elementUsertype = lua.new_usertype<Rml::Element>("Element", sol::no_constructor);
		elementUsertype[sol::meta_function::to_string] = pointer_to_string<Rml::Element>("sol.Element");
		// M
		elementUsertype["AddEventListener"] = sol::overload(
			[](Rml::Element& s, const Rml::String& e, sol::protected_function f) { functions::addEventListener(s, e, f, false); },
			sol::resolve<void(Rml::Element&, const Rml::String&, sol::protected_function, bool)>(&functions::addEventListener),
			sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state)>(&functions::addEventListener),
			sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state, bool)>(&functions::addEventListener)
		);
		elementUsertype["AppendChild"] = [](Rml::Element& self, Rml::ElementPtr& e) { self.AppendChild(std::move(e)); };
		elementUsertype["Blur"] = &Rml::Element::Blur;
		elementUsertype["Click"] = &Rml::Element::Click;
		elementUsertype["DispatchEvent"] = sol::overload(
			sol::resolve<bool(const Rml::String&, const Rml::Dictionary&)>(&Rml::Element::DispatchEvent),
			[](Rml::Element& s, const Rml::String& event, sol::table parameters)
			{
				Rml::Dictionary parameters2;
				for(auto kv : parameters)
				{
					auto key = kv.first.as<sol::optional<std::string>>();
					if(key)
						parameters2[*key] = makeVariantFromObject(kv.second);
				}
				s.DispatchEvent(event, parameters2);
			}
		);
		elementUsertype["Focus"] = &Rml::Element::Focus;
		elementUsertype["GetAttribute"] = &functions::getAttribute;
		elementUsertype["GetElementById"] = &Rml::Element::GetElementById;
		elementUsertype["GetElementsByTagName"] = &functions::getElementsByTagName;
		elementUsertype["QuerySelector"] = &Rml::Element::QuerySelector;
		elementUsertype["QuerySelectorAll"] = &functions::getQuerySelectorAll;
		elementUsertype["HasAttribute"] = &Rml::Element::HasAttribute;
		elementUsertype["HasChildNodes"] = &Rml::Element::HasChildNodes;
		elementUsertype["InsertBefore"] = [](Rml::Element& self, Rml::ElementPtr& element, Rml::Element* adjacent_element) { self.InsertBefore(std::move(element), adjacent_element); };
		elementUsertype["IsClassSet"] = &Rml::Element::IsClassSet;
		elementUsertype["RemoveAttribute"] = &Rml::Element::RemoveAttribute;
		elementUsertype["RemoveChild"] = &Rml::Element::RemoveChild;
		elementUsertype["ReplaceChild"] = [](Rml::Element& self, Rml::ElementPtr& inserted_element, Rml::Element* replaced_element) { self.ReplaceChild(std::move(inserted_element), replaced_element); };
		elementUsertype["ScrollIntoView"] = [](Rml::Element& self, sol::variadic_args va) { if (va.size() == 0) self.ScrollIntoView(true); else self.ScrollIntoView(va[0].as<bool>()); };
		elementUsertype["SetAttribute"] = static_cast<void(Rml::Element::*)(const Rml::String&, const Rml::String&)>(&Rml::Element::SetAttribute);
		elementUsertype["SetClass"] = &Rml::Element::SetClass;
		//--
		elementUsertype["GetElementsByClassName"] = &functions::getElementsByClassName;
		elementUsertype["Clone"] = &Rml::Element::Clone;
		elementUsertype["Closest"] = &Rml::Element::Closest;
		elementUsertype["SetPseudoClass"] = &Rml::Element::SetPseudoClass;
		elementUsertype["IsPseudoClassSet"] = &Rml::Element::IsPseudoClassSet;
		elementUsertype["ArePseudoClassesSet"] = &Rml::Element::ArePseudoClassesSet;
		elementUsertype["GetActivePseudoClasses"] = &Rml::Element::GetActivePseudoClasses;
		elementUsertype["IsPointWithinElement"] = &Rml::Element::IsPointWithinElement;
		elementUsertype["ProcessDefaultAction"] = &Rml::Element::ProcessDefaultAction;

		// G+S
		elementUsertype["class_name"] = sol::property(&Rml::Element::GetClassNames, &Rml::Element::SetClassNames);
		elementUsertype["id"] = sol::property(&Rml::Element::GetId, &Rml::Element::SetId);
		elementUsertype["inner_rml"] = sol::property(sol::resolve<Rml::String() const>(&Rml::Element::GetInnerRML), &Rml::Element::SetInnerRML);
		elementUsertype["scroll_left"] = sol::property(&Rml::Element::GetScrollLeft, &Rml::Element::SetScrollLeft);
		elementUsertype["scroll_top"] = sol::property(&Rml::Element::GetScrollTop, &Rml::Element::SetScrollTop);

		// G
		elementUsertype["attributes"] = sol::readonly_property(&functions::getAttributes);
		elementUsertype["child_nodes"] = sol::readonly_property(&getIndexedTable<Rml::Element, Rml::Element, &Rml::Element::GetChild, &child::getMaxChildren>);
		elementUsertype["client_left"] = sol::readonly_property(&Rml::Element::GetClientLeft);
		elementUsertype["client_height"] = sol::readonly_property(&Rml::Element::GetClientHeight);
		elementUsertype["client_top"] = sol::readonly_property(&Rml::Element::GetClientTop);
		elementUsertype["client_width"] = sol::readonly_property(&Rml::Element::GetClientWidth);
		elementUsertype["first_child"] = sol::readonly_property(&Rml::Element::GetFirstChild);
		elementUsertype["last_child"] = sol::readonly_property(&Rml::Element::GetLastChild);
		elementUsertype["next_sibling"] = sol::readonly_property(&Rml::Element::GetNextSibling);
		elementUsertype["offset_height"] = sol::readonly_property(&Rml::Element::GetOffsetHeight);
		elementUsertype["offset_left"] = sol::readonly_property(&Rml::Element::GetOffsetLeft);
		elementUsertype["offset_parent"] = sol::readonly_property(&Rml::Element::GetOffsetParent);
		elementUsertype["offset_top"] = sol::readonly_property(&Rml::Element::GetOffsetTop);
		elementUsertype["offset_width"] = sol::readonly_property(&Rml::Element::GetOffsetWidth);
		elementUsertype["owner_document"] = sol::readonly_property(&functions::getOwnerDocument);
		elementUsertype["parent_node"] = sol::readonly_property(&Rml::Element::GetParentNode);
		elementUsertype["previous_sibling"] = sol::readonly_property(&Rml::Element::GetPreviousSibling);
		elementUsertype["scroll_height"] = sol::readonly_property(&Rml::Element::GetScrollHeight);
		elementUsertype["scroll_width"] = sol::readonly_property(&Rml::Element::GetScrollWidth);
		elementUsertype["style"] = sol::readonly_property(&style::getElementStyleProxy);
		elementUsertype["tag_name"] = sol::readonly_property(&Rml::Element::GetTagName);
		//--
		elementUsertype["address"] = sol::readonly_property([](Rml::Element& self) { return self.GetAddress(); });
		elementUsertype["absolute_left"] = sol::readonly_property(&Rml::Element::GetAbsoluteLeft);
		elementUsertype["absolute_top"] = sol::readonly_property(&Rml::Element::GetAbsoluteTop);
		elementUsertype["baseline"] = sol::readonly_property(&Rml::Element::GetBaseline);
		elementUsertype["line_height"] = sol::readonly_property(&Rml::Element::GetLineHeight);
		elementUsertype["visible"] = sol::readonly_property(&Rml::Element::IsVisible);
		elementUsertype["z_index"] = sol::readonly_property(&Rml::Element::GetZIndex);
	}

} // end namespace Rml::SolLua
