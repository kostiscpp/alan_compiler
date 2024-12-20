#ifndef __AST_HPP__
#define __AST_HPP__

#include <iostream>
#include <vector>
#include <string>
#include <llvm/IR/Value.h> 
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include "../lexer/lexer.hpp"
#include "../symbol/types.hpp"
#include "../symbol/symbol.hpp"
#include "../codegen/codegen.hpp"

std::string compareToString(compare op);

// AST Base Class
class AST
{
public:
    int line;
    int column;
    AST(int line, int column) : line(line), column(column) {}
    virtual ~AST() {}
    virtual void sem() {}
    virtual llvm::Value* igen() const { return nullptr; } 
    void llvm_igen(bool optimize = false);
    static llvm::LLVMContext TheContext;
    void codegenLibs();
protected:
    static llvm::IRBuilder<> Builder;
    static std::unique_ptr<llvm::Module> TheModule;
    static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;
    std::string filename;
    static llvm::Type *proc;
    static llvm::Type *i1;
    static llvm::Type *i8;
    static llvm::Type *i32;
    static GenScope scopes;
    static std::stack<GenBlock*> blockStack;
    static llvm::ConstantInt* c1(bool b); 
    static llvm::ConstantInt* c8(char c);
    static llvm::ConstantInt* c32(int n);
};

// Expr Class
class Expr : public AST
{
public:
    Expr(int line, int column) : AST(line, column) {}
    virtual ~Expr() {}
    virtual void sem() override = 0;
    virtual llvm::Value* igen() const override = 0;
    virtual std::string* getName() const { return nullptr; }
    Type *getType() const;
    TypeEnum getTypeEnum() const;

protected:
    Type *type;
};

// Stmt Class
class Stmt : public AST
{
public:
    Stmt(int line, int column) : AST(line, column), external(false), isReturn(false), fromIf(false) {}
    virtual ~Stmt() {}
    virtual void sem() override = 0;
    virtual llvm::Value* igen() const override = 0;
    void setExternal(bool e);
    bool getExternal() const;
    bool isReturnStatement() const;
    void setFromIf(bool fromIf);
protected:
    bool external;
    bool isReturn;
    bool fromIf;
};

// StmtList Class
class StmtList : public Stmt
{
public:
    StmtList(int line, int column);  
    ~StmtList();
    void append(Stmt *stmt);
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
private:
    std::vector<Stmt *> stmts;
};

// LocalDef Class
class LocalDef : public AST
{
public:
    LocalDef(int line, int column) : AST(line, column) {}
    virtual ~LocalDef() {}
    virtual void sem() override = 0;

protected:
    Type *type;
};

// LocalDefList Class
class LocalDefList : public AST
{
public:
    LocalDefList(int line, int column);
    ~LocalDefList();
    void append(LocalDef *def);
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    std::vector<LocalDef *> defs;
};

// Fpar Class
class Fpar : public AST
{
public:
    Fpar(std::string *n, Type *t, ParameterType p, int line, int column);
    ~Fpar();
    virtual void sem() override;
    ParameterType getParameterType() const;
    std::string* getName() const;
    Type *getType() const;
private:
    ParameterType parameterType;
    std::string *name;
    Type *type;
    bool isArray;
};

class CapturedVar {
public:
    CapturedVar(const std::string& name, Type* type, bool isParam = false, ParameterType paramType = ParameterType::VALUE);

    const std::string& getName() const;
    Type* getType() const;
    bool getIsParam() const;
    ParameterType getParameterType() const;

private:
    std::string name; 
    Type* type;
    bool isParam;
    ParameterType parameterType;
};


// FparList Class
class FparList : public AST
{
public:
    FparList(int line, int column);
    ~FparList();
    void append(Fpar *f);
    virtual void sem() override;
    const std::vector<Fpar *> &getParameters() const;

private:
    std::vector<Fpar *> fpar;
};

// FuncDef Class
class FuncDef : public LocalDef
{
public:
    FuncDef(std::string *n, Type *t, LocalDefList *l, Stmt *s, FparList *f, int line, int column);
    ~FuncDef();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
    std::string* getName() const;
    void setReturn();

private:
    std::string *name;
    FparList *fpar;
    Type *type;
    LocalDefList *localDef;
    Stmt *stmts;
    bool hasReturn;
    std::vector<CapturedVar*> capturedVars;
};

// VarDef Class
class VarDef : public LocalDef
{
public:
    VarDef(std::string *n, Type *t, bool arr, int arraySize, int line, int column);
    ~VarDef();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    std::string *name;
    Type *type;
    int size;
    bool isArray;
};

