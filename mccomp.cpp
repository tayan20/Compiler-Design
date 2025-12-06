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

// Helper function to create tree-style indentation
static std::string indent_str(int level, bool isLast = false) {
  if (level == 0) return "";
  std::string result = "";
  for (int i = 0; i < level - 1; i++) {
    result += "|  ";
  }
  result += isLast ? "`--" : "|--";
  return result;
};

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

static std::map<std::string, AllocaInst*> NamedValues;        // Local variables
static std::map<std::string, GlobalVariable*> GlobalNamedValues; // Global variables
static std::map<std::string, Function*> FunctionTable;        // Function declarations
static Function* CurrentFunction = nullptr;                    // Track current function being compiled

// PART 3 ADDITION
// Store array metadata: name -> {element type, dimensions}
struct ArrayInfo {
  std::string elementType;
  std::vector<int> dimensions;
};
static std::map<std::string, ArrayInfo> LocalArrayInfo; // Local array metadata
static std::map<std::string, ArrayInfo> GlobalArrayInfo; // Global array metadata
static std::map<std::string, ArrayInfo> ParamArrayInfo; // Array parameters

// Get LLVM type from string type name
static Type* getLLVMType(const std::string& typeName) {
  if (typeName == "int") return Type::getInt32Ty(TheContext);
  if (typeName == "float") return Type::getFloatTy(TheContext);
  if (typeName == "bool") return Type::getInt1Ty(TheContext);
  if (typeName == "void") return Type::getVoidTy(TheContext);
  return nullptr;
}

// Create alloca instruction in entry block
static AllocaInst* CreateEntryBlockAlloca(Function* TheFunction, 
                                          const std::string& VarName,
                                          Type* type) {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                     TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, nullptr, VarName);
}

// PART 3 ADDITION
// Create LLVM array type from element type and dimensions
static llvm::Type* createArrayType(llvm::Type* elementType, const std::vector<int>& dims) {
  llvm::Type* result = elementType;
  // Build from innermost to outermost dimension
  for (int i = dims.size() - 1; i>= 0; i--) {
    result = ArrayType::get(result, dims[i]);
  }
  return result;
}

// Type promotion helper
static Value* promoteType(Value* V, Type* targetType) {
    Type* srcType = V->getType();
    if (srcType == targetType) return V;
    
    // int to float (widening)
    if (srcType->isIntegerTy(32) && targetType->isFloatTy()) {
        return Builder.CreateSIToFP(V, targetType, "intToFloat");
    }
    // bool to int
    if (srcType->isIntegerTy(1) && targetType->isIntegerTy(32)) {
        return Builder.CreateZExt(V, targetType, "boolToInt");
    }
    // bool to float
    if (srcType->isIntegerTy(1) && targetType->isFloatTy()) {
        Value* intVal = Builder.CreateZExt(V, Type::getInt32Ty(TheContext), "boolToInt");
        return Builder.CreateSIToFP(intVal, targetType, "intToFloat");
    }
    // int to bool (for conditions)
    if (srcType->isIntegerTy(32) && targetType->isIntegerTy(1)) {
        return Builder.CreateICmpNE(V, ConstantInt::get(srcType, 0), "intToBool");
    }
    // float to bool
    if (srcType->isFloatTy() && targetType->isIntegerTy(1)) {
        return Builder.CreateFCmpUNE(V, ConstantFP::get(srcType, 0.0), "floatToBool");
    }
    return V;
}

// Error reporting for codegen
static Value* LogErrorV(const char* Str) {
    fprintf(stderr, "Code generation ERROR: %s\n", Str);
    exit(2);
    return nullptr;
}

/// ASTnode - Base class for all AST nodes.
class ASTnode {

public:
  virtual ~ASTnode() {}
  virtual Value *codegen() { return nullptr; };
  virtual std::string to_string(int indent = 0, bool isLast = false) const { return ""; };
};

/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public ASTnode {
  int Val;
  TOKEN Tok;
  // std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "IntLiteral(" + std::to_string(Val) + ")";
  }

  virtual Value* codegen() override {
    return ConstantInt::get(TheContext, APInt(32, Val, true));
  }
};

/// BoolASTnode - Class for boolean literals true and false,
class BoolASTnode : public ASTnode {
  bool Bool;
  TOKEN Tok;

public:
  BoolASTnode(TOKEN tok, bool B) : Bool(B), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "BoolLiteral(" + std::string(Bool ? "true" : "false") + ")";
  }

  virtual Value* codegen() override {
    return ConstantInt::get(TheContext, APInt(1, Bool ? 1 : 0));
  }
};

/// FloatASTnode - Node class for floating point literals like "1.0".
class FloatASTnode : public ASTnode {
  double Val;
  TOKEN Tok;

public:
  FloatASTnode(TOKEN tok, double Val) : Val(Val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "FloatLiteral(" + std::to_string(Val) + ")";
  }

  virtual Value* codegen() override {
    return ConstantFP::get(TheContext, APFloat((float)Val));
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

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "Variable(" + Name + ")";
  }

  virtual Value* codegen() override {
    // Look up variable in local scope first
    AllocaInst* A = NamedValues[Name];
    if (A) {
      return Builder.CreateLoad(A->getAllocatedType(), A, Name.c_str());
    }

    // Check global scope
    GlobalVariable* G = GlobalNamedValues[Name];
    if (G) {
      return Builder.CreateLoad(G->getValueType(), G, Name.c_str());
    }

    return LogErrorV(("Unknown variable name: " + Name).c_str());
  }
};

// PART 3 ADDITION
// ArrayAccessAST - Class for accessing array elements like arr[i] or arr[i][j]
class ArrayAccessAST : public ASTnode {
  std::string Name;
  std::vector<std::unique_ptr<ASTnode>> Indices; // 1, 2 or 3 index expressions

public:
  ArrayAccessAST(const std::string& name, std::vector<std::unique_ptr<ASTnode>> indices)
      : Name(name), Indices(std::move(indices)) {}

