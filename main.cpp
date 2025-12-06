#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

enum Token {
  tok_eof = -1,
  tok_def = -2,
  tok_extern = -3,
  tok_identifier = -4,
  tok_number = -5,
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for = -9,
  tok_in = -10,
  tok_binary = -11,
  tok_unary = -12,
  tok_var = -13,
};

static std::string IdentifierStr;
static double NumVal;

static int gettok() {
  static int LastChar = ' ';

  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) {
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "def")    return tok_def;
    if (IdentifierStr == "extern") return tok_extern;
    if (IdentifierStr == "if")     return tok_if;
    if (IdentifierStr == "then")   return tok_then;
    if (IdentifierStr == "else")   return tok_else;
    if (IdentifierStr == "for")    return tok_for;
    if (IdentifierStr == "in")     return tok_in;
    if (IdentifierStr == "binary") return tok_binary;
    if (IdentifierStr == "unary")  return tok_unary;
    if (IdentifierStr == "var")    return tok_var;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), 0);
    return tok_number;
  }

  if (LastChar == '#') {
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  if (LastChar == EOF)
    return tok_eof;

  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// AST
//===----------------------------------------------------------------------===//

class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual void print(int indent = 0) const = 0;
};

class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double Val) : Val(Val) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "Number: " << Val << "\n";
  }
};

class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  const std::string &getName() const { return Name; }
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "Variable: " << Name << "\n";
  }
};

class VarDeclExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  std::unique_ptr<ExprAST> Body;
public:
  VarDeclExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
                 std::unique_ptr<ExprAST> Body)
      : VarNames(std::move(VarNames)), Body(std::move(Body)) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "VarDecl:\n";
    for (const auto &var : VarNames)
      std::cout << std::string(indent + 2, ' ') << var.first << "\n";
    Body->print(indent + 2);
  }
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;
public:
  BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
      : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "BinOp: " << Op << "\n";
    LHS->print(indent + 2);
    RHS->print(indent + 2);
  }
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;
public:
  CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args)
      : Callee(Callee), Args(std::move(Args)) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "Call: " << Callee << "\n";
    for (const auto &arg : Args)
      arg->print(indent + 2);
  }
};

class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;
public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "If:\n";
    std::cout << std::string(indent + 2, ' ') << "Cond:\n";
    Cond->print(indent + 4);
    std::cout << std::string(indent + 2, ' ') << "Then:\n";
    Then->print(indent + 4);
    if (Else) {
      std::cout << std::string(indent + 2, ' ') << "Else:\n";
      Else->print(indent + 4);
    }
  }
};

class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;
public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
      : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}
  void print(int indent = 0) const override {
    std::cout << std::string(indent, ' ') << "For: " << VarName << "\n";
    Start->print(indent + 2);
    End->print(indent + 2);
    if (Step) Step->print(indent + 2);
    Body->print(indent + 2);
  }
};

class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;
public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Prec = 0)
      : Name(Name), Args(std::move(Args)), IsOperator(IsOperator), Precedence(Prec) {}
  const std::string &getName() const { return Name; }
  const std::vector<std::string> &getArgs() const { return Args; }
  bool isOperator() const { return IsOperator; }
  unsigned getPrecedence() const { return Precedence; }
};

class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;
public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  PrototypeAST *getProto() { return Proto.get(); }
  ExprAST *getBody() { return Body.get(); }
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

static std::map<char, int> BinopPrecedence;

static int getTokPrecedence() {
  if (!isascii(CurTok)) return -1;
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParsePrimary();

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken();
  return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  getNextToken();

  if (CurTok != '(')
    return std::make_unique<VariableExprAST>(IdName);

  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return nullptr;
      getNextToken();
    }
  }

  getNextToken();
  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken();
  auto V = ParseExpression();
  if (!V) return nullptr;

  if (CurTok != ')')
    return nullptr;
  getNextToken();
  return V;
}

static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();
  auto Cond = ParseExpression();
  if (!Cond) return nullptr;

  if (CurTok != tok_then)
    return nullptr;
  getNextToken();

  auto Then = ParseExpression();
  if (!Then) return nullptr;

  std::unique_ptr<ExprAST> Else;
  if (CurTok == tok_else) {
    getNextToken();
    Else = ParseExpression();
    if (!Else) return nullptr;
  }

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then), std::move(Else));
}

