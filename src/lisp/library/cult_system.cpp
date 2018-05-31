#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/library/libraries.h"

#include "lisp/semantics/cult/cult.h"
#include "lisp/backend/BootstrapInterpreter.h"
//#include "lisp/backend/llvm/llvm_internal.h"

using namespace craft;
using namespace craft::types;
using namespace craft::lisp;
using namespace craft::lisp::library::helper;


instance<Module> library::make_module_builtin_cult_system(instance<Namespace> ns, instance<> loader)
{
	//
	// Aquire objects, set up returns
	//
	auto backend_interp = ns->get<BootstrapInterpreter>();

	auto ret = instance<Module>::make(ns, loader);
	auto sem = instance<CultSemantics>::make(ret);
	ret->builtin_setSemantics(sem);

	//
	// Core Types
	//
	sem->builtin_defineType<int8_t>("Int8");
	sem->builtin_defineType<int16_t>("Int16");
	sem->builtin_defineType<int32_t>("Int32");
	sem->builtin_defineType<int64_t>("Int64");

	sem->builtin_defineType<uint8_t>("UInt8");
	sem->builtin_defineType<uint16_t>("UInt16");
	sem->builtin_defineType<uint32_t>("UInt32");
	sem->builtin_defineType<uint64_t>("UInt64");
							
	sem->builtin_defineType<float>("Float32");
	sem->builtin_defineType<double>("Float64");
							
	sem->builtin_defineType<bool>("Bool");
	sem->builtin_defineType<std::string>("String");

	//
	// Multimethods
	//
	sem->builtin_addMultiMethod("is-subtype");

	sem->builtin_addMultiMethod("truth");
	sem->builtin_implementMultiMethod("truth",
		[](instance<bool> value) -> instance<bool>
	{
		return value;
	});
	sem->builtin_implementMultiMethod("truth",
		[](instance<> value) -> instance<bool>
	{
		return instance<bool>::make(!value.isNull());
	});
	sem->builtin_addMultiMethod("get");

	//
	// Multimethod Implementations
	//

	//
	// Special forms - Reader
	//
	// TODO, make this a multimethod
	sem->builtin_addSpecialForm("require");
	sem->builtin_specialFormReader("require",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		if (sexpr->cells.size() != 2 || !sexpr->cells[1].isType<std::string>())
			throw stdext::exception("malformed: (require \"<uri>\")");

		return ScopeManipulation::Require(*sexpr->cells[1].asType<std::string>());
	});
	sem->builtin_addSpecialForm("namespace");
	sem->builtin_specialFormReader("namespace",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		if (sexpr->cells.size() != 2 || !sexpr->cells[1].isType<std::string>())
			throw stdext::exception("malformed: (namespace \"<namespace>\")");

		return ScopeManipulation::SetNamespace(*sexpr->cells[1].asType<std::string>());
	});
	sem->builtin_addSpecialForm("using");
	sem->builtin_specialFormReader("using",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		if (sexpr->cells.size() != 2 || !sexpr->cells[1].isType<std::string>())
			throw stdext::exception("malformed: (namespace-using \"<namespace>\")");

		return ScopeManipulation::UsingNamespace(*sexpr->cells[1].asType<std::string>());
	});

	sem->builtin_addSpecialForm("define");
	sem->builtin_specialFormReader("define",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
		{
			bool force = false;
			if (sexpr->cells.size() != 3)
				throw stdext::exception("malformed: (define <symbol> <expr>)");

			instance<Symbol> symbol;
			try
			{
				symbol = library::helper::symbol(sexpr->cells[1]);
			}
			catch (std::exception const& ex)
			{
				throw stdext::exception(ex, "Not Implemented: Dynamic Bindings");
			}

			return instance<BindSite>::make(symbol, rs->read(sexpr, 2));
		});

	sem->builtin_addSpecialForm("do");
	sem->builtin_specialFormReader("do",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		auto ret = instance<Block>::make();

		auto size = sexpr->cells.size();
		ret->preSize(size - 1);
		for (auto i = 1; i < size; i++)
		{
			ret->push(rs->read(sexpr, i));
		}

		return ret;
	});

	sem->builtin_addSpecialForm("cond");
	sem->builtin_specialFormReader("cond",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		auto ret = instance<Condition>::make();

		auto size = sexpr->cells.size();
		ret->preSize(size / 2);
		for (auto i = 1; i + 1 < size; i += 2)
		{
			ret->push(rs->read(sexpr, i), rs->read(sexpr, i + 1));
		}
		if (size % 2 == 0)
			ret->push(instance<>(), rs->read(sexpr, size - 1));

		return ret;
	});

	sem->builtin_addSpecialForm("while");
	sem->builtin_specialFormReader("while",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		if (sexpr->cells.size() != 3)
			throw stdext::exception("malformed: (while <cond> <body>)");

		auto ret = instance<Loop>::make(rs->read(sexpr, 1), rs->read(sexpr, 2));

		return ret;
	});

	sem->builtin_addSpecialForm("type");
	sem->builtin_specialFormReader("type",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		auto ret = instance<TypeDescription>::make();
		for (auto it = sexpr->cells.begin() + 1; it != sexpr->cells.end(); it++)
		{
			auto slot_cell = *it;
			if (slot_cell.isType<Sexpr>())
			{
				auto cells = slot_cell.asType<Sexpr>();
				if (cells->cells.size() != 2
					|| !cells->cells[0].isType<Symbol>()
					|| !cells->cells[1].isType<Symbol>())
					throw stdext::exception("Malformed Slot Definition");


				auto slotname = library::helper::symbol(cells->cells[0]);
				if (slotname->isKeyword()) throw stdext::exception("Slot Extensions not implemented");

				ret->push(instance<BindSite>::make(slotname, 
					instance<Variable>::make(instance<>(), rs->read(sexpr, 1))));
			}
			else // Untyped Slot
			{
				auto symbol = library::helper::symbol(slot_cell);
				ret->push(instance<BindSite>::make(symbol, instance<Variable>::make()));
			}
		}
		
		return ret;
	});

	sem->builtin_addSpecialForm("function");
	sem->builtin_specialFormReader("function",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		auto ret = instance<Function>::make();

		// Read the arg expression

		// check for void args expression
		if (sexpr->cells[1].isType<Sexpr>())
		{
			// Special reed:
			for (auto arg_cell : sexpr->cells[1].asType<Sexpr>()->cells)
			{
				if (arg_cell.isType<Sexpr>())
				{

				}
				else
				{
					auto symbol = library::helper::symbol(arg_cell);
					ret->pushArg(instance<BindSite>::make(symbol, instance<Variable>::make()));
				}
			}
		}

		// Read the body
		auto body_block = instance<Block>::make();
		auto size = sexpr->cells.size();
		body_block->preSize(size - 2);
		for (auto i = 2; i < size; i++)
		{
			body_block->push(rs->read(sexpr, i));
		}
		ret->setBody(body_block);

		return ret;
	});

	sem->builtin_addSpecialForm("variable");
	sem->builtin_specialFormReader("variable",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		auto size = sexpr->cells.size();

		if (size > 3 || size < 2)
			throw stdext::exception("malformed: (variable <value> [<type>])");

		auto ret = instance<Variable>::make(
			(size > 1 ? rs->read(sexpr, 1) : instance<SCultSemanticNode>()),
			(size > 2 ? rs->read(sexpr, 2) : instance<SCultSemanticNode>())
		);

		return ret;
	});

	sem->builtin_addSpecialForm("set");
	sem->builtin_specialFormReader("set",
		[](CultSemantics::ReadState* rs, instance<Sexpr> sexpr) -> instance<SCultSemanticNode>
	{
		if (sexpr->cells.size() != 3)
			throw stdext::exception("malformed: (set <target> <value>)");

		auto symbol = library::helper::symbol(sexpr->cells[1]);

		auto ret = instance<Assign>::make(
			instance<Resolve>::make(symbol, Resolve::Mode::ResolveOnly),
			rs->read(sexpr, 2)
		);

		return ret;
	});

	//
	// Semantics - Interpreter
	//
	sem->builtin_addMultiMethod("exec");
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Constant> ast) -> instance<>
	{
		return ast->getValue();
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Resolve> ast) -> instance<>
	{
		auto slot = interp->slot(ast->getBinding());

		if (ast->isGetter())
		{
			// TODO call getter
			return slot->getValue();
		}

		return slot;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Assign> ast) -> instance<>
	{
		auto destination = interp->interp_exec(ast->slotAst());

		if (!destination.typeId().isType<RuntimeSlotReference>())
			throw stdext::exception("Slot AST failed....");

		auto value = interp->interp_exec(ast->valueAst());

		destination.asType<RuntimeSlotReference>()->setValue(value);

		return value;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<ScopeManipulation> ast) -> instance<>
	{
		// TODO call top-level here
		// ast

		return instance<>(); // return module value?
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Variable> ast) -> instance<>
	{
		return interp->interp_exec(ast->initalizerAst());
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<BindSite> ast) -> instance<>
	{
		auto symbol = ast->getStaticSymbol(); // TODO execute dynamic binding
		auto binding = SScope::findScope(ast)->lookup_recurse(symbol);

		auto value = interp->interp_exec(ast->valueAst());

		if (!ast->isAttachSite())
			interp->slot(binding)->setValue(value);
		return value;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<CallSite> ast) -> instance<>
	{
		auto callee = interp->interp_exec(ast->calleeAst());

		auto count = ast->argCount();
		GenericInvoke invoke(count);
		for (auto i = 0; i < count; i++)
		{
			invoke.args.push_back(interp->interp_exec(ast->argAst(i)));
		}

		instance<> value;
		try
		{
			value = interp->interp_call(callee, invoke);
		}
		catch (std::exception const& ex)
		{
			// TODO
			// Poor mans stack trace, theorhetically this should be inside interp call and be sent along with it, and the stack trace should come through stack introspection!
			std::string callee_name;
			if (callee.hasFeature<SBindable>())
			{
				callee_name = callee.asFeature<SBindable>()->getBinding()->getSymbol()->getDisplay();
			}
			throw stdext::exception(ex, "{0} {1}", ast->sourceLocationToString(), callee_name);
		}

		return value;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Block> ast) -> instance<>
	{
		InterpreterFrameSection::Push _hold(interp, ast, interp->top());

		auto count = ast->statementCount();
		instance<> last_res;
		for (auto i = 0; i < count; i++)
		{
			last_res = interp->interp_exec(ast->statementAst(i));
		}

		return last_res;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Condition> ast) -> instance<>
	{
		auto truth = interp->getBackend().asType<BootstrapInterpreter>()->builtin_truth;
		auto count = ast->branchCount();
		for (auto i = 0; i < count; i++)
		{
			auto cond = interp->interp_exec(ast->branchConditionAst(i));
			if (*interp->interp_call(truth, { cond }).asType<bool>())
			{
				return interp->interp_exec(ast->branchAst(i));
			}
		}

		return interp->interp_exec(ast->branchDefaultAst());
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Loop> ast) -> instance<>
	{
		auto truth = interp->getBackend().asType<BootstrapInterpreter>()->builtin_truth;
		
		instance<> last;
		while (*interp->interp_call(truth, { interp->interp_exec(ast->conditionAst()) }).asType<bool>())
		{
			last = interp->interp_exec(ast->bodyAst());
		}
		return last;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<Function> ast) -> instance<>
	{
		auto curSubframe = interp->top();
		if (!ast->hasFreeBindings() || curSubframe->getScope().isNull())
			return ast;

		return instance<SubroutineClosure>::make(curSubframe, ast);
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<TypeDescription> ast) -> instance<>
	{
		/*auto curSubframe = interp->top();
		if (!ast->hasFreeBindings() || curSubframe->getScope().isNull())
			return ast;

		return instance<SubroutineClosure>::make(curSubframe, ast);*/

		auto typebind = ast->getBinding();

		auto scope = SScope::findScope(typebind->getSite());
		if (!scope.isType<CultSemantics>()) throw stdext::exception("We don't support dynamic type declaration");


		auto module_ = interp->getEntryModule(0);
		auto uri = module_->uri();
		auto tname = typebind->getSymbol()->getDisplay();
		
		auto tfname = fmt::format("{1}", "", tname);
		
		auto graphNode = graph().getByIndex<GraphPropertyTypeName>(&tfname);

		if (graphNode) {
			//graphNode->
		}
		else
		{
			/*for(auto i = 0; i < ast->statementCount(); i++)
			{
				auto variable = ast->statementAst(i).asType<Variable>();
				auto vtype = variable->typeAst();

			}*/
			// Success
			auto node = graph().add<GraphNodeLispType>(ast.asInternalPointer());
			graph().add<GraphPropertyTypeName>(node, new std::string(tfname));
			//graph().add<GraphPropertyBuiltinModuleUri>(node, new std::string(uri));
			//graph().add
		}

		return ast;
	});
	sem->builtin_implementMultiMethod("exec",
		[](instance<InterpreterFrameSection> interp, instance<> ast) -> instance<>
	{
		return ast;
	});

	//
	// Semantics - Llvm
	//
	// TODO, make this a multimethod


	//
	// Validate, set state
	//
	backend_interp->builtin_validateSpecialForms(ret);
	//backend_llvm->builtin_validateSpecialForms(ret);

	ret->builtin_setSemantics(sem);
	return ret;
}
