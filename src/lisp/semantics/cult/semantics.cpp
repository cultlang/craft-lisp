#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/semantics/cult/cult.h"
#include "lisp/semantics/cult/semantics.h"

using namespace craft;
using namespace craft::types;
using namespace craft::lisp;

/******************************************************************************
** CultSemantics
******************************************************************************/

CRAFT_DEFINE(CultSemantics)
{
	_.use<PSemantics>().singleton<CultSemanticsProvider>();
	_.use<SCultSemanticNode>().byCasting();
	_.use<SScope>().byCasting();

	_.defaults();
}


CultSemantics::CultSemantics(instance<lisp::Module> forModule)
{
	_module = forModule;
}

void CultSemantics::rebuildModulesCache()
{
	SPDLOG_TRACE(_module->getEnvironment()->log(),
		"CultSemantics::rebuildModulesCache");
	std::set<instance<Module>> result_set;
	std::list<instance<Module>> working_list;
	std::copy(_modules.begin(), _modules.end(), std::inserter(working_list, working_list.end()));

	while (!working_list.empty())
	{
		auto cur = working_list.front(); working_list.pop_front();
		auto sem = cur->get<CultSemantics>();

		if (result_set.find(cur) != result_set.end() || !sem)
			continue;
		result_set.insert(cur);

		for (auto module : sem->_modules)
		{
			working_list.push_back(module);
		}
	}

	std::copy(result_set.begin(), result_set.end(), std::inserter(_modules_cache, _modules_cache.end()));
}

instance<SCultSemanticNode> CultSemantics::read_cultLisp(ReadState* rs, instance<> syntax)
{
	instance<SCultSemanticNode> node;

	if (syntax.isType<Symbol>() && !syntax.asType<Symbol>()->isKeyword())
	{
		instance<Symbol> symbol = syntax;

		// '.' seperates resolutions
		std::vector<uint32_t> parts;
		auto push_parts = [&]()
		{
			if (node)
				node = instance<Member>::make(node, Symbol::makeSymbol(parts), Resolve::Mode::ResolveAndGet);
			else
				node = instance<Resolve>::make(Symbol::makeSymbol(parts), Resolve::Mode::ResolveAndGet);
			parts.clear();
		};
		
		auto const size = symbol->size();
		for (size_t i =0; i < size; ++i)
		{
			auto sym = symbol->at(i);
			if (Symbol::isPath(sym) && Symbol::toChar(sym) == '.')
				push_parts();
			else
				parts.push_back(sym);
		}
		push_parts();
	}
	else if (syntax.isType<Sexpr>())
	{
		instance<Sexpr> expr = syntax;
		auto size = expr->cells.size();

		if (size == 0)
			throw stdext::exception("Unquoted empty list.");

		// -- Read Time Dispatch --
		instance<> head = expr->cells[0];
		if (head.isType<Symbol>())
		{
			instance<Binding> binding = lookup_recurse(head.asType<Symbol>()); // TODO: read state lookup of macros?
			if (binding)
			{
				auto siteValue = binding->getSite()->valueAst();

				// Special Form
				if (siteValue.isType<SpecialForm>())
				{
					node = siteValue.asType<SpecialForm>()->_read(rs, expr);
				}
				// Macro Form
				else if (siteValue.isType<Macro>())
				{
					throw stdext::exception("Macros not finished.");
					//	ast = inspect_head.asType<Macro>()->expand(scope, expr->cells);
					//	return read(scope, ast);
				}
			}
		}

		// -- Call Site --
		if (!node)
			node = instance<CallSite>::make(read_cultLisp(rs, head), rs->readAll(expr));

		if (expr->cell_locs.size() != 0)
		{
			node->_source_start = expr->cell_locs[0];
			node->_source_end = expr->cell_locs[expr->cell_locs.size() - 1];
		}
	}
	else // Raw Value
		node = instance<Constant>::make(syntax);

	return node;
}

