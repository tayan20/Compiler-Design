#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::sys;

FILE *pFile;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  LBOX = int('['),  // left bracket
  RBOX = int(']'),  // right bracket
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

// TOKEN class is used to keep track of information about a token
class TOKEN {
public:
  TOKEN() = default;
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
  const std::string getIdentifierStr() const;
  const int getIntVal() const;
  const float getFloatVal() const;
  const bool getBoolVal() const;
};

static std::string globalLexeme;
static int lineNo, columnNo;

const std::string TOKEN::getIdentifierStr() const {
  if (type != IDENT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getIdentifierStr called on non-IDENT token");
    exit(2);
  }
  return lexeme;
}

const int TOKEN::getIntVal() const {
  if (type != INT_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getIntVal called on non-INT_LIT token");
    exit(2);
  }
  return strtod(lexeme.c_str(), nullptr);
}

const float TOKEN::getFloatVal() const {
  if (type != FLOAT_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getFloatVal called on non-FLOAT_LIT token");
    exit(2);
  }
  return strtof(lexeme.c_str(), nullptr);
}

const bool TOKEN::getBoolVal() const {
  if (type != BOOL_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getBoolVal called on non-BOOL_LIT token");
    exit(2);
  }
  return (lexeme == "true");
}

static TOKEN returnTok(std::string lexVal, int tok_type) {
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok() {

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) {
    if (LastChar == '\n' || LastChar == '\r') {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_')) { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    globalLexeme = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_')) {
      globalLexeme += LastChar;
      columnNo++;
    }

    if (globalLexeme == "int")
      return returnTok("int", INT_TOK);
    if (globalLexeme == "bool")
      return returnTok("bool", BOOL_TOK);
    if (globalLexeme == "float")
      return returnTok("float", FLOAT_TOK);
    if (globalLexeme == "void")
      return returnTok("void", VOID_TOK);
    if (globalLexeme == "bool")
      return returnTok("bool", BOOL_TOK);
    if (globalLexeme == "extern")
      return returnTok("extern", EXTERN);
    if (globalLexeme == "if")
      return returnTok("if", IF);
    if (globalLexeme == "else")
      return returnTok("else", ELSE);
    if (globalLexeme == "while")
      return returnTok("while", WHILE);
    if (globalLexeme == "return")
      return returnTok("return", RETURN);
    if (globalLexeme == "true") {
      //   BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (globalLexeme == "false") {
      //   BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }
    return returnTok(globalLexeme.c_str(), IDENT);
  }

  if (LastChar == '=') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.') { // Floatingpoint Number: .[0-9]+
      do {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      //   FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    } else {
      do { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.') { // Floatingpoint Number: [0-9]+.[0-9]+)
        do {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        // FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      } else { // Integer : [0-9]+
        // IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&') {
    NextChar = getc(pFile);
    if (NextChar == '&') { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|') {
    NextChar = getc(pFile);
    if (NextChar == '|') { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/') { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/') { // definitely a comment
      do {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    } else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

static TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

static void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

/// ASTnode - Base class for all AST nodes.
class ASTnode {

public:
  virtual ~ASTnode() {}
  virtual Value *codegen() { return nullptr; };
  virtual std::string to_string() const { return ""; };
};

/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public ASTnode {
  int Val;
  TOKEN Tok;
  std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return "IntLiteral(" + std::to_string(Val) + ")";
  }
};

/// BoolASTnode - Class for boolean literals true and false,
class BoolASTnode : public ASTnode {
  bool Bool;
  TOKEN Tok;

public:
  BoolASTnode(TOKEN tok, bool B) : Bool(B), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return "BoolLiteral(" + std::string(Bool ? "true" : "false") + ")";
  }
};

/// FloatASTnode - Node class for floating point literals like "1.0".
class FloatASTnode : public ASTnode {
  double Val;
  TOKEN Tok;

public:
  FloatASTnode(TOKEN tok, double Val) : Val(Val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return "FloatLiteral(" + std::to_string(Val) + ")";
  }
};

/// VariableASTnode - Class for referencing a variable (i.e. identifier), like
/// "a".
enum IDENT_TYPE { IDENTIFIER = 0 };
class VariableASTnode : public ASTnode {
protected:
  TOKEN Tok;
  std::string Name;
  IDENT_TYPE VarType;

public:
  VariableASTnode(TOKEN tok, const std::string &Name)
      : Tok(tok), Name(Name), VarType(IDENT_TYPE::IDENTIFIER) {}
  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Tok.lexeme; }
  const IDENT_TYPE getVarType() const { return VarType; }

  virtual std::string to_string() const override {
    return "Variable(" + Name + ")";
  }
};

/// ParamAST - Class for a parameter declaration
class ParamAST {
  std::string Name;
  std::string Type;

public:
  ParamAST(const std::string &name, const std::string &type)
      : Name(name), Type(type) {}
  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Type; }

  std::string to_string() const {
    return Type + " " + Name;
  }
};

/// DeclAST - Base class for declarations, variables and functions
class DeclAST : public ASTnode {

public:
  virtual ~DeclAST() {}
};

/// VarDeclAST - Class for a variable declaration
class VarDeclAST : public DeclAST {
  std::unique_ptr<VariableASTnode> Var;
  std::string Type;

public:
  VarDeclAST(std::unique_ptr<VariableASTnode> var, const std::string &type)
      : Var(std::move(var)), Type(type) {}
  const std::string &getType() const { return Type; }
  const std::string &getName() const { return Var->getName(); }

  virtual std::string to_string() const override {
    return "LocalVarDecl(" + Type + " " + Var->getName() + ")";
  }
};

/// GlobVarDeclAST - Class for a Global variable declaration
class GlobVarDeclAST : public DeclAST {
  std::unique_ptr<VariableASTnode> Var;
  std::string Type;

public:
  GlobVarDeclAST(std::unique_ptr<VariableASTnode> var, const std::string &type)
      : Var(std::move(var)), Type(type) {}
  const std::string &getType() const { return Type; }
  const std::string &getName() const { return Var->getName(); }

  virtual std::string to_string() const override {
    return "GlobalVarDecl(" + Type + " " + Var->getName() + ")";
  }
};

/// FunctionPrototypeAST - Class for a function declaration's signature
class FunctionPrototypeAST {
  std::string Name;
  std::string Type;
  std::vector<std::unique_ptr<ParamAST>> Params; // vector of parameters

public:
  FunctionPrototypeAST(const std::string &name, const std::string &type,
                       std::vector<std::unique_ptr<ParamAST>> params)
      : Name(name), Type(type), Params(std::move(params)) {}

  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Type; }
  int getSize() const { return Params.size(); }
  std::vector<std::unique_ptr<ParamAST>> &getParams() { return Params; }

  std::string to_string() const {
    std::string result = "FunctionProto(" + Type + " " + Name + "(";
    
    for (size_t i = 0; i < Params.size(); i++) {
      if (i > 0) result += ", ";
      result += Params[i]->to_string();
    } 
    result += "))";
    return result;
  }
};

/*
class ExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Node1;
  char Op;
  std::unique_ptr<ASTnode> Node2;

public:
  ExprAST(std::unique_ptr<ASTnode> node1, char op,
          std::unique_ptr<ASTnode> node2)
      : Node1(std::move(node1)), Op(op), Node2(std::move(node2)) {}
  const std::string &getType();
};
*/

class ExprAST : public ASTnode {
  TOKEN OpTok; // which operator: +. -, *, /, ==, &&, etc.
  std::unique_ptr<ASTnode> LHS;
  std::unique_ptr<ASTnode> RHS; // may be null for unary

  public:
    // binary operator constructor: lhs <op> rhs
    ExprAST(TOKEN opTok,
            std::unique_ptr<ASTnode> lhs,
            std::unique_ptr<ASTnode> rhs)
        : OpTok(opTok), LHS(std::move(lhs)), RHS(std::move(rhs)) {}
    
    // unary operator constructor: <op> rhs
    ExprAST(TOKEN opTok,
            std::unique_ptr<ASTnode> rhs)
        : OpTok(opTok), LHS(nullptr), RHS(std::move(rhs)) {}

    // optional helpers to inspect operator later in codegen
    TOKEN getOpToken() const { return OpTok; }

    virtual std::string to_string() const override {
      std::string opStr;
      switch(OpTok.type) {
        case PLUS: opStr = "+"; break;
        case MINUS: opStr = "-"; break;
        case ASTERIX: opStr = "*"; break;
        case DIV: opStr = "/"; break;
        case MOD: opStr = "%"; break;
        case LT: opStr = "<"; break;
        case LE: opStr = "<="; break;
        case GT: opStr = ">"; break;
        case GE: opStr = ">="; break;
        case EQ: opStr = "=="; break;
        case NE: opStr = "!="; break;
        case AND: opStr = "&&"; break;
        case OR: opStr = "||"; break;
        case NOT: opStr = "!"; break;
        default: opStr = "?"; break;
      }

      if (LHS) {
        //binary operator
        return "BinaryExpr(" + opStr + " " + LHS->to_string() + ", " + RHS->to_string() + ")";
      } else {
        // unary operator
        return "UnaryExpr(" + opStr + ", " + RHS->to_string() + ")";
      }
    }
};

class AssignExprAST : public ASTnode {
  std::unique_ptr<VariableASTnode> LHS;
  std::unique_ptr<ASTnode> RHS;

  public:
    AssignExprAST(std::unique_ptr<VariableASTnode> lhs,
                  std::unique_ptr<ASTnode> rhs)
        :LHS(std::move(lhs)), RHS(std::move(rhs)) {}

    virtual std::string to_string() const override {
      return "Assignment(" + LHS->getName() + " = " + RHS->to_string() + ")";
    }
};

/// BlockAST - Class for a block with declarations followed by statements
class BlockAST : public ASTnode {
  std::vector<std::unique_ptr<VarDeclAST>> LocalDecls; // vector of local decls
  std::vector<std::unique_ptr<ASTnode>> Stmts;         // vector of statements

public:
  BlockAST(std::vector<std::unique_ptr<VarDeclAST>> localDecls,
           std::vector<std::unique_ptr<ASTnode>> stmts)
      : LocalDecls(std::move(localDecls)), Stmts(std::move(stmts)) {}

  virtual std::string to_string() const override {
    std::string result = "Block {\n";
    result += "  LocalDecls:\n";
    for (const auto& decl : LocalDecls) {
      result += "    " + decl->to_string() + "\n";
    }
    result += "  Statements:\n";
    for (const auto& stmt : Stmts) {
      if (stmt) result += "    " + stmt->to_string() + "\n";
    }
    result += "}";
    return result;
  }
};

/// FunctionDeclAST - This class represents a function definition itself.
class FunctionDeclAST : public DeclAST {
  std::unique_ptr<FunctionPrototypeAST> Proto;
  std::unique_ptr<ASTnode> Block;

public:
  FunctionDeclAST(std::unique_ptr<FunctionPrototypeAST> Proto,
                  std::unique_ptr<ASTnode> Block)
      : Proto(std::move(Proto)), Block(std::move(Block)) {}

  virtual std::string to_string() const override {
    return "FunctionDecl(\n  " + Proto->to_string() + ",\n  " + Block->to_string() + "\n)";
  }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then,
            std::unique_ptr<ASTnode> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
  
  virtual std::string to_string() const override {
    std::string result = "If(\n  Condition: " + Cond->to_string() + ",\n";
    result += "  Then: " + Then->to_string();
    if (Else) {
      result += ",\n Else: " + Else->to_string();
    }
    result += "\n)";
    return result;
  }
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Body;

public:
  WhileExprAST(std::unique_ptr<ASTnode> cond, std::unique_ptr<ASTnode> body)
      : Cond(std::move(cond)), Body(std::move(body)) {}
  
  virtual std::string to_string() const override {
    return "While(\n  Condition: " + Cond->to_string() + ",\n Body: " + Body->to_string() + "\n)";
  }
};

/// ReturnAST - Class for a return value
class ReturnAST : public ASTnode {
  std::unique_ptr<ASTnode> Val;

public:
  ReturnAST(std::unique_ptr<ASTnode> value) : Val(std::move(value)) {}

  virtual std::string to_string() const override {
    if (Val) {
      return "Return(" + Val->to_string() + ")";
    } else {
      return "Return(void)";
    }
  }
};

/// ArgsAST - Class for a function argumetn in a function call
class ArgsAST : public ASTnode {
  std::string Callee;
  std::vector<std::unique_ptr<ASTnode>> ArgsList;

public:
  ArgsAST(const std::string &Callee, std::vector<std::unique_ptr<ASTnode>> list)
      : Callee(Callee), ArgsList(std::move(list)) {}

  virtual std::string to_string() const override {
    std::string result = "FunctionCall(" + Callee + "(";
    for (size_t i = 0; i < ArgsList.size(); i++) {
      if (i > 0) result += ", ";
      result += ArgsList[i]->to_string();
    }
    result+= "))";
    return result;
  }
};

// Global storage for AST nodes
static std::vector<std::unique_ptr<ASTnode>> ProgramAST;
static std::vector<std::unique_ptr<FunctionPrototypeAST>> ExternAST;

/// LogError* - These are little helper function for error handling.
std::unique_ptr<ASTnode> LogError(TOKEN tok, const char *Str) {
  fprintf(stderr, "%d:%d Error: %s\n", tok.lineNo, tok.columnNo, Str);
  exit(2);
  return nullptr;
}

std::unique_ptr<FunctionPrototypeAST> LogErrorP(TOKEN tok, const char *Str) {
  LogError(tok, Str);
  exit(2);
  return nullptr;
}

std::unique_ptr<ASTnode> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  exit(2);
  return nullptr;
}