  const std::string& getName() const { return Name; }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "ArrayAccess(" + Name + ")";
    result += "\n" + indent_str(indent + 1, true) + "Indices:";
    for (size_t i = 0; i < Indices.size(); i++) {
      bool lastIdx = (i == Indices.size() - 1);
      result += "\n" + Indices[i]->to_string(indent + 2, lastIdx);
    }
    return result;
  }

  // Returns the pointer to the element (for use in assignment LHS)
  Value* codegenPtr() {
    // Check if this is an array parameter (passed as pointer)
    if (ParamArrayInfo.count(Name)) {
      AllocaInst* PtrAlloca = NamedValues[Name];
      if (!PtrAlloca) return LogErrorV(("Unknown array parameter: " + Name).c_str());

      ArrayInfo& info = ParamArrayInfo[Name];

      // Load the pointer value
      Value* Ptr = Builder.CreateLoad(PtrAlloca->getAllocatedType(), PtrAlloca, Name + "_ptr");

      // Generate index values
      std::vector<Value*> idxList;
      for (auto& idx: Indices) {
        Value* idxVal = idx->codegen();
        if (!idxVal) return nullptr;
        if (!idxVal->getType()->isIntegerTy(32)) {
          idxVal = Builder.CreateIntCast(idxVal, Type::getInt32Ty(TheContext), true, "idx_cast");
        }
        idxList.push_back(idxVal);
      }

      //For 1D array parameter: use the single index
      // For multi-dimensional: calcualte offset
      if (idxList.size() == 1) {
        return Builder.CreateGEP(getLLVMType(info.elementType), Ptr, idxList[0], "arrayidx");
      } else {
        // Multi-dimensional array parameter - calculate linear offset
        // For int a[M][N], accessing a[i][j] = a + i*N + j
        Value* offset = idxList[0];
        for (size_t i = 1; i < idxList.size(); i++) {
          Value* dimSize = ConstantInt::get(Type::getInt32Ty(TheContext), info.dimensions[i]);
          offset = Builder.CreateMul(offset, dimSize, "offset_mul");
          offset = Builder.CreateAdd(offset, idxList[i], "offset_add");
        }
        return Builder.CreateGEP(getLLVMType(info.elementType), Ptr, offset, "arrayidx");
      }
    }
    // Look up in local arrays first
    AllocaInst* LocalArr = NamedValues[Name];
    if (LocalArr && LocalArrayInfo.count(Name)) {
      ArrayInfo& info = LocalArrayInfo[Name];
      
      std::vector<Value*> idxList;
      idxList.push_back(ConstantInt::get(TheContext, APInt(32, 0)));
      
      for (auto& idx : Indices) {
        Value* idxVal = idx->codegen();
        if (!idxVal) return nullptr;
        if (!idxVal->getType()->isIntegerTy(32)) {
          idxVal = Builder.CreateIntCast(idxVal, Type::getInt32Ty(TheContext), true, "idx_cast");
        }
        idxList.push_back(idxVal);
      }
      
      llvm::Type* arrayType = createArrayType(getLLVMType(info.elementType), info.dimensions);
      return Builder.CreateGEP(arrayType, LocalArr, idxList, "arrayidx");
    }
    GlobalVariable* GlobalArr = GlobalNamedValues[Name];
    if (GlobalArr && GlobalArrayInfo.count(Name)) {
      ArrayInfo& info = GlobalArrayInfo[Name];
      
      std::vector<Value*> idxList;
      idxList.push_back(ConstantInt::get(TheContext, APInt(32, 0)));
      
      for (auto& idx : Indices) {
        Value* idxVal = idx->codegen();
        if (!idxVal) return nullptr;
        if (!idxVal->getType()->isIntegerTy(32)) {
          idxVal = Builder.CreateIntCast(idxVal, Type::getInt32Ty(TheContext), true, "idx_cast");
        }
        idxList.push_back(idxVal);
      }
      
      llvm::Type* arrayType = createArrayType(getLLVMType(info.elementType), info.dimensions);
      return Builder.CreateGEP(arrayType, GlobalArr, idxList, "arrayidx");
    }
  }

  virtual Value* codegen() override {
    Value* elemPtr = codegenPtr();
    if (!elemPtr) return nullptr;

    // Determine element type
    // ArrayInfo* info = nullptr;
    std::string elemType = "int"; // default
    /*
    if (LocalArrayInfo.count(Name)) {
      info = &LocalArrayInfo[Name];
    } else if (GlobalArrayInfo.count(Name)) {
      info = &GlobalArrayInfo[Name];
    }
    */
    if (ParamArrayInfo.count(Name)) {
      elemType = ParamArrayInfo[Name].elementType;
    } else if (LocalArrayInfo.count(Name)) {
      elemType = LocalArrayInfo[Name].elementType;
    } else if (GlobalArrayInfo.count(Name)) {
      elemType = GlobalArrayInfo[Name].elementType;
    }
    // llvm::Type* elemType = getLLVMType(info->elementType);
    return Builder.CreateLoad(getLLVMType(elemType), elemPtr, Name + "_elem");
  }
};

/// ParamAST - Class for a parameter declaration
class ParamAST {
  std::string Name;
  std::string Type;
  std::vector<int> ArrayDims; // Empty for non-array, filled for array params
  bool IsArray;

public:
  ParamAST(const std::string &name, const std::string &type)
      : Name(name), Type(type), IsArray(false) {}