void CultSemantics::readPrepDefaults()
{
	SPDLOG_TRACE(_module->getEnvironment()->log(),
		"CultSemantics::readPrepDefaults\t({0})", _module);

	// These are implict, and must be made available by these execution engine.
	importModule(_module->getEnvironment()->importModule(_module, "builtin:cult/system"));
	importModule(_module->getEnvironment()->importModule(_module, "builtin:cult/core"));
}

void CultSemantics::read(instance<CultLispSyntax> syntax, PSemantics::ReadOptions const* opts)
{
	// TODO, make this executed by the interpreter with some special understanding about accessing
	//  macros and a different set of special forms
	// TODO, make this execute on one node at a time (e.g. to prevent blowing the stack) should
	//  also allow it to be re-entrant
	// TODO, make this handle reloading better.

	_ast.clear();
	_bindings.bindings.clear();
	_bindings.table.clear();
	_source = syntax;

	SPDLOG_TRACE(_module->getEnvironment()->log(),
		"CultSemantics::read\t({0})", _module);

	for (auto syntax : syntax->getRootForms())
	{
		ReadState rs { craft_instance() };

		auto ret = read_cultLisp(&rs, syntax);

		_ast.push_back(_astbind(ret));
	}
}

instance<SCultSemanticNode> CultSemantics::getParent() const
{
	return instance<>();
}
void CultSemantics::setParent(instance<SCultSemanticNode> parent)
{
	throw parent_already_set_error(craft_instance());
}



instance<CultSemantics> CultSemantics::getSemantics() const
{
	return craft_instance();
}
instance<SScope> CultSemantics::getParentScope() const
{
	return instance<>();
}
size_t CultSemantics::getSlotCount() const
{
	return _bindings.bindings.size();
}

instance<Binding> CultSemantics::lookup_local(instance<Symbol> symbol) const
{
	return _simple_lookup(_bindings, symbol);
}
instance<Binding> CultSemantics::lookup(instance<Symbol> symbol) const
{
	auto res = lookup_local(symbol);
	if (!res)
	{
		for (auto m : _modules_cache)
		{
			auto sem = m->get<CultSemantics>();
			if (sem)
			{
				res = sem->lookup_local(symbol);
				if (res)
					break;
			}
		}
	}
	return res;
}
instance<Binding> CultSemantics::lookupSlot(size_t index) const
{
	return lookup(index);
}
instance<Binding> CultSemantics::define(instance<Symbol> symbol, instance<BindSite> ast)
{
	return _simple_define(craft_instance(), _bindings, symbol, ast);
}

std::vector<instance<Binding>> CultSemantics::search(std::string const & search) const
{
	// TODO search symbols using a trie, and then look for those symbols in modules.
	// TODO search for namespace prefix

	auto const size = search.size();
	auto const& symbols = _module->getEnvironment()->symbolStore;

	std::vector<instance<Binding>> res;
	for (auto& it : _bindings.table)
	{
		auto const& sym = symbols.getValue((uint32_t)it.first);

		if (size <= sym.size() && search == sym.substr(0, size))
			res.push_back(_bindings.bindings[it.second]);
	}

	for (auto module : _modules)
	{
		auto sem = module->get<CultSemantics>();
		if (sem)
		{
			auto mres = sem->search(search);
			std::copy(mres.begin(), mres.end(), std::back_inserter(res));
		}
	}

	return res;
}

void CultSemantics::importModule(instance<Module> m)
{
	SPDLOG_TRACE(_module->getEnvironment()->log(),
		"CultSemantics::importModule\t({0}, {1})", _module, m);

	if (std::find(_modules.begin(), _modules.end(), m) != _modules.end())
		return;

	_modules.insert(_modules.begin(), m);
	rebuildModulesCache();
}

size_t CultSemantics::append(instance<CultSemantics> sem)
{
	size_t start = _ast.size();

	for (auto appending_ast : sem->_ast)
	{
		_ast.push_back(_astclonebind(appending_ast));
	}

	for (auto other_imported_module : sem->_modules)
	{
		if (other_imported_module == _module)
			continue;
		importModule(other_imported_module);
	}

	return start;
}
std::vector<size_t> CultSemantics::merge(instance<CultSemantics> sem)
{
	return {}; // TODO implement merge
}