// ExprList Class
class ExprList : public AST
{
public:
    ExprList(int line, int column);
    ~ExprList();
    void append(Expr *expr);
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
    const std::vector<Expr *> &getExprs() const;

private:
    std::vector<Expr *> exprs;
};

// Cond Class
class Cond : public AST 
{
public:
    Cond(int line, int column) : AST(line, column) {}
    virtual void sem() override = 0;
    virtual llvm::Value* igen() const override = 0;
};

// UnOp Class
class UnOp : public Expr 
{
public:
    UnOp(char o, Expr *e, int line, int column);
    ~UnOp();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    char op;
    Expr *expr;
};

// BinOp Class
class BinOp : public Expr 
{
public:
    BinOp(Expr *l, char o, Expr *r, int line, int column);
    ~BinOp();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    char op;
    Expr *left;
    Expr *right;
};

// CondCompOp Class
class CondCompOp : public Cond 
{
public:
    CondCompOp(Expr *l, compare o, Expr *r, int line, int column);
    ~CondCompOp();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    compare op;
    Expr *left;
    Expr *right;
};

// CondBoolOp Class
class CondBoolOp : public Cond
{
public:
    CondBoolOp(Cond *l, char o, Cond *r, int line, int column);
    ~CondBoolOp();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    char op;
    Cond *left;
    Cond *right;
};

// CondUnOp Class
class CondUnOp : public Cond
{
public:
    CondUnOp(char o, Cond *c, int line, int column);
    ~CondUnOp();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    char op;
    Cond *cond;
};

// IntConst Class
class IntConst : public Expr 
{
public:
    IntConst(int v, int line, int column);
    virtual void sem() override;
    int getValue() const;
    virtual llvm::Value* igen() const override;

private:
    int val;
};

// CharConst Class
class CharConst : public Expr 
{
public:
    CharConst(unsigned char c, int line, int column);
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    unsigned char val;
};

// Lval Class
class Lval : public Expr 
{
public:
    Lval(int line, int column) : Expr(line, column) {}
    virtual ~Lval() {}
    virtual void sem() override;
    virtual llvm::Value* igen() const override = 0;
    virtual std::string* getName() const override;

protected:
    std::string *name;
};

// StringConst Class
class StringConst : public Lval 
{
public:
    StringConst(std::string *v, int line, int column);
    ~StringConst();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
};

// BoolConst Class
class BoolConst : public Cond 
{
public:
    BoolConst(bool v, int line, int column);
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    bool val;
};

// Id Class
class Id : public Lval 
{
public:
    Id(std::string *n, int line, int column);
    ~Id();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    SymbolType symbolType;
};

// ArrayAccess Class
class ArrayAccess : public Lval 
{
public:
    ArrayAccess(std::string *n, Expr *index, int line, int column);
    ~ArrayAccess();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
    Expr *getIndexExpr() const;

private:
    Expr *indexExpr;
};

// Let Class
class Let : public Stmt 
{
public:
    Let(Lval *l, Expr *r, int line, int column);
    ~Let();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    Lval *lexpr;
    Expr *rexpr;
};

// FuncCall Class
class FuncCall : public Expr
{
public:
    FuncCall(std::string *n, ExprList *e, int line, int column);
    ~FuncCall();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;
    ExprList *getExprs() const;
    virtual std::string* getName() const override;

protected:
    std::string *name;
    ExprList *exprs;
    std::vector<CapturedVar*> capturedVars;
    bool isNested;
};

// ProcCall Class
class ProcCall : public Stmt 
{
public:
    ProcCall(FuncCall *f, int line, int column);
    ~ProcCall();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    FuncCall *funcCall;
};

// If Class
class If : public Stmt 
{
public:
    If(Cond *c, Stmt *t, Stmt *e, int line, int column);
    ~If();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    Cond *cond;
    Stmt *thenStmt;
    Stmt *elseStmt;
};

// While Class
class While : public Stmt 
{
public:
    While(Cond *c, Stmt *b, int line, int column);
    ~While();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    Cond *cond;
    Stmt *body;
};

// Return Class
class Return : public Stmt
{
public:
    Return(Expr *e, int line, int column);
    ~Return();
    virtual void sem() override;
    virtual llvm::Value* igen() const override;

private:
    Expr *expr;
};

// Empty Class
class Empty : public Stmt
{
public:
    Empty(int line, int column);
    virtual void sem() override {}
    virtual llvm::Value* igen() const override;

};

#endif // AST_HPP