// Global storage for the AST - stores all top-level declarations
static std::vector<std::unique_ptr<ASTnode>> TopLevelDecls;
static std::vector<std::unique_ptr<FunctionPrototypeAST>> ExternDecls;


//===----------------------------------------------------------------------===//
// Recursive Descent - Function call for each production
//===----------------------------------------------------------------------===//

static std::unique_ptr<ASTnode> ParseDecl();
static std::unique_ptr<ASTnode> ParseStmt();
static std::unique_ptr<ASTnode> ParseBlock();
static std::unique_ptr<ASTnode> ParseExper();
static std::unique_ptr<ParamAST> ParseParam();
static std::unique_ptr<VarDeclAST> ParseLocalDecl();
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime();

// element ::= FLOAT_LIT
static std::unique_ptr<ASTnode> ParseFloatNumberExpr() {
  auto Result = std::make_unique<FloatASTnode>(CurTok, CurTok.getFloatVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// element ::= INT_LIT
static std::unique_ptr<ASTnode> ParseIntNumberExpr() {
  auto Result = std::make_unique<IntASTnode>(CurTok, CurTok.getIntVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// element ::= BOOL_LIT
static std::unique_ptr<ASTnode> ParseBoolExpr() {
  auto Result = std::make_unique<BoolASTnode>(CurTok, CurTok.getBoolVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// param_list_prime ::= "," param param_list_prime
//                   |  ε
static std::vector<std::unique_ptr<ParamAST>> ParseParamListPrime() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  if (CurTok.type == COMMA) { // more parameters in list
    getNextToken();           // eat ","

    auto param = ParseParam();
    if (param) {
      printf("found param in param_list_prime: %s\n", param->getName().c_str());
      param_list.push_back(std::move(param));
      auto param_list_prime = ParseParamListPrime();
      for (unsigned i = 0; i < param_list_prime.size(); i++) {
        param_list.push_back(std::move(param_list_prime.at(i)));
      }
    }
  } else if (CurTok.type == RPAR) { // FOLLOW(param_list_prime)
    // expand by param_list_prime ::= ε
    // do nothing
  } else {
    LogError(CurTok, "expected ',' or ')' in list of parameter declarations");
  }

  return param_list;
}

// param ::= var_type IDENT
static std::unique_ptr<ParamAST> ParseParam() {
  std::string Type = CurTok.lexeme; // keep track of the type of the param
  getNextToken();                   // eat the type token

  if (CurTok.type == IDENT) { // parameter declaration
    std::string Name = CurTok.getIdentifierStr();
    getNextToken(); // eat "IDENT"
    return std::make_unique<ParamAST>(Name, Type);  
  }

  return nullptr;
}

// param_list ::= param param_list_prime
static std::vector<std::unique_ptr<ParamAST>> ParseParamList() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  auto param = ParseParam();
  if (param) {
    param_list.push_back(std::move(param));
    auto param_list_prime = ParseParamListPrime();
    for (unsigned i = 0; i < param_list_prime.size(); i++) {
      param_list.push_back(std::move(param_list_prime.at(i)));
    }
  }

  return param_list;
}

// params ::= param_list
//         |  ε
static std::vector<std::unique_ptr<ParamAST>> ParseParams() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  std::string Type;
  std::string Name = "";

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(param_list)

    auto list = ParseParamList();
    for (unsigned i = 0; i < list.size(); i++) {
      param_list.push_back(std::move(list.at(i)));
    }

  } else if (CurTok.type == VOID_TOK) { // FIRST("void")
    // void
    // check that the next token is a )
    getNextToken(); // eat 'void'
    if (CurTok.type != RPAR) {
      LogError(CurTok, "expected ')', after 'void' in \
       end of function declaration");
    }
  } else if (CurTok.type == RPAR) { // FOLLOW(params)
    // expand by params ::= ε
    // do nothing
  } else {
    LogError(
        CurTok,
        "expected 'int', 'bool' or 'float' in function declaration or ') in \
       end of function declaration");
  }

  return param_list;
}

/*** TODO : Task 2 - Parser ***

// args ::= arg_list
//      |  ε
// arg_list ::= arg_list "," expr
//      | expr

// rval ::= rval "||" rval
//      | rval "&&" rval
//      | rval "==" rval | rval "!=" rval
//      | rval "<=" rval | rval "<" rval | rval ">=" rval | rval ">" rval
//      | rval "+" rval | rval "-" rval
//      | rval "*" rval | rval "/" rval | rval "%" rval
//      | "-" rval | "!" rval
//      | "(" expr ")"
//      | IDENT | IDENT "(" args ")"
//      | INT_LIT | FLOAT_LIT | BOOL_LIT
**/

// expr ::= IDENT "=" expr
//      |  rval



/*
NEW TRANSFORMED GRAMMAR
expr ::= assign_expr

assign_expr ::= or_expr assign_expr'
assign_expr' ::= "=" assign_expr | ε

or_expr ::= and_expr or_expr'
or_expr' ::= "||" and_expr or_expr' | ε

and_expr ::= eq_expr and_expr'
and_expr' ::= "&&" eq_expr and_expr' | ε

eq_expr ::= rel_expr eq_expr'
eq_expr' ::= ("==" | "!=") rel_expr eq_expr' | ε

rel_expr ::= add_expr rel_expr'
rel_expr' ::= ("<" | "<=" | ">" | ">=") add_expr rel_expr' | ε

add_expr ::= mul_expr add_expr'
add_expr' ::= ("+" | "-") mul_expr add_expr' | ε

mul_expr ::= unary_expr mul_expr'
mul_expr' ::= ("*" | "/" | "%") unary_expr mul_expr' | ε

unary_expr ::= ("-" | "!") unary_expr
            | postfix_expr

postfix_expr ::= primary_expr postfix_expr'
postfix_expr' ::= "(" args ")" | ε

primary_expr ::= IDENT
              | INT_LIT
              | FLOAT_LIT
              | BOOL_LIT
              | "(" expr ")"

args ::= arg_list | ε
arg_list ::= expr arg_list'
arg_list' ::= "," expr arg_list' | ε


-----FIRST SETS-----
FIRST(expr) = FIRST(assign_expr) = FIRST(or_expr) = ... = {IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, "(", "-", "!"}

FIRST(assign_expr') = {"=", ε}
FIRST(or_expr') = {"||", ε}
FIRST(and_expr') = {"&&", ε}
FIRST(eq_expr') = {"==", "!=", ε}
FIRST(rel_expr') = {"<", "<=", ">", ">=", ε}
FIRST(add_expr') = {"+", "-", ε}
FIRST(mul_expr') = {"*", "/", "%", ε}
FIRST(unary_expr) = {"-", "!", IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, "("}
FIRST(postfix_expr') = {"(", ε}
FIRST(primary_expr) = {IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, "("}
FIRST(args) = {IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, "(", "-", "!", ε}

-----FOLLOW SETS-----

*/

// Forward declarations for all expression parsing functions
static std::unique_ptr<ASTnode> ParseOrExpr();                      // '||'
static std::unique_ptr<ASTnode> ParseAndExpr();                     // '&&'
static std::unique_ptr<ASTnode> ParseEqExpr();                      // '==' '!='
static std::unique_ptr<ASTnode> ParseRelExpr();                     // '<' '<=' '>' '>='
static std::unique_ptr<ASTnode> ParseAddExpr();                     // '+' '-'
static std::unique_ptr<ASTnode> ParseMulExpr();                     //'*' '/' '%'
static std::unique_ptr<ASTnode> ParseUnaryExpr();                   // prefix '!' '-'
static std::unique_ptr<ASTnode> ParsePostfixExpr();                 // calls vs plain ident
static std::unique_ptr<ASTnode> ParsePrimaryExpr();

static std::unique_ptr<ASTnode> ParseExpr();                        // top-level alias
static std::unique_ptr<ASTnode> ParseAssignExpr() {                 // '=' (lowest precedence)
  auto lhs = ParseOrExpr(); // next level down in precedence    

  if (CurTok.type == ASSIGN) { // '=' token

    VariableASTnode* varNode = dynamic_cast<VariableASTnode*>(lhs.get());
    if (!varNode) {
      return LogError(CurTok, "left side of assignment must be a variable");
    }
    TOKEN Op = CurTok;
    getNextToken();
    auto rhs = ParseAssignExpr(); // right-associative
    // wrap in AssignExprAST node
    return std::make_unique<AssignExprAST>(
        std::unique_ptr<VariableASTnode>(
          static_cast<VariableASTnode*>(lhs.release())),
        std::move(rhs)
    );
  }

  return lhs;

}

// '||' - logical OR (left-assosciative)
static std::unique_ptr<ASTnode> ParseOrExpr(){
  auto lhs = ParseAndExpr(); // parse higher precedence first
  if (!lhs) return nullptr;

  while (CurTok.type == OR) {
    TOKEN Op = CurTok;
    getNextToken(); // eat '||'
    auto rhs = ParseAndExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '&&' - logical AND (left-assosciative)
static std::unique_ptr<ASTnode> ParseAndExpr() {
  auto lhs = ParseEqExpr();
  if (!lhs) return nullptr;

  while (CurTok.type == AND) {
    TOKEN Op = CurTok;
    getNextToken(); // eat '&&'
    auto rhs = ParseEqExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '==' '!=' - equality operators (left-assosciative)
static std::unique_ptr<ASTnode> ParseEqExpr() {
  auto lhs = ParseRelExpr();
  if (!lhs) return nullptr;

  while (CurTok.type == EQ || CurTok.type == NE) {
    TOKEN Op = CurTok;
    getNextToken(); // eat '==' or '!='
    auto rhs = ParseRelExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '<' '<=' '>' '>=' - relational operators (left-assosciative)
static std::unique_ptr<ASTnode> ParseRelExpr() {
  auto lhs = ParseAddExpr();
  if (!lhs) return nullptr;

  while (CurTok.type == LT || CurTok.type == LE || 
         CurTok.type == GT || CurTok.type == GE) {
    TOKEN Op = CurTok;
    getNextToken(); // eat relatoinal operator
    auto rhs = ParseAddExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '+' '-' - additive operators (left-assosciative)
static std::unique_ptr<ASTnode> ParseAddExpr() {
  auto lhs = ParseMulExpr();
  if(!lhs) return nullptr;

  while (CurTok.type == PLUS || CurTok.type == MINUS) {
    TOKEN Op = CurTok;
    getNextToken(); // eat + or - 
    auto rhs = ParseMulExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '*' '/' '%' - multiplicative operators (left-assosciative)
static std::unique_ptr<ASTnode> ParseMulExpr() {
  auto lhs = ParseUnaryExpr();
  if (!lhs) return nullptr;
  
  while (CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
    TOKEN Op = CurTok;
    getNextToken(); // eat
    auto rhs = ParseUnaryExpr();
    if (!rhs) return nullptr;
    lhs = std::make_unique<ExprAST>(Op, std::move(lhs), std::move(rhs));
  }
  return lhs;
}

// '!' '-' - unary operators (prefix)
static std::unique_ptr<ASTnode> ParseUnaryExpr() {
  // Check for unary operators
  if (CurTok.type == NOT || CurTok.type == MINUS) {
    TOKEN Op = CurTok;
    getNextToken(); // eat unary operator
    auto operand = ParseUnaryExpr(); // right-assosciative (can stack: --x, !!x)
    if (!operand) return nullptr;
    return std::make_unique<ExprAST>(Op, std::move(operand));
  }
  // No unary operator, parse postfix expression
  return ParsePostfixExpr();
}

// Function calls: IDENT '(' args ')' or just IDENT
static std::unique_ptr<ASTnode> ParsePostfixExpr() {      // calls vs plain ident
  auto expr = ParsePrimaryExpr();
  if (!expr) return nullptr;

  // Check if this is a function call
  if (CurTok.type == LPAR) {
    getNextToken(); // eat '('
    // Parse arguments
    std::vector<std::unique_ptr<ASTnode>> args;

    // args ::= arg_list | eps
    if (CurTok.type != RPAR) {      // not empty args
      // arg_list ::= expr ("," expr)*
      while (true) {
        auto arg = ParseAssignExpr(); // parse each argument as an expression
        if (!arg) return nullptr;
        args.push_back(std::move(arg));

        if (CurTok.type != COMMA) break; // no more arguments
        getNextToken(); // eat ','
      }
    }
    
    if (CurTok.type != RPAR) {
      return LogError(CurTok, "expected ')' in function call");
    }
    getNextToken(); // eat')'
    
    // Get function name from the primary expression (should be an identifier)
    VariableASTnode* varNode = dynamic_cast<VariableASTnode*>(expr.get());
    if (!varNode) {
      return LogError(CurTok, "function name expected before '('");
    }
    std::string callee = varNode->getName();

    return std::make_unique<ArgsAST>(callee, std::move(args));
  }
  
  return expr;
}

// Primary expressions: literals, identifiers, parenthesized expressions
static std::unique_ptr<ASTnode> ParsePrimaryExpr() {
  switch (CurTok.type) {
    case IDENT: {
      // Variable reference
      auto result = std::make_unique<VariableASTnode>(CurTok, CurTok.getIdentifierStr());
      getNextToken(); // eat identifier
      return result;
    }
    
    case INT_LIT:
      return ParseIntNumberExpr();
    
    case FLOAT_LIT:
      return ParseFloatNumberExpr();
    
    case BOOL_LIT:
      return ParseBoolExpr();
    
    case LPAR: {      // "(" expr ")"
      getNextToken(); // eat '('
      auto expr = ParseAssignExpr(); // parse full expression (lowest precedence)
      if (!expr) return nullptr;
      
      if (CurTok.type != RPAR) {
        return LogError(CurTok, "expected ')'");
      }
      getNextToken(); // eat ')'
      return expr;
    }
    
    default:
      return LogError(CurTok, "unexpected token in expression");
  }
}

static std::unique_ptr<ASTnode> ParseExper() {
  //
  // TO BE COMPLETED
  //
  return ParseAssignExpr();
}


// expr_stmt ::= expr ";"
//            |  ";"
static std::unique_ptr<ASTnode> ParseExperStmt() {

  if (CurTok.type == SC) { // empty statement
    getNextToken();        // eat ;
    return nullptr;
  } else {
    auto expr = ParseExper();
    if (expr) {
      if (CurTok.type == SC) {
        getNextToken(); // eat ;
        return expr;
      } else {
        LogError(CurTok, "expected ';' to end expression statement");
      }
    } else
      return nullptr;
  }
  return nullptr;
}

// else_stmt  ::= "else" block
//             |  ε
static std::unique_ptr<ASTnode> ParseElseStmt() {

  if (CurTok.type == ELSE) { // FIRST(else_stmt)
    // expand by else_stmt  ::= "else" "{" stmt "}"
    getNextToken(); // eat "else"

    if (!(CurTok.type == LBRA)) {
      return LogError(
          CurTok, "expected { to start else block of if-then-else statment");
    }
    auto Else = ParseBlock();
    if (!Else)
      return nullptr;
    return Else;
  } else if (CurTok.type == NOT || CurTok.type == MINUS ||
             CurTok.type == PLUS || CurTok.type == LPAR ||
             CurTok.type == IDENT || CurTok.type == INT_LIT ||
             CurTok.type == BOOL_LIT || CurTok.type == FLOAT_LIT ||
             CurTok.type == SC || CurTok.type == LBRA || CurTok.type == WHILE ||
             CurTok.type == IF || CurTok.type == ELSE ||
             CurTok.type == RETURN ||
             CurTok.type == RBRA) { // FOLLOW(else_stmt)
    // expand by else_stmt  ::= ε
    // return an empty statement
    return nullptr;
  } else
    LogError(CurTok, "expected 'else' or one of \
    '!', '-', '+', '(' , IDENT , INT_LIT, BOOL_LIT, FLOAT_LIT, ';', \
    '{', 'while', 'if', 'else', ε, 'return', '}' ");

  return nullptr;
}

// if_stmt ::= "if" "(" expr ")" block else_stmt
static std::unique_ptr<ASTnode> ParseIfStmt() {
  getNextToken(); // eat the if.
  if (CurTok.type == LPAR) {
    getNextToken(); // eat (
    // condition.
    auto Cond = ParseExper();
    if (!Cond)
      return nullptr;
    if (CurTok.type != RPAR)
      return LogError(CurTok, "expected )");
    getNextToken(); // eat )

    if (!(CurTok.type == LBRA)) {
      return LogError(CurTok, "expected { to start then block of if statment");
    }

    auto Then = ParseBlock();
    if (!Then)
      return nullptr;
    auto Else = ParseElseStmt();

    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                       std::move(Else));

  } else
    return LogError(CurTok, "expected (");

  return nullptr;
}

// return_stmt ::= "return" ";"
//             |  "return" expr ";"
static std::unique_ptr<ASTnode> ParseReturnStmt() {
  getNextToken(); // eat the return
  if (CurTok.type == SC) {
    getNextToken(); // eat the ;
    // return a null value
    return std::make_unique<ReturnAST>(std::move(nullptr));
  } else if (CurTok.type == NOT || CurTok.type == MINUS ||
             CurTok.type == PLUS || CurTok.type == LPAR ||
             CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
             CurTok.type == INT_LIT ||
             CurTok.type == FLOAT_LIT) { // FIRST(expr)
    auto val = ParseExper();
    if (!val)
      return nullptr;

    if (CurTok.type == SC) {
      getNextToken(); // eat the ;
      return std::make_unique<ReturnAST>(std::move(val));
    } else
      return LogError(CurTok, "expected ';'");
  } else
    return LogError(CurTok, "expected ';' or expression");

  return nullptr;
}

// while_stmt ::= "while" "(" expr ")" stmt
static std::unique_ptr<ASTnode> ParseWhileStmt() {

  getNextToken(); // eat the while.
  if (CurTok.type == LPAR) {
    getNextToken(); // eat (
    // condition.
    auto Cond = ParseExper();
    if (!Cond)
      return nullptr;
    if (CurTok.type != RPAR)
      return LogError(CurTok, "expected )");
    getNextToken(); // eat )

    auto Body = ParseStmt();
    if (!Body)
      return nullptr;

    return std::make_unique<WhileExprAST>(std::move(Cond), std::move(Body));
  } else
    return LogError(CurTok, "expected (");
}

// stmt ::= expr_stmt
//      |  block
//      |  if_stmt
//      |  while_stmt
//      |  return_stmt
static std::unique_ptr<ASTnode> ParseStmt() {

  if (CurTok.type == NOT || CurTok.type == MINUS || CurTok.type == PLUS ||
      CurTok.type == LPAR || CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
      CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT ||
      CurTok.type == SC) { // FIRST(expr_stmt)
    // expand by stmt ::= expr_stmt
    auto expr_stmt = ParseExperStmt();
    fprintf(stderr, "Parsed an expression statement\n");
    return expr_stmt;
  } else if (CurTok.type == LBRA) { // FIRST(block)
    auto block_stmt = ParseBlock();
    if (block_stmt) {
      fprintf(stderr, "Parsed a block\n");
      return block_stmt;
    }
  } else if (CurTok.type == IF) { // FIRST(if_stmt)
    auto if_stmt = ParseIfStmt();
    if (if_stmt) {
      fprintf(stderr, "Parsed an if statment\n");
      return if_stmt;
    }
  } else if (CurTok.type == WHILE) { // FIRST(while_stmt)
    auto while_stmt = ParseWhileStmt();
    if (while_stmt) {
      fprintf(stderr, "Parsed a while statment\n");
      return while_stmt;
    }
  } else if (CurTok.type == RETURN) { // FIRST(return_stmt)
    auto return_stmt = ParseReturnStmt();
    if (return_stmt) {
      fprintf(stderr, "Parsed a return statment\n");
      return return_stmt;
    }
  }
  // else if(CurTok.type == RBRA) { // FOLLOW(stmt_list_prime)
  //  expand by stmt_list_prime ::= ε
  //  do nothing
  //}
  else { // syntax error
    return LogError(CurTok, "expected BLA BLA\n");
  }
  return nullptr;
}

// stmt_list ::= stmt stmt_list_prime
static std::vector<std::unique_ptr<ASTnode>> ParseStmtList() {
  std::vector<std::unique_ptr<ASTnode>> stmt_list; // vector of statements
  auto stmt = ParseStmt();
  if (stmt) {
    stmt_list.push_back(std::move(stmt));
  }
  auto stmt_list_prime = ParseStmtListPrime();
  for (unsigned i = 0; i < stmt_list_prime.size(); i++) {
    stmt_list.push_back(std::move(stmt_list_prime.at(i)));
  }
  return stmt_list;
}

// stmt_list_prime ::= stmt stmt_list_prime
//                  |  ε
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime() {
  std::vector<std::unique_ptr<ASTnode>> stmt_list; // vector of statements
  if (CurTok.type == NOT || CurTok.type == MINUS || CurTok.type == PLUS ||
      CurTok.type == LPAR || CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
      CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == SC ||
      CurTok.type == LBRA || CurTok.type == WHILE || CurTok.type == IF ||
      CurTok.type == ELSE || CurTok.type == RETURN) { // FIRST(stmt)
    // expand by stmt_list ::= stmt stmt_list_prime
    auto stmt = ParseStmt();
    if (stmt) {
      stmt_list.push_back(std::move(stmt));
    }
    auto stmt_prime = ParseStmtListPrime();
    for (unsigned i = 0; i < stmt_prime.size(); i++) {
      stmt_list.push_back(std::move(stmt_prime.at(i)));
    }

  } else if (CurTok.type == RBRA) { // FOLLOW(stmt_list_prime)
    // expand by stmt_list_prime ::= ε
    // do nothing
  }
  return stmt_list; // note stmt_list can be empty as we can have empty blocks,
                    // etc.
}

// local_decls_prime ::= local_decl local_decls_prime
//                    |  ε
static std::vector<std::unique_ptr<VarDeclAST>> ParseLocalDeclsPrime() {
  std::vector<std::unique_ptr<VarDeclAST>>
      local_decls_prime; // vector of local decls

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(local_decl)
    auto local_decl = ParseLocalDecl();
    if (local_decl) {
      local_decls_prime.push_back(std::move(local_decl));
    }
    auto prime = ParseLocalDeclsPrime();
    for (unsigned i = 0; i < prime.size(); i++) {
      local_decls_prime.push_back(std::move(prime.at(i)));
    }
  } else if (CurTok.type == MINUS || CurTok.type == NOT ||
             CurTok.type == LPAR || CurTok.type == IDENT ||
             CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT ||
             CurTok.type == BOOL_LIT || CurTok.type == SC ||
             CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE ||
             CurTok.type == RETURN) { // FOLLOW(local_decls_prime)
    // expand by local_decls_prime ::=  ε
    // do nothing;
  } else {
    LogError(
        CurTok,
        "expected '-', '!', ('' , IDENT , STRING_LIT , INT_LIT , FLOAT_LIT, \
      BOOL_LIT, ';', '{', 'if', 'while', 'return' after local variable declaration\n");
  }

  return local_decls_prime;
}

// local_decl ::= var_type IDENT ";"
// var_type ::= "int"
//           |  "float"
//           |  "bool"
static std::unique_ptr<VarDeclAST> ParseLocalDecl() {
  TOKEN PrevTok;
  std::string Type;
  std::string Name = "";
  std::unique_ptr<VarDeclAST> local_decl;

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(var_type)
    PrevTok = CurTok;
    getNextToken(); // eat 'int' or 'float or 'bool'
    if (CurTok.type == IDENT) {
      Type = PrevTok.lexeme;
      Name = CurTok.getIdentifierStr(); // save the identifier name
      auto ident = std::make_unique<VariableASTnode>(CurTok, Name);
      local_decl = std::make_unique<VarDeclAST>(std::move(ident), Type);

      getNextToken(); // eat 'IDENT'
      if (CurTok.type != SC) {
        LogError(CurTok, "Expected ';' to end local variable declaration");
        return nullptr;
      }
      getNextToken(); // eat ';'
      fprintf(stderr, "Parsed a local variable declaration\n");
    } else {
      LogError(CurTok, "expected identifier' in local variable declaration");
      return nullptr;
    }
  }
  return local_decl;
}

// local_decls ::= local_decl local_decls_prime
static std::vector<std::unique_ptr<VarDeclAST>> ParseLocalDecls() {
  std::vector<std::unique_ptr<VarDeclAST>> local_decls; // vector of local decls

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(local_decl)

    auto local_decl = ParseLocalDecl();
    if (local_decl) {
      local_decls.push_back(std::move(local_decl));
    }
    auto local_decls_prime = ParseLocalDeclsPrime();
    for (unsigned i = 0; i < local_decls_prime.size(); i++) {
      local_decls.push_back(std::move(local_decls_prime.at(i)));
    }

  } else if (CurTok.type == MINUS || CurTok.type == NOT ||
             CurTok.type == LPAR || CurTok.type == IDENT ||
             CurTok.type == INT_LIT || CurTok.type == RETURN ||
             CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT ||
             CurTok.type == COMMA || CurTok.type == LBRA || CurTok.type == IF ||
             CurTok.type == WHILE) { // FOLLOW(local_decls)
                                     // do nothing
  } else {
    LogError(
        CurTok,
        "expected '-', '!', '(' , IDENT , STRING_LIT , INT_LIT , FLOAT_LIT, \
        BOOL_LIT, ';', '{', 'if', 'while', 'return'");
  }

  return local_decls;
}

// parse block
// block ::= "{" local_decls stmt_list "}"
static std::unique_ptr<ASTnode> ParseBlock() {
  std::vector<std::unique_ptr<VarDeclAST>> local_decls; // vector of local decls
  std::vector<std::unique_ptr<ASTnode>> stmt_list;      // vector of statements

  getNextToken(); // eat '{'

  local_decls = ParseLocalDecls();
  fprintf(stderr, "Parsed a set of local variable declaration\n");
  stmt_list = ParseStmtList();
  fprintf(stderr, "Parsed a list of statements\n");
  if (CurTok.type == RBRA)
    getNextToken(); // eat '}'
  else {            // syntax error
    LogError(CurTok, "expected '}' , close body of block");
    return nullptr;
  }

  return std::make_unique<BlockAST>(std::move(local_decls),
                                    std::move(stmt_list));
}

// decl ::= var_decl
//       |  fun_decl
static std::unique_ptr<ASTnode> ParseDecl() {
  std::string IdName;
  std::vector<std::unique_ptr<ParamAST>> param_list;

  TOKEN PrevTok = CurTok; // to keep track of the type token

  if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
      CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    getNextToken(); // eat the VOID_TOK, INT_TOK, BOOL_TOK or FLOAT_TOK

    IdName = CurTok.getIdentifierStr(); // save the identifier name

    if (CurTok.type == IDENT) {
      auto ident = std::make_unique<VariableASTnode>(CurTok, IdName);
      getNextToken(); // eat the IDENT
      if (CurTok.type ==
          SC) {         // found ';' then this is a global variable declaration.
        getNextToken(); // eat ;
        fprintf(stderr, "Parsed a variable declaration\n");

        if (PrevTok.type != VOID_TOK){
          auto globVar = std::make_unique<GlobVarDeclAST>(std::move(ident), PrevTok.lexeme);
          // fprintf(stderr, "AST: %s\n", globVar->to_string().c_str());
          return globVar;
        } else
          return LogError(PrevTok,
                          "Cannot have variable declaration with type 'void'");
      } else if (CurTok.type ==
                 LPAR) { // found '(' then this is a function declaration.
        getNextToken();  // eat (

        auto P =
            ParseParams(); // parse the parameters, returns a vector of params
        // if (P.size() == 0) return nullptr;
        fprintf(stderr, "Parsed parameter list for function\n");

        if (CurTok.type != RPAR) // syntax error
          return LogError(CurTok, "expected ')' in function declaration");

        getNextToken();          // eat )
        if (CurTok.type != LBRA) // syntax error
          return LogError(
              CurTok, "expected '{' in function declaration, function body");

        auto B = ParseBlock(); // parse the function body
        if (!B)
          return nullptr;
        else
          fprintf(stderr, "Parsed block of statements in function\n");

        // now create a Function prototype
        // create a Function body
        // put these to together
        // and return a std::unique_ptr<FunctionDeclAST>
        fprintf(stderr, "Parsed a function declaration\n");

        auto Proto = std::make_unique<FunctionPrototypeAST>(
            IdName, PrevTok.lexeme, std::move(P));
        auto funcDecl = std::make_unique<FunctionDeclAST>(std::move(Proto), std::move(B));
        // fprintf(stderr, "AST: %s\n", funcDecl->to_string().c_str());
        return funcDecl;
      } else
        return LogError(CurTok, "expected ';' or ('");
    } else
      return LogError(CurTok, "expected an identifier");

  } else
    LogError(CurTok,
             "expected 'void', 'int' or 'float' or EOF token"); // syntax error

  return nullptr;
}

// decl_list_prime ::= decl decl_list_prime
//                  |  ε
static void ParseDeclListPrime() {
  if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
      CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) { // FIRST(decl)

    if (auto decl = ParseDecl()) {
      ProgramAST.push_back(std::move(decl));
      fprintf(stderr, "Parsed a top-level variable or function declaration\n");
      // fprintf(stderr, "AST: %s\n", decl->to_string().c_str());
      // TopLevelDecls.push_back(std::move(decl));
    }
    ParseDeclListPrime();
  } else if (CurTok.type == EOF_TOK) { // FOLLOW(decl_list_prime)
    // expand by decl_list_prime ::= ε
    // do nothing
  } else { // syntax error
    LogError(CurTok, "expected 'void', 'int', 'bool' or 'float' or EOF token");
  }
}

// decl_list ::= decl decl_list_prime
static void ParseDeclList() {
  auto decl = ParseDecl();
  if (decl) {
    ProgramAST.push_back(std::move(decl));
    fprintf(stderr, "Parsed a top-level variable or function declaration\n");
    // TopLevelDecls.push_back(std::move(decl));
    ParseDeclListPrime();
  }
}

// extern ::= "extern" type_spec IDENT "(" params ")" ";"
static std::unique_ptr<FunctionPrototypeAST> ParseExtern() {
  std::string IdName;
  TOKEN PrevTok;

  if (CurTok.type == EXTERN) {
    getNextToken(); // eat the EXTERN

    if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
        CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {

      PrevTok = CurTok; // to keep track of the type token
      getNextToken();   // eat the VOID_TOK, INT_TOK, BOOL_TOK or FLOAT_TOK

      if (CurTok.type == IDENT) {
        IdName = CurTok.getIdentifierStr(); // save the identifier name
        auto ident = std::make_unique<VariableASTnode>(CurTok, IdName);
        getNextToken(); // eat the IDENT

        if (CurTok.type ==
            LPAR) {       // found '(' - this is an extern function declaration.
          getNextToken(); // eat (

          auto P =
              ParseParams(); // parse the parameters, returns a vector of params
          if (P.size() == 0)
            return nullptr;
          else
            fprintf(stderr, "Parsed parameter list for external function\n");

          if (CurTok.type != RPAR) // syntax error
            return LogErrorP(
                CurTok, "expected ')' in closing extern function declaration");

          getNextToken(); // eat )

          if (CurTok.type == SC) {
            getNextToken(); // eat ";"
            auto Proto = std::make_unique<FunctionPrototypeAST>(
                IdName, PrevTok.lexeme, std::move(P));
            // fprintf(stderr, "AST: %s\n", Proto->to_string().c_str());
            return Proto;
          } else
            return LogErrorP(
                CurTok,
                "expected ;' in ending extern function declaration statement");
        } else
          return LogErrorP(CurTok,
                           "expected (' in extern function declaration");
      }

    } else
      LogErrorP(CurTok, "expected 'void', 'int' or 'float' in extern function "
                        "declaration\n"); // syntax error
  }

  return nullptr;
}

// extern_list_prime ::= extern extern_list_prime
//                   |  ε
static void ParseExternListPrime() {

  if (CurTok.type == EXTERN) { // FIRST(extern)
    if (auto Extern = ParseExtern()) {
      ExternAST.push_back(std::move(Extern));
      fprintf(stderr,
              "Parsed a top-level external function declaration -- 2\n");
      // ExternDecls.push_back(std::move(Extern)); // store extern in global AST
    }
    ParseExternListPrime();
  } else if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
             CurTok.type == FLOAT_TOK ||
             CurTok.type == BOOL_TOK) { // FOLLOW(extern_list_prime)
    // expand by decl_list_prime ::= ε
    // do nothing
  } else { // syntax error
    LogError(CurTok, "expected 'extern' or 'void',  'int' ,  'float',  'bool'");
  }
}

// extern_list ::= extern extern_list_prime
static void ParseExternList() {
  auto Extern = ParseExtern();
  if (Extern) {
    // fprintf(stderr, "Parsed a top-level external function declaration -- 1\n");
    // ExternDecls.push_back(std::move(Extern)); // Store extern in global AST
    ExternAST.push_back(std::move(Extern));
    fprintf(stderr, "Parsed a top-level external function declaration -- 1\n");
    // fprintf(stderr, "Current token: %s \n", CurTok.lexeme.c_str());
    if (CurTok.type == EXTERN)
      ParseExternListPrime();
  }
}

// program ::= extern_list decl_list
static void parser() {
  if (CurTok.type == EOF_TOK)
    return;
  ParseExternList();
  if (CurTok.type == EOF_TOK)
    return;
  ParseDeclList();
  if (CurTok.type == EOF_TOK)
    return;
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

// Function to print the complete AST
static void PrintAST() {
  fprintf(stderr, "\n");
  fprintf(stderr, "==================== COMPLETE AST ====================\n");
  fprintf(stderr, "\n");
  
  // Print extern declarations
  if (!ExternDecls.empty()) {
    fprintf(stderr, "=== Extern Declarations ===\n");
    for (const auto& externDecl : ExternDecls) {
      fprintf(stderr, "%s\n\n", externDecl->to_string().c_str());
    }
  }
  
  // Print top-level declarations
  if (!TopLevelDecls.empty()) {
    fprintf(stderr, "=== Top-Level Declarations ===\n");
    for (const auto& decl : TopLevelDecls) {
      fprintf(stderr, "%s\n\n", decl->to_string().c_str());
    }
  }
  
  fprintf(stderr, "======================================================\n");
  fprintf(stderr, "\n");
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);

  // get the first token
  getNextToken();

  // Run the parser now - Do not consume tokens before this
  fprintf(stderr, "Starting parser...\n");
  parser();
  fprintf(stderr, "Parsing Finished\n");

  fprintf(stderr, "\n=== Abstract Syntax Tree ===\n");
  for (const auto& ext : ExternAST) {
    fprintf(stderr, "AST: %s\n", ext->to_string().c_str());
  }
  for (const auto& node : ProgramAST) {
    fprintf(stderr, "AST: %s\n", node->to_string().c_str());
  }
  fprintf(stderr, "=== End of AST ===\n\n");

  // Print the complete AST after successful parse
  // PrintAST();

  printf(
      "********************* FINAL IR (begin) ****************************\n");

  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }

  TheModule->print(dest, nullptr);
  printf("********************* FINAL IR (end) ******************************\n");

  fclose(pFile);
  return 0;
  /*
  while (CurTok.type != EOF_TOK) {
    fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
            CurTok.type);
    getNextToken();
  }
  fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);
*/

  // Run the parser now.

  /* UNCOMMENT : Task 2 - Parser */
  /*
  parser();
  fprintf(stderr, "Parsing Finished\n");  

  printf(
      "********************* FINAL IR (begin) ****************************\n");
  // Print out all of the generated code into a file called output.ll
  // printf("%s\n", argv[1]);
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  printf(
      "********************* FINAL IR (end) ******************************\n");

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
*/
  }