  ParamAST(const std::string &name, const std::string &type, std::vector<int> dims)
      : Name(name), Type(type), ArrayDims(std::move(dims)), IsArray(true) {}

  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Type; }
  bool isArray() const { return IsArray; }
  const std::vector<int>& getDims() const {return ArrayDims;}

  std::string to_string(int indent = 0, bool isLast = false) const {
    std::string result = indent_str(indent, isLast) + "Param(" + Type + " " + Name;
    if (IsArray) {
      for (int dim : ArrayDims) {
        result += "[" + std::to_string(dim) + "]";
      }
    }
    result += ")";
    return result;
  }
};

/// DeclAST - Base class for declarations, variables and functions
class DeclAST : public ASTnode {

public:
  virtual ~DeclAST() {}
  virtual const std::string& getName() const = 0;
};

/// VarDeclAST - Class for a variable declaration
class VarDeclAST : public DeclAST {
  std::unique_ptr<VariableASTnode> Var;
  std::string Type;

public:
  VarDeclAST(std::unique_ptr<VariableASTnode> var, const std::string &type)
      : Var(std::move(var)), Type(type) {}
  const std::string &getType() const { return Type; }
  const std::string &getName() const override { return Var->getName(); }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "LocalVarDecl(" + Type + " " + Var->getName() + ")";
  }

  virtual Value* codegen() override {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    llvm::Type* VarType = getLLVMType(Type);

    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, getName(), VarType);
    
    // Initialize to 0
    Value* InitVal;
    if (VarType->isFloatTy())
        InitVal = ConstantFP::get(TheContext, APFloat(0.0f));
    else if (VarType->isIntegerTy(1))
        InitVal = ConstantInt::get(TheContext, APInt(1, 0));
    else
        InitVal = ConstantInt::get(TheContext, APInt(32, 0));
    
    Builder.CreateStore(InitVal, Alloca);
    NamedValues[getName()] = Alloca;
    
    return InitVal;
  }
};

// PART 3 ADDITION
/// LocalArrayDeclAST - Class for local array declarations like int arr[5][10];
class LocalArrayDeclAST : public DeclAST {
    std::string Name;
    std::string Type;
    std::vector<int> Dimensions;

public:
    LocalArrayDeclAST(const std::string& name, const std::string& type, std::vector<int> dims)
        : Name(name), Type(type), Dimensions(std::move(dims)) {}
    
    const std::string& getName() const override { return Name; }
    const std::string& getType() const { return Type; }
    const std::vector<int>& getDimensions() const { return Dimensions; }
    
    virtual std::string to_string(int indent = 0, bool isLast = false) const override {
        std::string dimStr = "";
        for (int d : Dimensions) {
            dimStr += "[" + std::to_string(d) + "]";
        }
        return indent_str(indent, isLast) + "LocalArrayDecl(" + Type + " " + Name + dimStr + ")";
    }
    
    virtual Value* codegen() override {
        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        llvm::Type* elemType = getLLVMType(Type);
        llvm::Type* arrayType = createArrayType(elemType, Dimensions);
        
        // Create alloca for the array
        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, Name, arrayType);
        
        // Initialize array to zero
        Builder.CreateMemSet(
            Alloca,
            ConstantInt::get(Type::getInt8Ty(TheContext), 0),
            TheModule->getDataLayout().getTypeAllocSize(arrayType),
            Alloca->getAlign()
        );
        
        // Store in symbol tables
        NamedValues[Name] = Alloca;
        LocalArrayInfo[Name] = {Type, Dimensions};
        
        return Alloca;
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
  const std::string &getName() const override { return Var->getName(); }

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    return indent_str(indent, isLast) + "GlobalVarDecl(" + Type + " " + Var->getName() + ")";
  }

  virtual Value* codegen() override {
    llvm::Type* VarType = getLLVMType(Type);
    
    // Check for redeclaration
    if (GlobalNamedValues.count(getName())) {
        return LogErrorV(("Global variable already defined: " + getName()).c_str());
    }
    
    GlobalVariable* GVar = new GlobalVariable(
        *TheModule, VarType, false,
        GlobalValue::CommonLinkage,
        Constant::getNullValue(VarType),
        getName()
    );
    
    GlobalNamedValues[getName()] = GVar;
    return GVar;
  }
};

// PART 3 ADDITION
/// GlobalArrayDeclAST - Class for global array declarations
class GlobalArrayDeclAST : public DeclAST {
    std::string Name;
    std::string Type;
    std::vector<int> Dimensions;

public:
    GlobalArrayDeclAST(const std::string& name, const std::string& type, std::vector<int> dims)
        : Name(name), Type(type), Dimensions(std::move(dims)) {}
    
    const std::string& getName() const override { return Name; }
    const std::string& getType() const { return Type; }
    
    virtual std::string to_string(int indent = 0, bool isLast = false) const override {
        std::string dimStr = "";
        for (int d : Dimensions) {
            dimStr += "[" + std::to_string(d) + "]";
        }
        return indent_str(indent, isLast) + "GlobalArrayDecl(" + Type + " " + Name + dimStr + ")";
    }
    
