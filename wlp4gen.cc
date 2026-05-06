#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

// mips code
std::vector<std::string> mipsCode;
std::map<std::string, int> memOffset;
int varOffsetCounter = 8;
int regCounter = 5;
int whileCounter = 0;
int ifCounter = 0;
int skipDeleteCounter = 0;

// node struct
struct Node {
    std::vector<std::string> rule;
    std::string type;
    std::vector<std::unique_ptr<Node>> children;
};

// parses a wlp4i file
std::unique_ptr<Node> wlp4iParse(std::vector<std::vector<std::string>>& input) {
    if (input.empty()) {
        return nullptr;
    }

    std::vector<std::string> curLine = input[0];
    input.erase(input.begin());

    std::unique_ptr<Node> tree = std::make_unique<Node>(curLine);

    if (curLine.size() >= 4 && curLine[curLine.size() - 2] == ":") {
        tree->type = " : " + curLine.back();
        curLine.pop_back();
        curLine.pop_back();
        tree->rule.pop_back();
        tree->rule.pop_back();
    }

    for (size_t i = 1; i < curLine.size(); i++) {
        if (!input.empty() && input[0][0] == curLine[i]) {
            tree->children.push_back(wlp4iParse(input));
        }
    }
    return tree;
}

// print function
void print(std::vector<std::string> mipsInstructions) {
    for (auto instruction : mipsInstructions) {
        std::cout << instruction;
    }
}

// push function
void mipsPush(int symbolRegister) {
    mipsCode.push_back("sw $" + std::to_string(symbolRegister) + ", -4($30)\n");
    mipsCode.push_back("sub $30, $30, $4\n");
}

// pop function
void mipsPop(int symbolRegister) {
    mipsCode.push_back("add $30, $30, $4\n");
    mipsCode.push_back("lw $" + std::to_string(symbolRegister) + ", -4($30)\n");
}

// forward declarations for mipsFactor and mipsTerm
void mipsTerm(const std::unique_ptr<Node> &tree);
void mipsExpr(const std::unique_ptr<Node> &tree);
void mipsLvalue(const std::unique_ptr<Node> &tree);

// arglist function
int mipsArglist(const std::unique_ptr<Node> &tree) {
    mipsExpr(tree->children[0]);
    mipsPush(3);
    if (tree->rule.size() == 4) {
        return 1 + mipsArglist(tree->children[2]);
    } else {
        return 1;
    }
}

// factor function
void mipsFactor(const std::unique_ptr<Node> &tree) {
    if (tree->rule.size() == 2 && tree->rule[1] == "NUM") {
        mipsCode.push_back("lis $3\n");
        mipsCode.push_back(".word " + tree->children[0]->rule[1] + "\n");
    } else if (tree->rule.size() == 2 && tree->rule[1] == "ID") {
        mipsCode.push_back("lw $3, " + std::to_string(memOffset[tree->children[0]->rule[1]]) + "($29)\n");
    } else if (tree->rule.size() == 4 && tree->rule[1] == "GETCHAR") {
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word 0xffff0004\n");
        mipsCode.push_back("lw $3, 0($5)\n");
    } else if (tree->rule.size() == 4 && tree->rule[2] == "LPAREN") {
        mipsPush(29);
        mipsPush(31);
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word F" + tree->children[0]->rule[1] + "\n");
        mipsCode.push_back("jalr $5\n");
        mipsPop(31);
        mipsPop(29);
    } else if (tree->rule.size() == 5 && tree->rule[3] == "arglist") {
        mipsPush(29);
        mipsPush(31);

        int argCounter = mipsArglist(tree->children[2]);

        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word F" + tree->children[0]->rule[1] + "\n");
        mipsCode.push_back("jalr $5\n");

        for (int i = 0; i < argCounter; i++) {
            mipsCode.push_back("add $30, $30, $4\n");
        }

        mipsPop(31);
        mipsPop(29);
    } else if (tree->rule.size() == 2 && tree->rule[1] == "NULL") {
        mipsCode.push_back("add $3, $0, $11\n");
    } else if (tree->rule.size() == 3 && tree->rule[1] == "AMP") {
        mipsLvalue(tree->children[1]);
    } else if (tree->rule.size() == 3 && tree->rule[1] == "STAR") {
        mipsFactor(tree->children[1]);
        mipsCode.push_back("lw $3, 0($3)\n");
    } else if (tree->rule.size() == 6 && tree->rule[1] == "NEW") {
        mipsPush(1);
        mipsExpr(tree->children[3]);
        mipsCode.push_back("add $1, $3, $0\n");
        mipsPush(31);
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word new\n");
        mipsCode.push_back("jalr $5\n");
        mipsPop(31);
        mipsCode.push_back("bne $3, $0, 1\n");
        mipsCode.push_back("add $3, $11, $0\n");
        mipsPop(1);
    } else {
        mipsExpr(tree->children[1]);
    }
}

