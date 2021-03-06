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

		instance<Environment> _env;

		std::list<instance<SFrameSection>> _stack;

		static thread_local instance<Execution> _tl_current;

		// Lifecycle
	public:

		CRAFT_LISP_EXPORTED Execution(instance<Environment> env);

		CRAFT_LISP_EXPORTED void makeCurrent();
		CRAFT_LISP_EXPORTED static instance<Execution> getCurrent();
		CRAFT_LISP_EXPORTED void clearFromCurrent();

		// Instance members
	public:
		CRAFT_LISP_EXPORTED instance<Environment> getEnvironment() const;

		CRAFT_LISP_EXPORTED std::list<instance<SFrameSection>> const& stack() const;
		CRAFT_LISP_EXPORTED void push_frame(instance<SFrameSection> _push);
		CRAFT_LISP_EXPORTED void pop();

		template<typename T>
		inline instance<T> topIfOfType() const
		{
			auto const& s = stack();
			if (s.size() == 0) return instance<>();
			auto t = s.front();
			if (!t.isType<T>()) return instance<>();
			return t.asType<T>();
		}

		inline instance<SFrameSection> top() const
		{
			auto const& s = stack();
			if (s.size() == 0) return instance<>();
			return s.front();
		}

		// Helpers
	public:
		CRAFT_LISP_EXPORTED static instance<> exec_fromCurrentModule(std::string const& a, types::GenericInvoke const& b);
		CRAFT_LISP_EXPORTED static instance<> exec(instance<PSubroutine> callable, types::GenericInvoke const& b);

		CRAFT_LISP_EXPORTED static instance<> eval(std::string const&);
	};
}}
