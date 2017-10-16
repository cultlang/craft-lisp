#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/Macro.h"

using namespace craft;
using namespace craft::types;
using namespace craft::lisp;


CRAFT_OBJECT_DEFINE(Macro)
{
	_.defaults();
}

Macro::Macro(f_macro macro)
{
	_macro = macro;
}

instance<> Macro::expand(instance<Scope> const& scope, std::vector<instance<>> const& forms)
{
	return _macro(scope, forms);
}