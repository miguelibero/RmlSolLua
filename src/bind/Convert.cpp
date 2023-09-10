#include "bind.h"

#include "plugin/SolLuaDocument.h"


namespace Rml::SolLua
{

	namespace functions
	{
		template <typename T>
		T* convert(Rml::Element& element)
		{
			return rmlui_dynamic_cast<T*>(&element);
		}
	}

	void bind_convert(sol::state_view& lua)
	{
		auto element = lua.create_named_table("Element");
		auto element_as = element.create_named("As");

		element_as["Document"] = &functions::convert<SolLuaDocument>;
		element_as["ElementText"] = &functions::convert<Rml::ElementText>;
		element_as["ElementDataGrid"] = &functions::convert<Rml::ElementDataGrid>;
		element_as["ElementDataGridRow"] = &functions::convert<Rml::ElementDataGridRow>;
		element_as["ElementDataGridCell"] = &functions::convert<Rml::ElementDataGridCell>;
		element_as["ElementForm"] = &functions::convert<Rml::ElementForm>;
		element_as["ElementFormControl"] = &functions::convert<Rml::ElementFormControl>;
		element_as["ElementFormControlInput"] = &functions::convert<Rml::ElementFormControlInput>;
		element_as["ElementFormControlSelect"] = &functions::convert<Rml::ElementFormControlSelect>;
		element_as["ElementFormControlDataSelect"] = &functions::convert<Rml::ElementFormControlDataSelect>;
		element_as["ElementFormControlTextArea"] = &functions::convert<Rml::ElementFormControlTextArea>;
		element_as["ElementTabSet"] = &functions::convert<Rml::ElementTabSet>;
	}

} // end namespace Rml::SolLua
