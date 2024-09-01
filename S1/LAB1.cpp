#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <cmath>

using namespace std;


class Token {
public:
    enum Type {PLUS, MINUS, MUL,DIV, NUM, ERR, PD, PI, END, POW, NUM_FLOAT };
    Type type;
    string text;
    Token(Type);
    Token(Type, char c);
    Token(Type, const string& source, int first, int last);
};

class Scanner {
private:
    string input;
    int first, current;
public:
    Scanner(const char* in_s);
    Token* nextToken();
    ~Scanner();

};

enum BinaryOp { PLUS, MINUS,MUL,DIV , POW};

class Exp {
public:
    virtual void print() = 0;
    virtual double eval() = 0;
    virtual ~Exp() = 0;
    static char binopToChar(BinaryOp op);
};


class BinaryExp : public Exp {
public:
    Exp *left, *right;
    BinaryOp op;
    BinaryExp(Exp* l, Exp* r, BinaryOp op);
    void print();
    double eval();
    ~BinaryExp();
};

class NumberExp : public Exp {
public:
    double value;
    NumberExp(double v);
    void print();
    double eval();
    ~NumberExp();
};

class Parser {
private:
    Scanner* scanner;
    Token *current, *previous;
    bool match(Token::Type ttype);
    bool check(Token::Type ttype);
    bool advance();
    bool isAtEnd();
    Exp* parseExpression();
    Exp* parseTerm();
    Exp* parseFactor();
    Exp* parseFExp();
    bool tokenToOp(Token* tk, BinaryOp& op);
public:
    Parser(Scanner* scanner);
    Exp* parse();
};


Token::Token(Type type):type(type) { text = ""; }

Token::Token(Type type, char c):type(type) { text = c; }

Token::Token(Type type, const string& source, int first, int last):type(type) {
    text = source.substr(first,last);
}

std::ostream& operator << ( std::ostream& outs, const Token & tok )
{
    if (tok.text.empty())
        return outs << tok.type;
    else
        return outs << "TOK" << "(" << tok.text << ")";
}

std::ostream& operator << ( std::ostream& outs, const Token* tok ) {
    return outs << *tok;
}

// SCANNER //

Scanner::Scanner(const char* s):input(s),first(0), current(0) { }

Token* Scanner::nextToken() {
    Token* token;
    while (input[current] == ' ') current++;  // Saltar espacios en blanco
    if (input[current] == '\0') return new Token(Token::END);
    
    char c = input[current];
    first = current;

    // Manejo de números (enteros y decimales)
    if (isdigit(c)) {
        current++;
        bool hasDot = false;
        while (isdigit(input[current]) || (input[current] == '.' && !hasDot)) {
            if (input[current] == '.') {
                hasDot = true;  // Permitir solo un punto decimal
            }
            current++;
        }
        token = new Token(Token::NUM, input, first, current - first);
    } else if (strchr("+-*/().", c)) {
        switch (c) {
            case '+': token = new Token(Token::PLUS, c); break;
            case '-': token = new Token(Token::MINUS, c); break;
            case '*': {
                if (input[current + 1] == '*') {
                    token = new Token(Token::POW, '^');
                    current += 1;
                } else {
                    token = new Token(Token::MUL, c);
                }
                break;
            }
            case '/': token = new Token(Token::DIV, c); break;
            case '(': token = new Token(Token::PI, c); break;
            case ')': token = new Token(Token::PD, c); break;
            default: 
                cout << "No deberia llegar aca" << endl;
                token = new Token(Token::ERR, c); // En caso de error no esperado
        }
        current++;
    } else {
        token = new Token(Token::ERR, c);
        current++;
    }
    return token;
}


Scanner::~Scanner() { }

// PARSER //

bool Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(Token::Type ttype) {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp =current;
        if (previous) delete previous;
        current = scanner->nextToken();
        previous = temp;
        if (check(Token::ERR)) {
            cout << "Parse error, unrecognised character: " << current->text << endl;
            exit(0);
        }
        return true;
    }
    return false;
}

bool Parser::isAtEnd() {
    return (current->type == Token::END);
}

