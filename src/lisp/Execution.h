#pragma once
#include "lisp/common.h"
#include "lisp/lisp.h"

namespace craft {
namespace lisp
{
	class Execution
		: public virtual craft::types::Object
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Execution);
	private:

		instance<Namespace> _namespace;

		std::vector<instance<SFrame>> _stack;

		static thread_local instance<Execution> _tl_current;

		// Lifecycle
	public:

		CRAFT_LISP_EXPORTED Execution(instance<Namespace> ns);

		CRAFT_LISP_EXPORTED void makeCurrent();
		CRAFT_LISP_EXPORTED static instance<Execution> getCurrent();
		CRAFT_LISP_EXPORTED void clearFromCurrent();

		// Instance members
	public:
		CRAFT_LISP_EXPORTED instance<Namespace> getNamespace() const;

		CRAFT_LISP_EXPORTED std::vector<instance<SFrame>> const& stack() const;
		CRAFT_LISP_EXPORTED void push_frame(instance<SFrame> _push);
		CRAFT_LISP_EXPORTED void pop();

		// Helpers
	public:
		CRAFT_LISP_EXPORTED static instance<> exec(std::string const& a, types::GenericInvoke const& b);
		CRAFT_LISP_EXPORTED static instance<> exec(instance<lisp::Function> a, types::GenericInvoke const& b);
		CRAFT_LISP_EXPORTED static instance<> exec(instance<lisp::MultiMethod> a, types::GenericInvoke const& b);
	};
}}