    virtual Value* codegen() override {
        llvm::Type* elemType = getLLVMType(Type);
        llvm::Type* arrayType = createArrayType(elemType, Dimensions);
        
        // Check for redeclaration
        if (GlobalNamedValues.count(Name)) {
            return LogErrorV(("Global array already defined: " + Name).c_str());
        }
        
        // Create global variable with zero initializer
        GlobalVariable* GVar = new GlobalVariable(
            *TheModule,
            arrayType,
            false,  // not constant
            GlobalValue::CommonLinkage,
            Constant::getNullValue(arrayType),
            Name
        );
        
        // Store in symbol tables
        GlobalNamedValues[Name] = GVar;
        GlobalArrayInfo[Name] = {Type, Dimensions};
        
        return GVar;
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

  std::string to_string(int indent = 0, bool isLast = false) const {
    std::string result = indent_str(indent, isLast) + "FunctionProto(" + Type + " " + Name + ")";
    for (size_t i = 0; i < Params.size(); i++) {
      bool lastParam = (i == Params.size() - 1);
      result += "\n" + Params[i]->to_string(indent + 1, lastParam);
    } 
    return result;
  }

  Function* codegen() {
    std::vector<llvm::Type*> ParamTypes;
    for (auto& P : Params) {
      if (P->isArray()) {
        // Array parameters decay to pointers in C
        // For int a[10], will use ptr (pointer to element type)
        llvm::Type* elemType = getLLVMType(P->getType());
        ParamTypes.push_back(PointerType::getUnqual(TheContext));
      } else {
        ParamTypes.push_back(getLLVMType(P->getType()));
      }
    }
    
    llvm::Type* RetType = getLLVMType(Type);
    FunctionType* FT = FunctionType::get(RetType, ParamTypes, false);
    
    Function* F = Function::Create(FT, Function::ExternalLinkage, 
                                   Name, TheModule.get());
    
    // Set names for arguments
    unsigned Idx = 0;
    for (auto& Arg : F->args()) {
        Arg.setName(Params[Idx++]->getName());
    }
    
    return F;
  }
};


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

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
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
      std::string result = indent_str(indent, isLast) + "BinaryExpr(" + opStr + ")";
      result += "\n" + LHS->to_string(indent + 1, false);
      result += "\n" + RHS->to_string(indent + 1, true);
      return result;
    } else {
      // unary operator
      std::string result = indent_str(indent, isLast) + "UnaryExpr(" + opStr + ")";
      result += "\n" + RHS->to_string(indent + 1, true);
      return result;
    }
  }

  virtual Value* codegen() override {
  // Handle unary operators (LHS is nullptr)
  if (!LHS) {
      Value* R = RHS->codegen();
      if (!R) return nullptr;
      
      switch(OpTok.type) {
          case NOT: {
              // Convert to bool first if needed, then negate
              if (!R->getType()->isIntegerTy(1)) {
                  R = Builder.CreateICmpNE(R, 
                      ConstantInt::get(R->getType(), 0), "tobool");
              }
              return Builder.CreateNot(R, "nottmp");
          }
          case MINUS: {
              if (R->getType()->isFloatTy())
                  return Builder.CreateFNeg(R, "negtmp");
              else
                  return Builder.CreateNeg(R, "negtmp");
          }
          default:
              return LogErrorV("Invalid unary operator");
      }
  }

  // Handle binary operators
  Value* L = LHS->codegen();
  Value* R = RHS->codegen();
  if (!L || !R) return nullptr;
  
  // Type coercion - promote to common type
  if (L->getType() != R->getType()) {
      if (L->getType()->isFloatTy() || R->getType()->isFloatTy()) {
          L = promoteType(L, Type::getFloatTy(TheContext));
          R = promoteType(R, Type::getFloatTy(TheContext));
      } else if (L->getType()->isIntegerTy(32) || R->getType()->isIntegerTy(32)) {
          L = promoteType(L, Type::getInt32Ty(TheContext));
          R = promoteType(R, Type::getInt32Ty(TheContext));
      }
  }

  bool isFloat = L->getType()->isFloatTy();
    
    switch(OpTok.type) {
        case PLUS:
            return isFloat ? Builder.CreateFAdd(L, R, "addtmp")
                          : Builder.CreateAdd(L, R, "addtmp");
        case MINUS:
            return isFloat ? Builder.CreateFSub(L, R, "subtmp")
                          : Builder.CreateSub(L, R, "subtmp");
        case ASTERIX:
            return isFloat ? Builder.CreateFMul(L, R, "multmp")
                          : Builder.CreateMul(L, R, "multmp");
        case DIV:
            return isFloat ? Builder.CreateFDiv(L, R, "divtmp")
                          : Builder.CreateSDiv(L, R, "divtmp");
        case MOD:
            return Builder.CreateSRem(L, R, "modtmp");
        case LT:
            return isFloat ? Builder.CreateFCmpULT(L, R, "cmptmp")
                          : Builder.CreateICmpSLT(L, R, "cmptmp");
        case LE:
            return isFloat ? Builder.CreateFCmpULE(L, R, "cmptmp")
                          : Builder.CreateICmpSLE(L, R, "cmptmp");
        case GT:
            return isFloat ? Builder.CreateFCmpUGT(L, R, "cmptmp")
                          : Builder.CreateICmpSGT(L, R, "cmptmp");
        case GE:
            return isFloat ? Builder.CreateFCmpUGE(L, R, "cmptmp")
                          : Builder.CreateICmpSGE(L, R, "cmptmp");
        case EQ:
            return isFloat ? Builder.CreateFCmpUEQ(L, R, "cmptmp")
                          : Builder.CreateICmpEQ(L, R, "cmptmp");
        case NE:
            return isFloat ? Builder.CreateFCmpUNE(L, R, "cmptmp")
                          : Builder.CreateICmpNE(L, R, "cmptmp");
        case AND: {
            Value* LBool = Builder.CreateICmpNE(L, 
                ConstantInt::get(L->getType(), 0), "tobool");
            Value* RBool = Builder.CreateICmpNE(R, 
                ConstantInt::get(R->getType(), 0), "tobool");
            return Builder.CreateAnd(LBool, RBool, "andtmp");
        }
        case OR: {
            Value* LBool = Builder.CreateICmpNE(L, 
                ConstantInt::get(L->getType(), 0), "tobool");
            Value* RBool = Builder.CreateICmpNE(R, 
                ConstantInt::get(R->getType(), 0), "tobool");
            return Builder.CreateOr(LBool, RBool, "ortmp");
        }
        default:
            return LogErrorV("Invalid binary operator");
    }
  }
};

