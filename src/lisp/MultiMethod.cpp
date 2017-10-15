#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/MultiMethod.h"

using namespace craft;
using namespace craft::types;
using namespace craft::lisp;


CRAFT_OBJECT_DEFINE(MultiMethod)
{
	_.use<PSubroutine>().singleton<AutoSubroutine>();

	_.defaults();
}

MultiMethod::MultiMethod()
{

}

instance<> MultiMethod::call(instance<Scope> const& scope, std::vector<instance<>> const& args)
{
	auto subroutine = dispatch();

	if (!subroutine)
		throw stdext::exception("Dispatch failed.");

	return subroutine.getFeature<PSubroutine>()->call(subroutine, scope, args);
}

instance<> MultiMethod::dispatch()
{
	return _only;
}
void MultiMethod::attach(instance<> in)
{
	if (_only)
		assert(false && "Attach not implemented");

	if (!in.hasFeature<PSubroutine>())
		stdext::exception("Attached subroutine must implement PSubroutine.");

	_only = in;
}