static std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();
  if (CurTok != tok_identifier)
    return nullptr;

  std::string IdName = IdentifierStr;
  getNextToken();

  if (CurTok != '=')
    return nullptr;
  getNextToken();

  auto Start = ParseExpression();
  if (!Start) return nullptr;

  if (CurTok != ',')
    return nullptr;
  getNextToken();

  auto End = ParseExpression();
  if (!End) return nullptr;

  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
    getNextToken();
    Step = ParseExpression();
    if (!Step) return nullptr;
  }

  if (CurTok != tok_in)
    return nullptr;
  getNextToken();

  auto Body = ParseExpression();
  if (!Body) return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
                                       std::move(Step), std::move(Body));
}

static std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken();

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  if (CurTok != tok_identifier)
    return nullptr;

  while (1) {
    std::string Name = IdentifierStr;
    getNextToken();

    std::unique_ptr<ExprAST> Init;
    if (CurTok == '=') {
      getNextToken();
      Init = ParseExpression();
      if (!Init) return nullptr;
    }

    VarNames.push_back(std::make_pair(Name, std::move(Init)));

    if (CurTok != ',')
      break;
    getNextToken();
    if (CurTok != tok_identifier)
      return nullptr;
  }

  if (CurTok != tok_in)
    return nullptr;
  getNextToken();

  auto Body = ParseExpression();
  if (!Body) return nullptr;

  return std::make_unique<VarDeclExprAST>(std::move(VarNames), std::move(Body));
}

static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  case tok_if:
    return ParseIfExpr();
  case tok_for:
    return ParseForExpr();
  case tok_var:
    return ParseVarExpr();
  default:
    return nullptr;
  }
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  while (1) {
    int TokPrec = getTokPrecedence();

    if (TokPrec < ExprPrec)
      return LHS;

    int BinOp = CurTok;
    getNextToken();

    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    int NextPrec = getTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;

  unsigned Kind = 0;
  unsigned BinaryPrecedence = 30;

  switch (CurTok) {
  default:
    return nullptr;
  case tok_identifier:
    FnName = IdentifierStr;
    Kind = 0;
    getNextToken();
    break;
  case tok_binary:
    getNextToken();
    if (!isascii(CurTok))
      return nullptr;
    FnName = "binary";
    FnName += (char)CurTok;
    Kind = 2;
    getNextToken();
    if (CurTok == tok_number) {
      BinaryPrecedence = (unsigned)NumVal;
      getNextToken();
    }
    break;
  case tok_unary:
    getNextToken();
    if (!isascii(CurTok))
      return nullptr;
    FnName = "unary";
    FnName += (char)CurTok;
    Kind = 1;
    getNextToken();
    break;
  }

  if (CurTok != '(')
    return nullptr;

  std::vector<std::string> ArgNames;
  getNextToken();
  if (CurTok != ')') {
    while (1) {
      if (CurTok != tok_identifier)
        return nullptr;

      ArgNames.push_back(IdentifierStr);
      getNextToken();

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return nullptr;

      getNextToken();
    }
  }
  getNextToken();

  if (Kind && ArgNames.size() != Kind)
    return nullptr;

  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames),
                                        Kind != 0, BinaryPrecedence);
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                 std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken();
  return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Main Driver
//===----------------------------------------------------------------------===//

static void InitializeModuleAndPassManager() {
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;
  BinopPrecedence['/'] = 40;
  BinopPrecedence['%'] = 40;
}

static void MainLoop() {
  while (1) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';':
      getNextToken();
      break;
    case tok_def: {
      if (auto FnAST = ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
        FnAST->getBody()->print();
      } else {
        getNextToken();
      }
      break;
    }
    case tok_extern: {
      if (auto ProtoAST = ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
      } else {
        getNextToken();
      }
      break;
    }
    default: {
      if (auto FnAST = ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
        FnAST->getBody()->print();
      } else {
        getNextToken();
      }
      break;
    }
    }
  }
}

int main() {
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;
  BinopPrecedence['/'] = 40;
  BinopPrecedence['%'] = 40;

  fprintf(stderr, "ready> ");
  getNextToken();
  MainLoop();

  return 0;
}