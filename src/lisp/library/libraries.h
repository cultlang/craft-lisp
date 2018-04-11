#pragma once
#include "lisp/common.h"
#include "lisp/lisp.h"

#include "lisp/semantics/cult/cult.h"

//
// globals.h
namespace craft {
namespace lisp {

	/*
		Namespace for special forms
	*/
	namespace library
	{
		namespace helper
		{
			/* Try to get a string from a symbol like object (e.g. strings, symbols, keywords).
			
			*/
			CRAFT_LISP_EXPORTED std::string symbol(instance<>);

			/* Throw an exception if a given argument is not of the expected type

			*/
			template <typename T>
			inline instance<T> expect(instance<> inst)
			{
				if (!inst)
					return inst;

				if (inst.typeId().isType<T>())
					return inst;

				throw stdext::exception("Expected {0} got {1}", types::cpptype<T>::typeDesc().toString(), inst.typeId().toString());
			}

			/* Takes an sexpr represting a function binding, and returns a signature

			a "macro"

			*/
			CRAFT_LISP_EXPORTED types::ExpressionStore binding_expr_to_signature(instance<SScope> scope, instance<Sexpr>);
		}

		namespace core
		{
			void make_string_globals(instance<Module> m);
			void make_shim_globals(instance<Module> m);
			void make_arithmatic_globals(instance<Module> m);
			void make_cast_globals(instance<Module> m);
			void make_logic_globals(instance<Module> m);
			void make_fs_globals(instance<Module> m);
			void make_llvm_globals(instance<Module> m);
			void make_zmq_globals(instance<Module> m);
			void make_env_globals(instance<Module> m);
			void make_regex_globals(instance<Module> m);
			void make_http_globals(instance<Module> m);
			void make_meta_globals(instance<Module> m);
			void make_subprocess_globals(instance<Module> m);
			void make_list_globals(instance<Module> m);
			void make_map_globals(instance<Module> m);
			void make_platform_globals(instance<Module> m);
			void make_buffer_globals(instance<Module> m);
			void make_security_globals(instance<Module> m);
			void make_repl_globals(instance<Module> m);
			void make_json_globals(instance<Module> m);
		}

		instance<Module> make_module_builtin_cult_system(instance<Namespace> ns);
		instance<Module> make_module_builtin_cult_core(instance<Namespace> ns);
	}
}}

//
// C++ interfaces for libraries
//

#include "lisp/library/iteration.h"