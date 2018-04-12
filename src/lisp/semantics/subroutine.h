#pragma once
#include "lisp/common.h"
#include "lisp/lisp.h"

namespace craft {
namespace lisp
{
	class PSubroutine
		: public craft::types::Provider
	{
		CRAFT_LISP_EXPORTED CRAFT_LEGACY_FEATURE_DECLARE(craft::lisp::PSubroutine, "lisp.subroutine", types::SingletonProviderManager);

	public:
        
        CRAFT_LISP_EXPORTED virtual types::Function function(instance<>) const = 0;
		CRAFT_LISP_EXPORTED virtual types::ExpressionStore expression(instance<>) const = 0;

		//
		// Performs a full call on the subroutine, including pushing to the current execution
		//
		CRAFT_LISP_EXPORTED virtual instance<> execute(instance<>, types::GenericInvoke const& call) const = 0;
	};
}}