Parser::Parser(Scanner* sc):scanner(sc) {
    previous = current = NULL;
    return;
};

Exp* Parser::parse() {
    current = scanner->nextToken();
    if (check(Token::ERR)) {
        cout << "Error en scanner - caracter invalido" << endl;
        exit(0);
    }
    Exp* exp = parseExpression();
    if (current) delete current;
    return exp;
}

Exp* Parser::parseExpression() {

    Exp* left = parseTerm();

    while (match(Token::PLUS) || match(Token::MINUS)) {
        BinaryOp op;
        if (previous->type == Token::PLUS){
            op = PLUS;
        }
        else if (previous->type == Token::MINUS){
            op = MINUS;
        }
        Exp* right = parseTerm();
        left = new BinaryExp(left, right, op);
    }

    return left;
}

Exp* Parser::parseFExp() {
    Exp* left = parseFactor();
    
    while(match(Token::POW)){
        BinaryOp op = POW;
        Exp* right = parseFExp();
        left = new BinaryExp(left, right, op);
    }

    return left;
}

Exp* Parser::parseTerm() {

    Exp* left = parseFExp();

    while (match(Token::MUL) || match(Token::DIV)) {
        BinaryOp op;
        if (previous->type == Token::MUL){
            op = MUL;
        }
        else if (previous->type == Token::DIV){
            op = DIV;
        }
        Exp* right = parseFExp();
        left = new BinaryExp(left, right, op);
    }
    return left;
}

Exp* Parser::parseFactor() {
    Exp* e;
    if (match(Token::NUM)) {
        return new NumberExp(stod(previous->text));
    }
    else if (match(Token::PI)){
        e = parseExpression();
        if (!match(Token::PD)){
            cout << "Falta parentesis derecho" << endl;
        }
        return e;
    }
    cout << "Error: se esperaba un número." << endl;
    exit(0);
}

char Exp::binopToChar(BinaryOp op) {
    char  c=' ';
    switch(op) {
        case PLUS: c = '+'; break;
        case MINUS: c = '-'; break;
        case MUL: c = '*'; break;
        case DIV: c = '/'; break;
        case POW: c = '^'; break;
        default: c = '$';
    }
    return c;
}

// AST //


BinaryExp::BinaryExp(Exp* l, Exp* r, BinaryOp op):left(l),right(r),op(op) {}
NumberExp::NumberExp(double v) : value(v) {}


Exp::~Exp() {}
BinaryExp::~BinaryExp() { delete left; delete right; }
NumberExp::~NumberExp() { }


void BinaryExp::print() {
    left->print();
    char c = binopToChar(this->op);;
    cout << ' ' << c << ' ';
    right->print();
}



void NumberExp::print() {
    cout << value;
}

double BinaryExp::eval() {
    double result;
    double v1=left->eval();
    double v2=right->eval();
    switch(this->op) {
        case PLUS: result = v1+v2; break;
        case MINUS: result = v1-v2; break;
        case MUL: result = v1*v2; break;
        case DIV: result = v1/v2; break;
        case POW: 
            result = 1;
            for (int i = 0; i < v2; i++){
                result = result * v1;
            }
            break;
        default:
            cout << "Operador desconocido" << endl;
            result = 0;
    }
    return result;
}

double NumberExp::eval() {
    return value;
}


void test_scanner(Scanner * scanner) {
    Token* current;
    current = scanner->nextToken();
    while (current->type != Token::END) {
        if (current->type == Token::ERR) {
            cout << "Error en scanner - caracter invalido: " << current->text << endl;
            break;
        } else
            cout << current << endl;
        current = scanner->nextToken();
    }
    exit(1);

}

int main(int argc, const char* argv[]) {

    if (argc != 2) {
        cout << "Incorrect number of arguments" << endl;
        exit(1);
    }

    Scanner scanner(argv[1]);

    //test_scanner(&scanner);

    Parser parser(&scanner);

    Exp *exp = parser.parse();

    cout << "expr: ";
    exp->print();
    cout << endl;

    cout << "eval: ";
    cout << exp->eval() << endl;

    delete exp;
}