// term function
void mipsTerm(const std::unique_ptr<Node> &tree) {
    if (tree->rule[0] == "term" && tree->rule.size() == 2) {
        mipsFactor(tree->children[0]);
    } else if (tree->rule[2] == "STAR") {
        mipsTerm(tree->children[0]);
        mipsPush(3);
        mipsFactor(tree->children[2]);
        mipsPop(5);
        mipsCode.push_back("mult $5, $3\n");
        mipsCode.push_back("mflo $3\n");
    } else if (tree->rule[2] == "SLASH") {
        mipsTerm(tree->children[0]);
        mipsPush(3);
        mipsFactor(tree->children[2]);
        mipsPop(5);
        mipsCode.push_back("div $5, $3\n");
        mipsCode.push_back("mflo $3\n");
    } else if (tree->rule[2] == "PCT") {
        mipsTerm(tree->children[0]);
        mipsPush(3);
        mipsFactor(tree->children[2]);
        mipsPop(5);
        mipsCode.push_back("div $5, $3\n");
        mipsCode.push_back("mfhi $3\n");
    }
}

// expr function
void mipsExpr(const std::unique_ptr<Node> &tree) {
    if (tree->rule.size() == 2 && tree->rule[0] == "expr") {
        mipsTerm(tree->children[0]);
    } else if (tree->rule[0] == "expr" && tree->rule[2] == "PLUS") {
        if (tree->children[0]->type == " : int*" && tree->children[2]->type == " : int") {
            mipsExpr(tree->children[0]);
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsCode.push_back("mult $3, $4\n");
            mipsCode.push_back("mflo $3\n");
            mipsPop(5);
            mipsCode.push_back("add $3, $5, $3\n");
        } else if (tree->children[0]->type == " : int" && tree->children[2]->type == " : int*") {
            mipsExpr(tree->children[0]);
            mipsCode.push_back("mult $3, $4\n");
            mipsCode.push_back("mflo $3\n");
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsPop(5);
            mipsCode.push_back("add $3, $5, $3\n");
        } else {
            mipsExpr(tree->children[0]);
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsPop(5);
            mipsCode.push_back("add $3, $5, $3\n");
        }
    } else if (tree->rule[0] == "expr" && tree->rule[2] == "MINUS") { 
        if (tree->children[0]->type == " : int*" && tree->children[2]->type == " : int") {
            mipsExpr(tree->children[0]);
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsCode.push_back("mult $3, $4\n");
            mipsCode.push_back("mflo $3\n");
            mipsPop(5);
            mipsCode.push_back("sub $3, $5, $3\n");
        } else if (tree->children[0]->type == " : int*" && tree->children[2]->type == " : int*") {
            mipsExpr(tree->children[0]);
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsPop(5);
            mipsCode.push_back("sub $3, $5, $3\n");
            mipsCode.push_back("div $3, $4\n");
            mipsCode.push_back("mflo $3\n");
        } else {
            mipsExpr(tree->children[0]);
            mipsPush(3);
            mipsTerm(tree->children[2]);
            mipsPop(5);
            mipsCode.push_back("sub $3, $5, $3\n");
        }
    }
}

// dcls function
void mipsDcls(const std::unique_ptr<Node>& tree) {
    if (tree->rule[0] == "dcls" && tree->rule.size() == 6 && tree->rule[3] == "BECOMES") {
        mipsDcls(tree->children[0]);

        if (tree->rule[4] == "NUM" && tree->children[3]->rule[1] == "0") {
            mipsPush(0);
        } else if (tree->rule[4] == "NULL") {
            mipsPush(11);
        } else {
            mipsCode.push_back("lis $3\n");
            mipsCode.push_back(".word " + tree->children[3]->rule[1] + "\n");
            mipsPush(3);
        }
        memOffset[tree->children[1]->children[1]->rule[1]] = varOffsetCounter;
        varOffsetCounter -= 4;
    }
}

// lvalue function
void mipsLvalue(const std::unique_ptr<Node>& tree) {
    if (tree->rule[1] == "ID") {
        mipsCode.push_back("lis $3\n");
        mipsCode.push_back(".word " + std::to_string(memOffset[tree->children[0]->rule[1]]) + "\n");
        mipsCode.push_back("add $3, $3, $29\n");
    } else if (tree->rule[1] == "STAR") {
        mipsFactor(tree->children[1]);
    } else {
        mipsLvalue(tree->children[1]);
    }
}

