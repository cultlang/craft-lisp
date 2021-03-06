#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/semantics/cult/cult.h"
#include "lisp/semantics/cult/calling.h"

#include "lisp/backend/BootstrapInterpreter.h" // For Function calling interpreter

using namespace craft;
using namespace craft::types;
using namespace craft::lisp;

/******************************************************************************
** CallSite
******************************************************************************/

CRAFT_DEFINE(CallSite)
{
	_.use<PClone>().singleton<FunctionalCopyConstructor>([](instance<CallSite> that)
	{
		auto count = that->argCount();
		std::vector<instance<SCultSemanticNode>> args;
		args.reserve(count);
		for (size_t i =0; i < count; ++i)
			args.push_back(SCultSemanticNode::_clone(that->argAst(i)));

		return instance<CallSite>::make(_clone(that->calleeAst()), args);
	});

	_.use<SCultSemanticNode>().byCasting();

	_.defaults();
}

CallSite::CallSite(instance<SCultSemanticNode> callee, std::vector<instance<SCultSemanticNode>> args)
{
	_callee = callee;

	_args.reserve(args.size());
	for (auto arg : args)
		_args.push_back(arg);
}

void CallSite::craft_setupInstance()
{
	Object::craft_setupInstance();

	_ast(_callee);
	for (auto arg : _args)
		_ast(arg);
}

instance<> CallSite::calleeAst() const
{
	return _callee;
}

size_t CallSite::argCount() const
{
	return _args.size();
}
instance<> CallSite::argAst(size_t index) const
{
	return _args.at(index);
}

void CallSite::bind()
{
	_callee->bind();
	for (auto arg : _args)
	{
		arg->bind();
	}
}

/******************************************************************************
** Function
******************************************************************************/
namespace craft {
namespace lisp {
	class FunctionSubroutineProvider
		: public types::Implements<PSubroutine>::For<Function>
	{
		static ExpressionStore MultiMethodAny;

		virtual types::Function function(instance<> _) const override
		{
			throw stdext::exception("Needs to support closures....");
		}
		virtual types::ExpressionStore expression(instance<> _) const override
		{
			instance<Function> fn = _;

			return fn->subroutine_signature();
		}

		//
		// Performs a full call on the subroutine, including pushing to the current execution
		//
		virtual instance<> execute(instance<> _, types::GenericInvoke const& call) const override
		{
			instance<Function> fn = _;

			// TODO make this generic
			auto frame = InterpreterFrameSection::ensureCurrent(Execution::getCurrent()->getEnvironment()->get<BootstrapInterpreter>());

			return frame->interp_call(fn, call);
		}
	};
}}


CRAFT_DEFINE(lisp::Function)
{
	_.use<PSubroutine>().singleton<FunctionSubroutineProvider>();
	_.use<PClone>().singleton<FunctionalCopyConstructor>([](instance<lisp::Function> that)
	{
		auto clone = instance<lisp::Function>::make();

		clone->setBody(_clone(that->bodyAst()));

		auto count = that->argCount();
		//clone->preSize(count);
		for (size_t i =0; i < count; ++i)
			clone->pushArg(_clone(that->argAst(i)));

		return clone;
	});

	_.use<SCultSemanticNode>().byCasting();
	_.use<SBindable>().byCasting();
	_.use<SScope>().byCasting();

	_.use<SObjectComposite>().byConfiguring<ObjectCompositer>()
		->withOffset<Binding>("binding", offset_of(&lisp::Function::_binding));

	_.defaults();
}

lisp::Function::Function()
{
}

void lisp::Function::setBody(instance<SCultSemanticNode> body)
{
	_body = _ast(body);
}
instance<SCultSemanticNode> lisp::Function::bodyAst() const
{
	return _body;
}

size_t lisp::Function::argCount() const
{
	return _args.size();
}
void lisp::Function::pushArg(instance<SCultSemanticNode> arg)
{
	_args.push_back(_ast(arg));
}
instance<SCultSemanticNode> lisp::Function::argAst(size_t index) const
{
	return _args[index];
}

bool lisp::Function::hasFreeBindings() const
{
	return _freeBindings.size() != 0;
}

types::ExpressionStore lisp::Function::subroutine_signature() const
{
	auto rc = argCount();
	std::vector<IExpression*> args;
	for (size_t i =0; i < rc; ++i)
	{
		auto a = argAst(i);
		if (!(a && a.isType<BindSite>())) throw stdext::exception("Malformed Argument");
		auto val = a.asType<BindSite>()->valueAst();
		if (!(val && val.isType<Variable>())) throw stdext::exception("Malformed Argument");

		auto var = val.asType<Variable>();

		auto type = var->typeAst();
		if (!type)
		{
			args.push_back(&ExpressionAny::Value);
		}
		else
		{
			args.push_back(&ExpressionAny::Value);
		}
		//a->
	}

	// TODO Infer or extract return type
	auto retType = &ExpressionAny::Value;

	return ExpressionStore(new ExpressionArrow(new ExpressionTuple(args), retType));
}

