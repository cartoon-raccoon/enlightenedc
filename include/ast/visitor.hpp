#pragma once

#ifndef ECC_AST_VISITOR_H
#define ECC_AST_VISITOR_H

#include "abstract/visitor.hpp"

namespace ecc::ast {

class Program;
class AttributeArg;
class Attribute;
class Function;
class TypeDeclaration;
class VariableDeclaration;
class ParameterDeclaration;
class Declarator;
class ParenDeclarator;
class ArrayDeclarator;
class FunctionDeclarator;
class InitDeclarator;
class Pointer;
class ClassDeclarator;
class ClassDeclaration;
class Enumerator;
class StorageClassSpecifier;
class TypeQualifier;
class EnumSpecifier;
class ClassSpecifier;
class UnionSpecifier;
class TypeIdentifier;
class VoidSpecifier;
class PrimitiveSpecifier;
class Initializer;
class TypeName;
class IdentifierDeclarator;
class CompoundStatement;
class ExpressionStatement;
class CaseStatement;
class CaseRangeStatement;
class DefaultStatement;
class LabeledStatement;
class PrintStatement;
class IfStatement;
class SwitchStatement;
class WhileStatement;
class DoWhileStatement;
class ForStatement;
class GotoStatement;
class BreakStatement;
class ContinueStatement;
class ReturnStatement;
class BinaryExpression;
class CastExpression;
class UnaryExpression;
class AssignmentExpression;
class ConditionalExpression;
class IdentifierExpression;
class ConstExpression;
class LiteralExpression;
class StringExpression;
class CallExpression;
class MemberAccessExpression;
class ReinterpretExpression;
class ArraySubscriptExpression;
class PostfixExpression;
class SizeofExpression;

class ASTVisitor
    : public Visitor<
          ASTVisitor, Program, AttributeArg, Attribute, Function, TypeDeclaration,
          VariableDeclaration, ParameterDeclaration, Declarator, ParenDeclarator, ArrayDeclarator,
          FunctionDeclarator, InitDeclarator, Pointer, ClassDeclarator, ClassDeclaration,
          Enumerator, StorageClassSpecifier, TypeQualifier, EnumSpecifier, ClassSpecifier,
          UnionSpecifier, TypeIdentifier, VoidSpecifier, PrimitiveSpecifier, Initializer, TypeName,
          IdentifierDeclarator, CompoundStatement, ExpressionStatement, CaseStatement,
          CaseRangeStatement, DefaultStatement, LabeledStatement, PrintStatement, IfStatement,
          SwitchStatement, WhileStatement, DoWhileStatement, ForStatement, GotoStatement,
          BreakStatement, ContinueStatement, ReturnStatement, BinaryExpression, CastExpression,
          UnaryExpression, AssignmentExpression, ConditionalExpression, IdentifierExpression,
          ConstExpression, LiteralExpression, StringExpression, CallExpression,
          MemberAccessExpression, ReinterpretExpression, ArraySubscriptExpression,
          PostfixExpression, SizeofExpression> {};

} // namespace ecc::ast

#endif