// test function
void mipsTest(const std::unique_ptr<Node> &tree) {
    mipsExpr(tree->children[0]);
    mipsPush(3);
    mipsExpr(tree->children[2]);
    mipsPop(5);
    if (tree->rule[2] == "EQ") { 
        mipsCode.push_back("sub $3, $5, $3\n");
        mipsCode.push_back("sltu $3, $3, $11\n");
    } else if (tree->rule[2] == "NE") { 
        mipsCode.push_back("sub $3, $5, $3\n");
        mipsCode.push_back("sltu $3, $0, $3\n");
    } else if (tree->rule[2] == "LT") { 
        if (tree->children[0]->type == " : int") {
            mipsCode.push_back("slt $3, $5, $3\n");
        } else {
            mipsCode.push_back("sltu $3, $5, $3\n");
        }
    } else if (tree->rule[2] == "LE") { 
        if (tree->children[0]->type == " : int") {
            mipsCode.push_back("slt $3, $3, $5\n");
        } else {
            mipsCode.push_back("sltu $3, $3, $5\n");
        }
        mipsCode.push_back("sub $3, $11, $3\n");
    } else if (tree->rule[2] == "GE") { 
        if (tree->children[0]->type == " : int") {
            mipsCode.push_back("slt $3, $5, $3\n");
        } else {
            mipsCode.push_back("sltu $3, $5, $3\n");
        }
        mipsCode.push_back("sub $3, $11, $3\n");
    } else if (tree->rule[2] == "GT") { 
        if (tree->children[0]->type == " : int") {
            mipsCode.push_back("slt $3, $3, $5\n");
        } else {
            mipsCode.push_back("sltu $3, $3, $5\n");
        }
    }
}

// forward declaration for mipsStatements
void mipsStatements(const std::unique_ptr<Node> &tree);

// statement function
void mipsStatement(const std::unique_ptr<Node>& tree) {
    if (tree->rule[1] == "lvalue" && tree->rule.size() == 5) {
        mipsLvalue(tree->children[0]);
        mipsPush(3);
        mipsExpr(tree->children[2]);
        mipsPop(5);
        mipsCode.push_back("sw $3, 0($5)\n");
    } else if (tree->rule[1] == "IF" && tree->rule.size() == 12) {
        int curCounter = ifCounter++;

        mipsTest(tree->children[2]);
        mipsCode.push_back("beq $3, $0, else" + std::to_string(curCounter) + "\n");

        mipsCode.push_back("if" + std::to_string(curCounter) + ":\n");
        mipsStatements(tree->children[5]);
        mipsCode.push_back("beq $0, $0, endIf" + std::to_string(curCounter) + "\n");
        mipsCode.push_back("else" + std::to_string(curCounter) + ":\n");
        mipsStatements(tree->children[9]);
        mipsCode.push_back("endIf" + std::to_string(curCounter) + ":\n");
    } else if (tree->rule[1] == "WHILE" && tree->rule.size() == 8) {
        int curCounter = whileCounter++;

        mipsCode.push_back("while" + std::to_string(curCounter) + ":\n");
        mipsTest(tree->children[2]);
        mipsCode.push_back("beq $3, $0, endWhile" + std::to_string(curCounter) + "\n");
        mipsStatements(tree->children[5]);
        mipsCode.push_back("beq $0, $0, while" + std::to_string(curCounter) + "\n");
        mipsCode.push_back("endWhile" + std::to_string(curCounter) + ":\n");
    } else if (tree->rule[1] == "PUTCHAR" && tree->rule.size() == 6) {
        mipsExpr(tree->children[2]);
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word 0xffff000c\n");
        mipsCode.push_back("sw $3, 0($5)\n");
    } else if (tree->rule[1] == "PRINTLN" && tree->rule.size() == 6) {
        mipsPush(1);
        mipsExpr(tree->children[2]);
        mipsCode.push_back("add $1, $3, $0\n");
        mipsPush(31);
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word print\n");
        mipsCode.push_back("jalr $5\n");
        mipsPop(31);
        mipsPop(1);
    } else if (tree->rule[1] == "DELETE" && tree->rule.size() == 6) {
        skipDeleteCounter++;
        mipsPush(1);
        mipsExpr(tree->children[3]);
        mipsCode.push_back("beq $3, $11, skipDelete" + std::to_string(skipDeleteCounter) + "\n");
        mipsCode.push_back("add $1, $3, $0\n");
        mipsPush(31);
        mipsCode.push_back("lis $5\n");
        mipsCode.push_back(".word delete\n");
        mipsCode.push_back("jalr $5\n");
        mipsPop(31);
        mipsCode.push_back("skipDelete" + std::to_string(skipDeleteCounter) + ":\n");
        mipsPop(1);
    }
}

