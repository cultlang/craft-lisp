#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/library/libraries.h"


using namespace craft;
using namespace craft::types;
using namespace craft::lisp;
using namespace craft::lisp::library::helper;


/*
bool helper::truth(instance<SFrame> frame, instance<PSubroutine> truth, instance<> code)
{
	auto result = frame->getNamespace()->environment()->eval(frame, code);
	auto value = truth->call(truth, frame, { result });

	assert(value.typeId().isType<bool>());

	return *value.asType<bool>();
}
*/

/*
ExpressionStore library::helper::binding_expr_to_signature(instance<SScope> scope, instance<Sexpr> expr)
{
	// (foo bar baz)
	// ((foo Int) bar (baz (Union String Int)))
	// (foo Int, bar, baz (Union String Int))
	// TODO:
	// * optional
	//   ? ((foo Int) (bar Int 8))
	//   ? ((foo Int) (bar Int :optional))
	// * keyword arugments
	//   ? ((foo Int) (:bar Int) (:baz 8))
	//   ? ((foo Int) (:keyword bar Int) (:keyword baz 8))
	// * return type
	//   ? ((foo Int) (bar Int 8) (:baz 8) Int)
	//   ? ((foo Int) (bar Int 8) (:baz 8) (:returns Int))
	// * collections
	//   ? ((foo Int) (args :args))
	//   ? ((foo Int) (args :args) (kwargs :keywords))

	auto env = scope->environment();
	auto ret = instance<SubroutineSignature>::make();

	for (auto cell : expr->cells)
	{
		auto arg = instance<Argument>::make();

		if (cell.typeId().isType<Sexpr>())
		{
			auto arg_expr = cell.asType<Sexpr>();

			if (arg_expr->cells.size() > 0)
			{
				arg->name = symbol(arg_expr->car());
			}
			if (arg_expr->cells.size() > 1)
			{
				// TODO build type expression
				auto next = env->read(scope, arg_expr->cells[1]);
				next = env->read_eval(scope, next);

				if (!next.hasFeature<types::SType>())
					throw stdext::exception("Signature expected type object.");
				arg->type = next;
			}
			else
			{
				arg->type = env->type_any();
			}
		}
		else
		{
			arg->name = symbol(cell);
			arg->type = env->type_any();
		}

		ret->arguments.push_back(arg);
	}

	ret->complete();
	return ret;
}
*/