class AssignExprAST : public ASTnode {
  std::unique_ptr<ASTnode> LHS;  // Changed from VariableASTnode - can be Variable OR ArrayAccess
  std::unique_ptr<ASTnode> RHS;

public:
  AssignExprAST(std::unique_ptr<ASTnode> lhs, std::unique_ptr<ASTnode> rhs)
      :LHS(std::move(lhs)), RHS(std::move(rhs)) {}

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "Assignment";
    result += "\n" + LHS->to_string(indent + 1, false);
    result += "\n" + RHS->to_string(indent + 1, true);
    return result;
  }

  virtual Value* codegen() override {
    // Generate RHS value
    Value* Val = RHS->codegen();
    if (!Val) return nullptr;
    
    // Check if LHS is an array access
    ArrayAccessAST* arrayAccess = dynamic_cast<ArrayAccessAST*>(LHS.get());
    if (arrayAccess) {
      Value* elemPtr = arrayAccess->codegenPtr();
      if (!elemPtr) return nullptr;

      // Get element type for proper type promotion
      // ... (need to get array info for type)
      ArrayInfo* info = nullptr;
      std::string arrName = arrayAccess->getName();
      if (LocalArrayInfo.count(arrName)) {
        info = &LocalArrayInfo[arrName];
      } else if (GlobalArrayInfo.count(arrName)) {
        info = &GlobalArrayInfo[arrName];
      }

      if (info) {
        Val = promoteType(Val, getLLVMType(info->elementType));
      }

      Builder.CreateStore(Val, elemPtr);
      return Val;
    }

    // Otherwise, its a regular variable assignment
    VariableASTnode* varNode = dynamic_cast<VariableASTnode*>(LHS.get());
    if (!varNode) {
      return LogErrorV("Invalid left-hand side in assignment");
    }

    std::string varName = varNode->getName();
    
    // Look up variable in local scope first
    AllocaInst* Variable = NamedValues[varName];
    if (Variable) {
        // Type check and store locally
        Val = promoteType(Val, Variable->getAllocatedType());
        Builder.CreateStore(Val, Variable);
        return Val;
    }
    
    // Check global scope
    GlobalVariable* GVar = GlobalNamedValues[varName];
    if (GVar) {
        // Type check and store
        Val = promoteType(Val, GVar->getValueType());
        Builder.CreateStore(Val, GVar);
        return Val;
    }
    
    return LogErrorV(("Unknown variable in assignment: " + varName).c_str());
  }
};

/// BlockAST - Class for a block with declarations followed by statements
class BlockAST : public ASTnode {
  std::vector<std::unique_ptr<DeclAST>> LocalDecls; // vector of local decls
  std::vector<std::unique_ptr<ASTnode>> Stmts;         // vector of statements

public:
  BlockAST(std::vector<std::unique_ptr<DeclAST>> localDecls,
           std::vector<std::unique_ptr<ASTnode>> stmts)
      : LocalDecls(std::move(localDecls)), Stmts(std::move(stmts)) {}

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "Block:";

    bool hasLocalDecls = !LocalDecls.empty();
    bool hasStmts = !Stmts.empty();

