#pragma once
#include "lisp/common.h"
#include "lisp/lisp.h"
#include "lisp/semantics/cult/cult.h"

namespace craft {
namespace lisp
{
	/******************************************************************************
	** Variable
	******************************************************************************/

	/*
		Variable define node
	*/
	class Variable
		: public virtual craft::types::Object
		, public craft::types::Implements<SCultSemanticNode>
		, public craft::types::Implements<SBindable>
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Variable);
	private:
		instance<Binding> _binding;

	private:

		instance<SCultSemanticNode> _initalizer;
		instance<SCultSemanticNode> _type;

	public:
		CRAFT_LISP_EXPORTED Variable(instance<SCultSemanticNode> initalizer = instance<>(), instance<SCultSemanticNode> type = instance<>());
		CRAFT_LISP_EXPORTED void craft_setupInstance();

		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> initalizerAst() const;
		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> typeAst() const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual void bind() override;

		// SBindable
	public:
		CRAFT_LISP_EXPORTED virtual instance<Binding> getBinding() const override;
		CRAFT_LISP_EXPORTED virtual void setBinding(instance<Binding>) override;

	};

	/******************************************************************************
	** Resolve
	******************************************************************************/

	/*
		Resolves the value of a symbol.
	*/
	class Resolve
		: public virtual craft::types::Object
		, public craft::types::Implements<SCultSemanticNode>
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Resolve);
	public:
		// TODO make expected evaluation results inspectable to issue gets as necessary
		// Also remove from member
		enum class Mode
		{
			ResolveOnly = 0,
			ResolveAndGet = 1,
		};

	private:
		instance<Symbol> _symbol;
		Mode _mode;

		instance<Binding> _binding;

	public:
		CRAFT_LISP_EXPORTED Resolve(instance<Symbol> binding, Mode mode = Mode::ResolveOnly);

		CRAFT_LISP_EXPORTED bool isGetter();

		CRAFT_LISP_EXPORTED instance<Symbol> getSymbol() const;
		CRAFT_LISP_EXPORTED instance<Binding> getBinding() const;

		CRAFT_LISP_EXPORTED instance<> getConstantValue() const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual void bind() override;
	};


	/******************************************************************************
	** Member
	******************************************************************************/

	/*
		Resolves the value of a symbol.
	*/
	class Member
		: public virtual craft::types::Object
		, public craft::types::Implements<SCultSemanticNode>
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Member);

	private:
		instance<SCultSemanticNode> _object;
		instance<Symbol> _symbol;
		Resolve::Mode _mode;

	public:
		CRAFT_LISP_EXPORTED Member(instance<SCultSemanticNode> object, instance<Symbol> member, Resolve::Mode mode = Resolve::Mode::ResolveOnly);
		CRAFT_LISP_EXPORTED void craft_setupInstance();

		CRAFT_LISP_EXPORTED bool isGetter();

		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> objectAst() const;
		CRAFT_LISP_EXPORTED instance<Symbol> getSymbol() const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual void bind() override;
	};

	/******************************************************************************
	** Assign
	******************************************************************************/

	/*
		Assigns into a slot (or symbol).
	*/
	class Assign
		: public virtual craft::types::Object
		, public craft::types::Implements<SCultSemanticNode>
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Assign);
	public:

	private:
		instance<SCultSemanticNode> _slot;
		instance<SCultSemanticNode> _value;

	public:
		CRAFT_LISP_EXPORTED Assign(instance<SCultSemanticNode> slot, instance<SCultSemanticNode> value);
		CRAFT_LISP_EXPORTED void craft_setupInstance();

		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> slotAst() const;
		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> valueAst() const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual void bind() override;
	};

	/******************************************************************************
	** Block
	******************************************************************************/

	/*
		A do block node.
	*/
	class Block
		: public virtual craft::types::Object
		, public craft::types::Implements<SCultSemanticNode>
		, public craft::types::Implements<SScope>
	{
		CRAFT_LISP_EXPORTED CRAFT_OBJECT_DECLARE(craft::lisp::Block);
	private:
		instance<SScope> _parentScope;
		_SimpleSymbolTableBindings _symbols;

	private:
		std::vector<instance<SCultSemanticNode>> _statements;

	public:
		CRAFT_LISP_EXPORTED Block();

		CRAFT_LISP_EXPORTED void preSize(size_t);
		CRAFT_LISP_EXPORTED void push(instance<SCultSemanticNode>);

		CRAFT_LISP_EXPORTED size_t statementCount() const;
		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> statementAst(size_t index) const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual void bind() override;

		// SScope
	public:
		CRAFT_LISP_EXPORTED virtual instance<SScope> getParentScope() const override;
		CRAFT_LISP_EXPORTED virtual size_t getSlotCount() const override;

		CRAFT_LISP_EXPORTED virtual instance<Binding> lookup(instance<Symbol>) const override;
		CRAFT_LISP_EXPORTED virtual instance<Binding> lookupSlot(size_t) const override;
		CRAFT_LISP_EXPORTED virtual instance<Binding> define(instance<Symbol> symbol, instance<BindSite> ast) override;
	};
}}