/*
instance<Module> lisp::make_library_globals(instance<Namespace> ns)
{
	auto env = ns->environment();
	auto ret = instance<Module>::make(ns, "builtin:cult.system");

	// -- Types --
	auto Any = instance<types::Special>::make();
	Any->kind = types::Special::Any;
	ret->define_eval("Any", Any);

	auto Bottom = instance<types::Special>::make();
	Bottom->kind = types::Special::Bottom;
	ret->define_eval("Bottom", Bottom);

	auto String = instance<lisp::types::CraftType>::make(types::cpptype<std::string>::typeDesc());
	ret->define_eval("String", String);

	auto Bool = instance<lisp::types::CraftType>::make(types::cpptype<bool>::typeDesc());
	ret->define_eval("Bool", Bool);

	auto Int = instance<lisp::types::CraftType>::make(types::cpptype<int64_t>::typeDesc());
	ret->define_eval("Int", Int);

	auto Float64 = instance<lisp::types::CraftType>::make(types::cpptype<double>::typeDesc());
	ret->define_eval("Float64", Float64);

	auto Symbol_ = instance<lisp::types::CraftType>::make(types::cpptype<lisp::Symbol>::typeDesc());
	ret->define_eval("Symbol", Symbol_);

	auto Keyword_ = instance<lisp::types::CraftType>::make(types::cpptype<lisp::Keyword>::typeDesc());
	ret->define_eval("Keyword", Keyword_);

	auto Sexpr_ = instance<lisp::types::CraftType>::make(types::cpptype<lisp::Sexpr>::typeDesc());
	ret->define_eval("Sexpr", Sexpr_);

	auto craft_type = instance<BuiltinFunction>::make(
		[](instance<SFrame> frame, std::vector<instance<>> args) -> instance<>
	{
		auto pidentifier = types::system().getManager<PIdentifier>();

		if (args.size() == 0)
		{
			for (auto tid : pidentifier->supportedTypes())
				frame->getNamespace()->environment()->log()->info(tid.toString());
			return instance<>();
		}

		auto lookup = symbol(args[0]);

		auto type = pidentifier->index(lookup);

		if (type == types::None)
			return instance<>();

		return instance<lisp::types::CraftType>::make(type);
	});
	ret->define_eval("craft-type", craft_type);

	auto Tuple = instance<BuiltinFunction>::make(
		[](auto frame, auto args) -> instance<>
	{
		auto ret = instance<lisp::types::Tuple>::make();

		for (auto it = args.begin(); it != args.end(); ++it)
		{
			//assert(it->hasFeature<::craft::lisp::types::SType>());
			ret->cells.push_back(*it);
		}

		return ret;
	});
	ret->define_eval("Tuple", Tuple);

	auto Union = instance<BuiltinFunction>::make(
		[](auto frame, auto args) -> instance<>
	{
		auto ret = instance<lisp::types::Union>::make();

		for (auto it = args.begin(); it != args.end(); ++it)
		{
			//assert(it->hasFeature<lisp::types::SType>());
			ret->variants.push_back(*it);
		}

		return ret;
	});
	ret->define_eval("Union", Union);

	auto subtype = instance<MultiMethod>::make();
	subtype->attach(env, instance<BuiltinFunction>::make(
		SubroutineSignature::makeFromArgsAndReturn<types::SType, types::SType, bool>(),
		[](instance<SFrame> frame, std::vector<instance<>> const& args) -> instance<>
	{
		instance<> a(args[0]), b(args[1]);

		return instance<bool>::make(types::is_subtype(frame->getNamespace()->environment(), a, b));
	}));
	ret->define_eval("subtype?", subtype);
	ret->define_eval("\u227C", subtype);

	// -- Compiler Specials --
	auto truth = instance<MultiMethod>::make();
	truth->attach(env, instance<BuiltinFunction>::make(
		SubroutineSignature::makeFromArgsAndReturn<bool, bool>(),
		[](auto frame, std::vector<instance<>> const& args)
	{
		return instance<bool>(args[0]);
	}));
	ret->define_eval("truth", truth);
	ret->define_eval("?", truth);

	auto isNull = instance<MultiMethod>::make();
	isNull->attach(env, instance<BuiltinFunction>::make(
		[](auto frame, std::vector<instance<>> const& args)
	{
		for (auto i : args)
		{
			if(i.get() == nullptr) return instance<bool>::make(true);
		}
		return instance<bool>::make(false);
	}));
	ret->define_eval("isnull", isNull);



	// -- Special Forms --

	auto quote = instance<SpecialForm>::make(
		[](instance<SScope> scope, instance<> head, instance<Sexpr> sexpr) -> instance<>
	{
		sexpr->cells[0] = head;
		return sexpr;
	},
		[](instance<SFrame> frame, instance<Sexpr> sexpr) -> instance<>
	{
		if (sexpr->cells.size() == 2)
		{
			return sexpr->cells[1];
		}
		auto res = instance<Sexpr>::make();
		for (auto it = sexpr->cells.begin() + 1; it != sexpr->cells.end(); ++it)
		{
			res->cells.push_back(*it);
		}

		return res;
	});
	ret->define_eval("quote", quote);

	auto fn = instance<SpecialForm>::make(
		[](instance<SScope> scope, instance<> head, instance<Sexpr> sexpr) -> instance<>
	{
		// Setup special form
		size_t size = sexpr->cells.size();
		assert(size == 3);
		size -= 1;

		auto binding = sexpr->cells[1];
		auto body = sexpr->cells[2];

		auto function = instance<Function>::make();

		auto signature = library::helper::binding_expr_to_signature(scope, binding);

		function->setSignature(signature);

		auto func_scope = signature->read_frame(scope);
		body = func_scope->environment()->read(func_scope, body);

		function->setBody(body);

		auto ret = instance<Sexpr>::make();
		ret->cells.push_back(head);
		ret->cells.push_back(function);
		return ret;
	},
		[](instance<SFrame> frame, instance<Sexpr> sexpr) -> instance<>
	{
		if (sexpr->cells.size() != 2 || !sexpr->cells[1].typeId().isType<Function>())
			throw stdext::exception("Malformed fn evaluated.");

		auto function = sexpr->cells[1];

		if (frame.typeId().isType<Frame>()
			&& frame.asType<Frame>()->getScope().typeId().isType<Module>())
			return function;

		return instance<Closure>::make(frame, function);
	});
	ret->define_eval("fn", fn);

	// -- Builtin Library --
	auto lookup = instance<MultiMethod>::make();
	lookup->attach(env, instance<BuiltinFunction>::make(
		[](auto frame, auto args)
	{
		instance<SScope> lookup_scope(args[0]);
		auto lookup_name = symbol(args[1]);

		auto binding = lookup_scope->lookup(lookup_name);
		if (binding)
			return binding->getValue(frame);

		return instance<>();
	}));
	ret->define_eval("lookup", lookup);

	auto lookup_meta = instance<MultiMethod>::make();
	lookup_meta->attach(env, instance<BuiltinFunction>::make(
		[](auto frame, auto args)
	{
		instance<SScope> lookup_scope(args[0]);
		auto lookup_name = symbol(args[1]);
		auto lookup_meta = symbol(args[2]);

		auto binding = lookup_scope->lookup(lookup_name);
		if (binding)
			return binding->getMeta(lookup_meta);

		return instance<>();
	}));
	ret->define_eval("lookup/meta", lookup_meta);

	auto _require_impl = instance<BuiltinFunction>::make(
		[](instance<SFrame> frame, auto args)
	{
		instance<Module> module = args[0];
		instance<std::string> uri = args[1];

		return module->namespace_()->requireModule(*uri, args.size() < 3 ? instance<>() : args[2]);
	});

	auto require = instance<Macro>::make(
		[_require_impl](instance<SScope> scope, std::vector<instance<>> const& code)
	{
		instance<Sexpr> expr = scope->environment()->parse(scope, "(:replace *module*)");

		auto parsed_macro = expr->cells[0].asType<Sexpr>();

		parsed_macro->cells[0] = _require_impl;
		for (auto it = code.begin() + 1; it != code.end(); ++it)
			parsed_macro->cells.push_back(*it);

		return parsed_macro;
	});
	ret->define_eval("require", require);

	auto eval = instance<BuiltinFunction>::make(
		SubroutineSignature::makeFromArgs<std::string>(),
		[](auto frame, auto args)
	{
		instance<std::string> a(expect<std::string>(args[0]));
		instance<Environment> env = frame->getNamespace()->environment();

		return frame->getNamespace()->environment()->eval(frame, *a);
	});
	ret->define_eval("eval", eval);

	
	auto _true = instance<BuiltinFunction>::make([](auto f, auto a) {return instance<bool>::make(true); });
	auto _false = instance<BuiltinFunction>::make([](auto f, auto a) {return instance<bool>::make(false); });

	ret->define_eval("#t", _true);
	ret->define_eval("#f", _false);


	auto file_text = instance<MultiMethod>::make();
	file_text->attach(env, instance<BuiltinFunction>::make(
		[](instance<SFrame> frame, auto args)
	{
    auto s = path::normalize(path::absolute(*args[0].template asType<std::string>()));
		auto text = craft::fs::read<std::string>(s, &craft::fs::string_read).get();

		return instance<std::string>::make(text);
	}));
	ret->define_eval("file/text", file_text);

	//
	// MultiMethod
	//
	auto MultiMethod_ = instance<MultiMethod>::make();
	MultiMethod_->attach(env, instance<BuiltinFunction>::make(
		[](auto frame, auto args)
	{
		return instance<MultiMethod>::make();
	}));
	ret->define_eval("MultiMethod", MultiMethod_);

	auto attach = instance<lisp::MultiMethod>::make();
	attach->attach(env, instance<BuiltinFunction>::make(
		[](instance<SFrame> frame, auto args)
	{
		instance<MultiMethod> a(args[0]); instance<> b(args[1]);

		a->attach(frame->getNamespace()->environment(), b);

		return a;
	}));
	ret->define_eval("attach", attach);

	auto dispatch = instance<lisp::MultiMethod>::make();
	dispatch->attach(env, instance<BuiltinFunction>::make(
		[](instance<SFrame> frame, auto args)
	{
		instance<MultiMethod> a(args[0]); instance<> b(args[1]);

		MultiMethod::Dispatch _dispatch;

		a->dispatch(frame->getNamespace()->environment(), b, &_dispatch);

		return _dispatch.subroutine;
	}));
	ret->define_eval("dispatch", dispatch);


	//
	// Introspection
	//

	auto report = instance<MultiMethod>::make();
	report->attach(env, instance<BuiltinFunction>::make(
		SubroutineSignature::makeFromArgs<Function>(),
		[](auto frame, auto args)
	{
		instance<Function> a(expect<Function>(args[0]));

		return instance<std::string>::make("");
	}));
	ret->define_eval("report", report);

	auto time_highres = instance<MultiMethod>::make();
	time_highres->attach(env, instance<BuiltinFunction>::make(
		[](auto frame, auto args)
	{
		return instance<std::int64_t>::make((int64_t)(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	}));
	ret->define_eval("time-highres", time_highres);

	auto timeit = instance<Macro>::make(
		[](instance<SScope> scope, std::vector<instance<>> const& code)
	{
		instance<Sexpr> expr = scope->environment()->parse(scope, "(do (define g_xxxx_begin (time-highres)) :replace (define g_xxxx_end (time-highres)) (- g_xxxx_end g_xxxx_begin))");

		expr->cells[0].asType<Sexpr>()->cells[2] = code[1];

		return expr->cells[0];
	});
	ret->define_eval("timeit", timeit);


	auto failure = instance<MultiMethod>::make();
	failure->attach(env, instance<BuiltinFunction>::make(
		SubroutineSignature::makeFromArgs<std::string>(),
		[](instance<SFrame> frame, auto args)
	{
		instance<std::string> a(expect<std::string>(args[0]));
		throw stdext::exception("FAILURE: {0}", *a);
		return instance<>();
	}));
	ret->define_eval("failure", failure);
	return ret;
}
*/