    if (hasLocalDecls) {
      bool localDeclsLast = !hasStmts;
      result += "\n" + indent_str(indent + 1, localDeclsLast) + "LocalDecls:";
      for (size_t i = 0; i < LocalDecls.size(); i++) {
        bool lastDecl = (i == LocalDecls.size() - 1);
        result += "\n" + LocalDecls[i]->to_string(indent + 2, lastDecl);
      }
    }
    if (hasStmts) {
      result += "\n" + indent_str(indent + 1, true) + "Statements:";
      for (size_t i = 0; i < Stmts.size(); i++) {
        if (Stmts[i]) {
          bool lastStmt = (i == Stmts.size() - 1);
          result += "\n" + Stmts[i]->to_string(indent + 2, lastStmt);
        }
      }
    }
    return result;
  }

  virtual Value* codegen() override {
    // Save current scope (for nested blocks)
    std::map<std::string, AllocaInst*> OldBindings;
    
    // Generate code for local declarations
    for (auto& Decl : LocalDecls) {
        // Save old binding if variable shadows outer scope
        if (NamedValues.count(Decl->getName())) {
            OldBindings[Decl->getName()] = NamedValues[Decl->getName()];
        }
        Decl->codegen();
    }
    
    // Generate code for statements
    Value* LastVal = nullptr;
    for (auto& Stmt : Stmts) {
        if (Stmt) {
            LastVal = Stmt->codegen();
            // Stop if we hit a terminator (return statement)
            if (Builder.GetInsertBlock()->getTerminator())
                break;
        }
    }

    // Restore outer scope bindings
    for (auto& Binding : OldBindings) {
        NamedValues[Binding.first] = Binding.second;
    }
    // Remove local variables that weren't shadowing
    for (auto& Decl : LocalDecls) {
        if (!OldBindings.count(Decl->getName())) {
            NamedValues.erase(Decl->getName());
        }
    }
    
    return LastVal;
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

  const std::string& getName() const override { return Proto->getName();}
  
  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "FunctionDecl\n"; 
    result += "\n" + Proto->to_string(indent + 1, false);
    result += "\n" + Block->to_string(indent + 1, true);
    return result;
  }

  virtual Value* codegen() override {
    // Check for existing function from extern declaration
    Function* TheFunction = TheModule->getFunction(Proto->getName());
    
    if (!TheFunction) {
        TheFunction = Proto->codegen();
    }
    
    if (!TheFunction) return nullptr;

    if (!TheFunction->empty()) {
      return LogErrorV("Function cannot be redefined");
    }
    
    // Create entry basic block
    BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);
    
    // Clear local variables and set current function
    NamedValues.clear();
    LocalArrayInfo.clear();
    ParamArrayInfo.clear();
    CurrentFunction = TheFunction;

    // Create allocas for function arguments
    auto& Params = Proto->getParams();
    unsigned Idx = 0;
    for (auto& Arg : TheFunction->args()) {
        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, 
                                                     std::string(Arg.getName()),
                                                     Arg.getType());
        Builder.CreateStore(&Arg, Alloca);
        NamedValues[std::string(Arg.getName())] = Alloca;

        // Track array parameters
        if (Params[Idx]->isArray()) {
          ArrayInfo info;
          info.elementType = Params[Idx]->getType();
          info.dimensions = Params[Idx]->getDims();
          ParamArrayInfo[std::string(Arg.getName())] = info;
        }
        Idx++;
    }
    
    // Generate function body - ADDED ERROR CHECK HERE
    Value* BodyVal = Block->codegen();
    if (!BodyVal) {
      // Error in function body - remove the function
      TheFunction->eraseFromParent();
      return nullptr; // Propagate error
    }
    
    // Add implicit return if function body doesn't end with return
    if (!Builder.GetInsertBlock()->getTerminator()) {
        if (TheFunction->getReturnType()->isVoidTy()) {
            Builder.CreateRetVoid();
        } else {
            // Return default value (0 or 0.0)
            Builder.CreateRet(Constant::getNullValue(TheFunction->getReturnType()));
        }
    }

    // Verify function
    verifyFunction(*TheFunction);
    
    return TheFunction;
  }
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then,
            std::unique_ptr<ASTnode> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
  
  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "If:";
    bool hasElse = (Else != nullptr);
    result += "\n" + indent_str(indent + 1, false) + "Condition:";
    result += "\n" + Cond->to_string(indent + 2, true) + "";
    result += "\n" + indent_str(indent + 1, !hasElse) + "Then:";
    result += "\n" + Then->to_string(indent + 2, true);
    if (Else) {
      result += "\n" + indent_str(indent, true) + "Else:";
      result += "\n" + Else->to_string(indent + 1, true);
    }
    return result;
  }

  virtual Value* codegen() override {
    Value* CondV = Cond->codegen();
    if (!CondV) return nullptr;
    
    // Convert condition to bool if needed
    if (!CondV->getType()->isIntegerTy(1)) {
        CondV = Builder.CreateICmpNE(CondV, 
            ConstantInt::get(CondV->getType(), 0), "ifcond");
    }
    
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    
    BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
    BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else");
    BasicBlock* MergeBB = BasicBlock::Create(TheContext, "ifcont");
    
    if (Else) {
        Builder.CreateCondBr(CondV, ThenBB, ElseBB);
    } else {
        Builder.CreateCondBr(CondV, ThenBB, MergeBB);
    }

    // Emit then block
    Builder.SetInsertPoint(ThenBB);
    Then->codegen();
    if (!Builder.GetInsertBlock()->getTerminator())
        Builder.CreateBr(MergeBB);
    
    // Emit else block if it exists
    if (Else) {
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder.SetInsertPoint(ElseBB);
        Else->codegen();
        if (!Builder.GetInsertBlock()->getTerminator())
            Builder.CreateBr(MergeBB);
    }
    
    // Emit merge block
    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder.SetInsertPoint(MergeBB);
    
    return Constant::getNullValue(Type::getInt32Ty(TheContext));
  }
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Body;

public:
  WhileExprAST(std::unique_ptr<ASTnode> cond, std::unique_ptr<ASTnode> body)
      : Cond(std::move(cond)), Body(std::move(body)) {}
  
  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "WhileStmt";
    result += "\n" + indent_str(indent + 1, false) + "Condition:";
    result += "\n" + Cond->to_string(indent + 2, true);
    result += "\n" + indent_str(indent + 1, true) + "Body:";
    result += "\n" + Body->to_string(indent + 2, true);
    return result;
  }

  virtual Value* codegen() override {
    Function* TheFunction = Builder.GetInsertBlock()->getParent();
    
    BasicBlock* LoopCondBB = BasicBlock::Create(TheContext, "loopcond", TheFunction);
    BasicBlock* LoopBodyBB = BasicBlock::Create(TheContext, "loopbody");
    BasicBlock* AfterLoopBB = BasicBlock::Create(TheContext, "afterloop");
    
    // Branch to loop condition
    Builder.CreateBr(LoopCondBB);
    
    // Emit loop condition
    Builder.SetInsertPoint(LoopCondBB);
    Value* CondV = Cond->codegen();
    if (!CondV) return nullptr;
    
    // Convert to bool if needed
    if (!CondV->getType()->isIntegerTy(1)) {
        CondV = Builder.CreateICmpNE(CondV, 
            ConstantInt::get(CondV->getType(), 0), "loopcond");
    }

    Builder.CreateCondBr(CondV, LoopBodyBB, AfterLoopBB);
    
    // Emit loop body
    TheFunction->insert(TheFunction->end(), LoopBodyBB);
    Builder.SetInsertPoint(LoopBodyBB);
    Body->codegen();
    Builder.CreateBr(LoopCondBB);
    
    // After loop
    TheFunction->insert(TheFunction->end(), AfterLoopBB);
    Builder.SetInsertPoint(AfterLoopBB);
    
    return Constant::getNullValue(Type::getInt32Ty(TheContext));
  }
};