// statements function
void mipsStatements(const std::unique_ptr<Node>& tree) {
    if (tree->rule[0] == "statements" && tree->rule.size() == 3) {
        mipsStatements(tree->children[0]);
        mipsStatement(tree->children[1]);
    }
}

// wain function
void mipsWain(const std::unique_ptr<Node>& tree) {
    // prologue
    mipsCode.push_back(".import init\n");
    mipsCode.push_back(".import new\n");
    mipsCode.push_back(".import delete\n");
    mipsCode.push_back(".import print\n");
    mipsCode.push_back("lis $4\n");
    mipsCode.push_back(".word 4\n");
    mipsCode.push_back("lis $11\n");
    mipsCode.push_back(".word 1\n");

    // params
    mipsPush(1);
    memOffset[tree->children[3]->children[1]->rule[1]] = varOffsetCounter;
    varOffsetCounter -= 4;

    mipsPush(2);
    memOffset[tree->children[5]->children[1]->rule[1]] = varOffsetCounter;
    varOffsetCounter -= 4;

    mipsCode.push_back("sub $29, $30, $4\n");
    
    mipsPush(31);
    varOffsetCounter -= 4;

    // init heap
    if (tree->children[3]->children[0]->children.size() == 1) { 
        mipsCode.push_back("add $2, $0, $0\n");  
    }
    mipsCode.push_back("lis $5\n");
    mipsCode.push_back(".word init\n");
    mipsCode.push_back("jalr $5\n");
    
    // declarations
    mipsDcls(tree->children[8]);
    // statements
    mipsStatements(tree->children[9]);
    // return
    mipsExpr(tree->children[11]);

    // epilogue
    mipsCode.push_back("add $30, $29, $0\n");
    mipsPop(31);
    mipsCode.push_back("add $30, $30, $4\n");
    mipsCode.push_back("add $30, $30, $4\n");
    mipsCode.push_back("jr $31\n");
}

// params function
void mipsParamlist(const std::unique_ptr<Node>& tree) {
    if (tree->rule.size() > 2) {
        mipsParamlist(tree->children[2]);
        varOffsetCounter += 4;
        memOffset[tree->children[0]->children[1]->rule[1]] = varOffsetCounter;
    } else {
        memOffset[tree->children[0]->children[1]->rule[1]] = varOffsetCounter;
    }
}

// procedure function
void mipsProcedure(const std::unique_ptr<Node>& tree) {
    // label
    mipsCode.push_back("F" + tree->children[1]->rule[1] + ":\n");
    mipsCode.push_back("sub $29, $30, $4\n");

    // params
    memOffset.clear();

    varOffsetCounter = 4;
    if (tree->children[3]->rule[1] == "paramlist") {
        mipsParamlist(tree->children[3]->children[0]);
    }
    varOffsetCounter = 0;

    // declarations
    mipsDcls(tree->children[6]);
    // statements
    mipsStatements(tree->children[7]);
    // return
    mipsExpr(tree->children[9]);
    // epilogue
    mipsCode.push_back("add $30, $29, $4\n");
    mipsCode.push_back("jr $31\n");
}

// procedures function
void mipsProcedures(const std::unique_ptr<Node>& tree) {
    if (tree->children[0]->rule[0] != "main") {
        mipsProcedures(tree->children[1]);
        mipsProcedure(tree->children[0]);
    } else {
        mipsWain(tree->children[0]);
    }
}


// main
int main() {
    std::istream& in = std::cin;
    std::string s;
    std::vector<std::vector<std::string>> input;

    // input collection
    while (std::getline(in, s)) {
        std::istringstream iss(s);
        std::vector<std::string> symbols;
        while (iss >> s) {
            symbols.push_back(std::move(s));
        }
        input.push_back(std::move(symbols));
    }

    // processes wlp4i tree
    std::unique_ptr<Node> wlp4iTree = wlp4iParse(input);
    if (wlp4iTree == nullptr) {
        std::cerr << "ERROR wlp4 not parsed correctly\n";
        return 1;
    }

    // generates mips assembly code
    mipsProcedures(wlp4iTree->children[1]);

    // prints assembly code
    print(mipsCode);
    return 0;
}