void lisp::Function::bind()
{
	_parentScope = SScope::findScope(_parent);

	for (auto arg : _args)
	{
		arg->bind();
	}

	_body->bind();

	// TODO build a visitor helper:
	typedef std::tuple<instance<SCultSemanticNode>, size_t> t_VistEntry;
	std::deque<t_VistEntry> _visitStack;
	_visitStack.push_front({ _body, 0 });
	
	while (!_visitStack.empty())
	{
		auto const top = _visitStack.front();
		auto const inst = std::get<0>(top);
		auto const count = std::get<1>(top);
		_visitStack.pop_front();

		if (inst.isNull())
			continue;
		else if (inst.isType<Resolve>())
		{
			auto i = inst.asType<Resolve>();
			auto bind = i->getBinding();
			auto scope = bind->getScope();

			if (scope.isType<CultSemantics>()
				|| (scope.isType<lisp::Function>() && scope.asType<lisp::Function>() == craft_instance()))
				continue;

			auto it = std::find_if(_visitStack.begin(), _visitStack.end(),
				[scope](t_VistEntry a) -> bool
				{
					instance<SCultSemanticNode> ai = std::get<0>(a);
					return ai.hasFeature<SScope>() && (instance<>)(ai) == (instance<>)(scope);
				});

			if (it == _visitStack.end())
			{
				this->_freeBindings.push_back(bind);
			}

			continue;
		}
		else if (inst.isType<Block>())
		{
			auto i = inst.asType<Block>();
			if (count == i->statementCount())
				continue;

			auto v = i->statementAst(count);

			_visitStack.push_front({ inst, count + 1 });
			_visitStack.push_front({ v, 0 });
			continue;
		}
		else if (inst.isType<CallSite>())
		{
			auto i = inst.asType<CallSite>();
			if (count == i->argCount())
				continue;

			auto v = i->argAst(count);

			_visitStack.push_front({ inst, count + 1 });
			_visitStack.push_front({ v, 0 });
			continue;
		}
	}
}

instance<Binding> lisp::Function::getBinding() const
{
	return _binding;
}
void lisp::Function::setBinding(instance<Binding> binding)
{
	_binding = binding;
}

instance<SScope> lisp::Function::getParentScope() const
{
	return _parentScope;
}
size_t lisp::Function::getSlotCount() const
{
	return argCount();
}

instance<Binding> lisp::Function::lookup(instance<Symbol> symbol) const
{
	return _simple_lookup(_symbols, symbol);
}
instance<Binding> lisp::Function::lookupSlot(size_t i) const
{
	return _symbols.bindings.at(i);
}
instance<Binding> lisp::Function::define(instance<Symbol> symbol, instance<BindSite> ast)
{
	return _simple_define(craft_instance(), _symbols, symbol, ast);
}

/******************************************************************************
** MultiMethod
******************************************************************************/
namespace craft {
namespace lisp
{
	class MultiMethodSubroutineProvider
		: public Implements<PSubroutine>::For<MultiMethod>
	{
		static ExpressionStore MultiMethodAny;

		virtual types::Function function(instance<> _) const override
		{
			throw stdext::exception("Needs to support closures....");
		}
		virtual types::ExpressionStore expression(instance<> _) const override
		{
			ExpressionStore any = (new ExpressionArrow(new ExpressionTuple({}, &ExpressionAny::Value), &ExpressionAny::Value));

			return any;
		}

		//
		// Performs a full call on the subroutine, including pushing to the current execution
		//
		virtual instance<> execute(instance<> _, types::GenericInvoke const& call) const override
		{
			instance<MultiMethod> mm = _;

			// TODO push a multimethod dispatch frame...

			std::vector<TypeId> exprs;
			exprs.reserve(call.args.size());
			std::transform(call.args.begin(), call.args.end(), std::back_inserter(exprs),
				[](instance<> const& inst) { return inst.typeId(); });

			auto res = mm->_dispatcher.dispatchWithRecord(exprs);
			auto entry = (MultiMethod::_Entry*)std::get<0>(res);

			if (entry == nullptr)
			{
				std::string dispatchList = stdext::join<char, std::vector<TypeId>::iterator>(
					std::string(", "), exprs.begin(), exprs.end(),
					[](auto it) { return it->toString(); });
				throw stdext::exception("{2}> Dispatch of `{0}` failed for [{1}].", mm->_binding->getSymbol()->getDisplay(), dispatchList, mm->sourceLocationToString());
			}

			return types::invoke(*std::get<1>(res), entry->function, call);
		}
	};
}}


CRAFT_DEFINE(MultiMethod)
{
	_.use<PSubroutine>().singleton<MultiMethodSubroutineProvider>();

	_.use<SCultSemanticNode>().byCasting();
	_.use<SBindable>().byCasting();

	_.defaults();
}

MultiMethod::MultiMethod()
{
}

void MultiMethod::attach(instance<BindSite> binding)
{
	auto value = binding->valueAst();

	if (value.isType<Constant>())
		value = value.asType<Constant>()->getValue();

	if (!value.hasFeature<PSubroutine>())
		throw stdext::exception("Bindsite `{0}` value is not a PSubroutine.", binding->getStaticSymbol()->getDisplay());

	auto psub = value.getFeature<PSubroutine>();
	
	auto it = _entries.insert({ psub->function(value), value, psub });
	_dispatcher.add(psub->expression(value), &*it);
}

instance<> MultiMethod::call_internal(types::GenericInvoke const& invoke) const
{
	std::vector<TypeId> exprs;
	exprs.reserve(invoke.args.size());
	std::transform(invoke.args.begin(), invoke.args.end(), std::back_inserter(exprs),
		[](instance<> const& inst) { return inst.typeId(); });

	auto res = _dispatcher.dispatchWithRecord(exprs);
	auto entry = (_Entry*)std::get<0>(res);

	if (entry == nullptr)
	{
		std::string dispatchList = stdext::join<char, std::vector<TypeId>::iterator>(
			std::string(", "), exprs.begin(), exprs.end(),
			[](auto it) { return it->toString(); });
		throw stdext::exception("{2}> Dispatch of `{0}` failed for [{1}].", _binding->getSymbol()->getDisplay(), dispatchList, sourceLocationToString());
	}

	return types::invoke(*std::get<1>(res), entry->function, invoke);
}

instance<Binding> MultiMethod::getBinding() const
{
	return _binding;
}
void MultiMethod::setBinding(instance<Binding> binding)
{
	_binding = binding;
}