/******************************************************************************
** CultSemanticsProvider
******************************************************************************/

instance<lisp::Module> CultSemanticsProvider::getModule(instance<> semantics) const
{
	return semantics.asType<CultSemantics>()->getModule();
}

instance<> CultSemanticsProvider::read(instance<lisp::Module> into, ReadOptions const* opts) const
{
	auto building = instance<CultSemantics>::make(into);
	
	auto loader = into->getLoader();
	auto prepReplacedDefault = loader.getFeature<PModuleLoader>()->prepSemantics(loader, building);
	if (!prepReplacedDefault) building->readPrepDefaults();

	auto syntax = into->get<CultLispSyntax>();
	if (syntax)
	{
		building->read(syntax, opts);
	}
	else
	{
		throw stdext::exception("No known syntax to build semantics from.");
	}

	return building;
}

instance<> CultSemanticsProvider::lookup(instance<> semantics_, instance<Symbol> sym) const
{
	instance<CultSemantics> semantics = semantics_;

	return semantics->lookup(sym)->getSite()->valueAst();
}

/******************************************************************************
** CultSemantics Helpers
******************************************************************************/

void CultSemantics::builtin_addSpecialForm(std::string const& symbol_name)
{
	auto symbol = Symbol::makeSymbol(symbol_name);
	auto bindsite = instance<BindSite>::make(symbol, instance<SpecialForm>::makeThroughLambda([](auto p) { return new (p) SpecialForm(); }));
	_ast.push_back(_astbind(bindsite));
}
void CultSemantics::builtin_specialFormReader(std::string const& symbol_name, CultSemantics::f_specialFormReader reader)
{
	auto binding = lookup(Symbol::makeSymbol(symbol_name));
	auto value = binding->getSite()->valueAst().asType<SpecialForm>()->_read = reader;
}

void CultSemantics::builtin_addMultiMethod(std::string const& symbol_name, types::cpp::Multimethod<types::ExpressionDispatcher>* existing)
{
	auto symbol = Symbol::makeSymbol(symbol_name);
	auto bindsite = instance<BindSite>::make(symbol, instance<MultiMethod>::make());
	_ast.push_back(_astbind(bindsite));
}
void CultSemantics::builtin_attachMultiMethod(std::string const& symbol_name, std::tuple<types::ExpressionStore, types::Function> impl)
{
	auto symbol = Symbol::makeSymbol(symbol_name);
	auto binding = lookup(symbol);
	if (!binding)
		builtin_addMultiMethod(symbol_name);
	binding = lookup(symbol);

	auto value = binding->getSite()->valueAst();
	if (!value.isType<MultiMethod>())
		throw stdext::exception("Can't attach to not a multimethod.");

	auto multi = value.asType<MultiMethod>();

	auto bindsite = instance<BindSite>::make(symbol, instance<Constant>::make(instance<CppFunctionPointer>::make(std::get<0>(impl), std::get<1>(impl))));
	_ast.push_back(_astbind(bindsite));
}

void CultSemantics::builtin_defineConstant(std::string const& symbol_name, instance<> constantValue)
{
	auto symbol = Symbol::makeSymbol(symbol_name);
	auto bindsite = instance<BindSite>::make(symbol, instance<Constant>::make(constantValue));
	_ast.push_back(_astbind(bindsite));
}

void CultSemantics::builtin_eval(std::string const& contents)
{

}

/******************************************************************************
** SpecialForm
******************************************************************************/


CRAFT_DEFINE(SpecialForm)
{
	_.use<SCultSemanticNode>().byCasting();

	_.defaults();
}

SpecialForm::SpecialForm()
{

}

instance<SCultSemanticNode> SpecialForm::getParent() const
{
	return _parent;
}
void SpecialForm::setParent(instance<SCultSemanticNode> parent)
{
	if (_parent) throw parent_already_set_error(craft_instance());
	_parent = parent;
}