/// ReturnAST - Class for a return value
class ReturnAST : public ASTnode {
  std::unique_ptr<ASTnode> Val;

public:
  ReturnAST(std::unique_ptr<ASTnode> value) : Val(std::move(value)) {}

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    if (Val) {
      std::string result = indent_str(indent, isLast) + "ReturnStmt:";
      result += "\n" + Val->to_string(indent + 1, true);
      return result;
    } else {
      return indent_str(indent, isLast) + "ReturnStmt(void)";
    }
  }

  virtual Value* codegen() override {
    if (Val) {
        Value* RetVal = Val->codegen();
        if (!RetVal) return nullptr;
        
        // Type check return value against function return type
        Type* FuncRetType = CurrentFunction->getReturnType();
        RetVal = promoteType(RetVal, FuncRetType);
        
        return Builder.CreateRet(RetVal);
    } else {
        return Builder.CreateRetVoid();
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

  virtual std::string to_string(int indent = 0, bool isLast = false) const override {
    std::string result = indent_str(indent, isLast) + "FunctionCall(" + Callee + ")";
    for (size_t i = 0; i < ArgsList.size(); i++) {
      bool lastArg = (i == ArgsList.size() - 1);
      result += "\n" + ArgsList[i]->to_string(indent + 1, lastArg);
    }
    return result;
  }

  virtual Value* codegen() override {
    // Look up function
    Function* CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF) 
        return LogErrorV(("Unknown function referenced: " + Callee).c_str());
    
    // Check argument count
    if (CalleeF->arg_size() != ArgsList.size())
        return LogErrorV("Incorrect number of arguments");
    
    std::vector<Value*> ArgsV;
    auto ArgIt = CalleeF->arg_begin();
    for (unsigned i = 0; i < ArgsList.size(); ++i, ++ArgIt) {
        Value* ArgVal = ArgsList[i]->codegen();
        if (!ArgVal) return nullptr;
        
        // Type coercion for arguments (widening only)
        ArgVal = promoteType(ArgVal, ArgIt->getType());
        ArgsV.push_back(ArgVal);
    }
    
    if (CalleeF->getReturnType()->isVoidTy()) {
        return Builder.CreateCall(CalleeF, ArgsV);
    }
    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
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

//===----------------------------------------------------------------------===//
// Recursive Descent - Function call for each production
//===----------------------------------------------------------------------===//

static std::unique_ptr<ASTnode> ParseDecl();
static std::unique_ptr<ASTnode> ParseStmt();
static std::unique_ptr<ASTnode> ParseBlock();
static std::unique_ptr<ASTnode> ParseExper();
static std::unique_ptr<ParamAST> ParseParam();
static std::unique_ptr<DeclAST> ParseLocalDecl();
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

    // Check for array parameter
    if (CurTok.type == LBOX) {
      std::vector<int> dims;

      while (CurTok.type == LBOX) {
        getNextToken(); // eat '['

        if (CurTok.type != INT_LIT) {
          LogError(CurTok, "Expected integer literal for array dimension in parameter");
          return nullptr;
        }
        dims.push_back(CurTok.getIntVal());
        getNextToken(); // eat INT_LIT

        if (CurTok.type != RBOX) {
          LogError(CurTok, "Expected ']' after array dimension in parameter");
          return nullptr;
        }
        getNextToken(); // eat ']'
      }
      
      return std::make_unique<ParamAST>(Name, Type, std::move(dims));
    }

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
static std::unique_ptr<ASTnode> ParseAssignExpr();
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

// '=' (lowest precedence)
static std::unique_ptr<ASTnode> ParseAssignExpr() {
  auto lhs = ParseOrExpr(); // next level down in precedence    

  if (CurTok.type == ASSIGN) { // '=' token
    // LHS must be a variable OR array access
    VariableASTnode* varNode = dynamic_cast<VariableASTnode*>(lhs.get());
    ArrayAccessAST* arrayNode = dynamic_cast<ArrayAccessAST*>(lhs.get());

    if (!varNode && !arrayNode) {
      return LogError(CurTok, "Left side of assignment must be a variable or array element");
    }

    getNextToken(); // eat '='
    auto rhs = ParseAssignExpr(); // right-associative
    // wrap in AssignExprAST node
    return std::make_unique<AssignExprAST>(std::move(lhs), std::move(rhs));
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

  // Check if this is array access
  if (CurTok.type == LBOX) {
    // Must be an indentifier for array access
    VariableASTnode* varNode = dynamic_cast<VariableASTnode*>(expr.get());
    if (!varNode) {
      return LogError(CurTok, "Expected identifier before '['");
    }
    std::string arrayName = varNode->getName();

    std::vector<std::unique_ptr<ASTnode>> indices;

    while (CurTok.type == LBOX) {
      getNextToken(); // eat '['

      auto index = ParseAssignExpr(); // Parse index expression
      if (!index) return nullptr;
      indices.push_back(std::move(index));

      if (CurTok.type != RBOX) {
        return LogError(CurTok, "Expected ']' after array index");
      }
      getNextToken(); // eat ']'
    }

    return std::make_unique<ArrayAccessAST>(arrayName, std::move(indices));
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
    return LogError(CurTok, "unexpected token in statement\n");
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
static std::vector<std::unique_ptr<DeclAST>> ParseLocalDeclsPrime() {
  std::vector<std::unique_ptr<DeclAST>>
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
static std::unique_ptr<DeclAST> ParseLocalDecl() {
  TOKEN PrevTok;
  std::string Type;
  std::string Name = "";
  // std::unique_ptr<VarDeclAST> local_decl;

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(var_type)
    PrevTok = CurTok;
    getNextToken(); // eat 'int' or 'float or 'bool'

    if (CurTok.type == IDENT) {
      Type = PrevTok.lexeme;
      Name = CurTok.getIdentifierStr(); // save the identifier name
      // getNextToken(); 
      // auto ident = std::make_unique<VariableASTnode>(CurTok, Name);
      // local_decl = std::make_unique<VarDeclAST>(std::move(ident), Type);
      getNextToken(); // eat 'IDENT'

      // Check for array declararion: IDENT "[" INT_LIT "]" ...
      if (CurTok.type == LBOX) {
        std::vector<int> dimensions;

        // Parse dimensions: [size1][size2][size3]
        while (CurTok.type == LBOX) {
          getNextToken(); // eat '['

          if (CurTok.type != INT_LIT) {
            LogError(CurTok, "Expected integer literal for array dimension");
            return nullptr;
          }
          dimensions.push_back(CurTok.getIntVal());
          getNextToken(); // eat INT_LIT

          if (CurTok.type != RBOX) {
            LogError(CurTok, "Expected ']' after array dimension");
            return nullptr;
          }
          getNextToken(); // eat ']'
        }

        if (CurTok.type != SC) {
          LogError(CurTok, "Expected ';' after array declaration");
          return nullptr;
        }
        getNextToken(); // eat ';'

        fprintf(stderr, "Parsed a local array declaration\n");
        return std::make_unique<LocalArrayDeclAST>(Name, Type, std::move(dimensions));
      }

      // Regular variable declarations
      if (CurTok.type != SC) {
        LogError(CurTok, "Expected ';' to end local variable declaration");
        return nullptr;
      }
      getNextToken(); // eat ';'
      auto ident = std::make_unique<VariableASTnode>(PrevTok, Name);
      fprintf(stderr, "Parsed a local variable declaration\n");
      return std::make_unique<VarDeclAST>(std::move(ident), Type);
    } else {
      LogError(CurTok, "expected identifier' in local variable declaration");
      return nullptr;
    }
  }
  return nullptr;
}

// local_decls ::= local_decl local_decls_prime
static std::vector<std::unique_ptr<DeclAST>> ParseLocalDecls() {
  std::vector<std::unique_ptr<DeclAST>> local_decls; // vector of local decls

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
  std::vector<std::unique_ptr<DeclAST>> local_decls; // vector of local decls
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

      // Check for global array declaration
      if (CurTok.type == LBOX) {
        std::vector<int> dimensions;

        while (CurTok.type == LBOX) {
          getNextToken(); // eat '['

          if (CurTok.type != INT_LIT) {
            return LogError(CurTok, "Expected integer literal for array dimension");
          }
          dimensions.push_back(CurTok.getIntVal());
          getNextToken(); // eat INT_LIT

          if (CurTok.type != RBOX) {
            return LogError(CurTok, "Expected ']' after array dimension");
          }
          getNextToken(); // eat ']'
        }

        if (CurTok.type != SC) {
          return LogError(CurTok, "Expected ';' after array declaration");
        }
        getNextToken(); // est ';'

        if (PrevTok.type == VOID_TOK) {
          return LogError(PrevTok, "Cannot have array with type 'void'");
        }

        fprintf(stderr, "Parsed a global array declaration\n");
        return std::make_unique<GlobalArrayDeclAST>(IdName, PrevTok.lexeme, std::move(dimensions));
      }

      if (CurTok.type == SC) {  // found ';' then this is a global variable declaration.
        getNextToken(); // eat ;
        fprintf(stderr, "Parsed a variable declaration\n");

        if (PrevTok.type != VOID_TOK){
          auto globVar = std::make_unique<GlobVarDeclAST>(std::move(ident), PrevTok.lexeme);
          // fprintf(stderr, "AST: %s\n", globVar->to_string().c_str());
          return globVar;
        } else
          return LogError(PrevTok,
                          "Cannot have variable declaration with type 'void'");
      } else if (CurTok.type == LPAR) { // found '(' then this is a function declaration.
        getNextToken();  // eat (

        auto P = ParseParams(); // parse the parameters, returns a vector of params
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
// AST Printer
//===----------------------------------------------------------------------===//

// Function to print the complete AST
static void PrintAST() {
  fprintf(stderr, "\n");
  fprintf(stderr, "==================== COMPLETE AST ====================\n");
  fprintf(stderr, "\n");
  
  // Print extern declarations
  if (!ExternAST.empty()) {
    fprintf(stderr, "=== Extern Declarations ===\n");
    for (const auto& externDecl : ExternAST) {
      fprintf(stderr, "%s\n\n", externDecl->to_string().c_str());
    }
  }
  
  // Print top-level declarations
  if (!ProgramAST.empty()) {
    fprintf(stderr, "=== Top-Level Declarations ===\n");
    for (const auto& decl : ProgramAST) {
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

  /*
  fprintf(stderr, "\n=== Abstract Syntax Tree ===\n");
  for (const auto& ext : ExternAST) {
    fprintf(stderr, "AST: %s\n", ext->to_string().c_str());
  }
  for (const auto& node : ProgramAST) {
    fprintf(stderr, "AST: %s\n", node->to_string().c_str());
  }
  fprintf(stderr, "=== End of AST ===\n\n");
  */

  // Print the complete AST after successful parse
  PrintAST();

  fprintf(stderr, "Starting code generation...\n");

  // Generate code for extern declarations
  fprintf(stderr, "Number of extern declarations: %zu\n", ExternAST.size());
  for (size_t i = 0; i < ExternAST.size(); i++) {
    fprintf(stderr, "  Generating extern %zu: %s\n", i, ExternAST[i]->getName().c_str());
    Function* F = ExternAST[i]->codegen();
    if (!F) {
      fprintf(stderr, "\n*** COMPILATION FAILED: Error in extern declaration ***\n");
      fclose(pFile);
      return 1;
    }
  }

  // Generate code for all top-level declarations
  fprintf(stderr, "Number of top-level declarations: %zu\n", ProgramAST.size());
  for (size_t i = 0; i < ProgramAST.size(); i++) {
    fprintf(stderr, "  Generating top-level declaration %zu\n", i);
    Value* V = ProgramAST[i]->codegen();
    if (!V) {
      fprintf(stderr, "\n*** COMPILATION FAILED: Semantic error detected ***\n");
      fclose(pFile);
      return 1;
    }
  }

  fprintf(stderr, "Code generation finished\n");

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
  // Print to stderr for debugging
  TheModule->print(errs(), nullptr);

  printf("********************* FINAL IR (end) ******************************\n");

  fclose(pFile);
  return 0;
}
