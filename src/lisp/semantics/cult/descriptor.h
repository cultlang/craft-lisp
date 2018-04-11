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
		instance<SCultSemanticNode> _parent;

		instance<Binding> _binding;

	public:

		instance<SCultSemanticNode> _initalizer;

	public:
		CRAFT_LISP_EXPORTED Variable();

		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> initalizerAst() const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual instance<SCultSemanticNode> getParent() const override;
		CRAFT_LISP_EXPORTED virtual void setParent(instance<SCultSemanticNode>) override;

		// SBindable
	public:
		CRAFT_LISP_EXPORTED virtual instance<Binding> getBinding() const override;
		CRAFT_LISP_EXPORTED virtual void setBinding(instance<Binding>) const override;

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
		instance<SCultSemanticNode> _parent;

		// TODO: Symbol equality
		std::map<size_t, instance<Binding>> _symbolTable; // Internal table

	public:

		std::vector<instance<SCultSemanticNode>> _statements;

	public:
		CRAFT_LISP_EXPORTED Block();

		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> statementCount() const;
		CRAFT_LISP_EXPORTED instance<SCultSemanticNode> statementAst(size_t index) const;

		// SCultSemanticNode
	public:
		CRAFT_LISP_EXPORTED virtual instance<SCultSemanticNode> getParent() const override;
		CRAFT_LISP_EXPORTED virtual void setParent(instance<SCultSemanticNode>) override;

		// SScope
	public:
		CRAFT_LISP_EXPORTED virtual instance<CultSemantics> getSemantics() const override;
		CRAFT_LISP_EXPORTED virtual instance<SScope> getParentScope() const override;

		// E.g. may enclose over other higher scopes
		CRAFT_LISP_EXPORTED virtual bool isLexicalScope() const override;

		CRAFT_LISP_EXPORTED virtual instance<Binding> lookup(instance<Symbol>) override;
		CRAFT_LISP_EXPORTED virtual instance<Binding> define(instance<Symbol> symbol, instance<> ast) override;
	};
}}
