#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

// procedure table
std::map<std::string, std::pair<std::vector<std::string>, std::map<std::string, std::string>>> procedureTable;
std::string curProcedure;

// node struct
struct Node {
    std::vector<std::string> rule;
    std::string type;
    std::vector<std::string> arglist;
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

    for (size_t i = 1; i < curLine.size(); i++) {
        if (!input.empty() && input[0][0] == curLine[i]) {
            tree->children.push_back(wlp4iParse(input));
        }
    }
    return tree;
}

// print function
void print(const std::unique_ptr<Node>& tree) {
    for (size_t i = 0; i < tree->rule.size(); i++) {
        if (i != 0) {
            std::cout << " ";
        }
        std::cout << tree->rule[i];
    }

    if (!tree->type.empty()) {
        std::cout << tree->type;
    }

    std::cout << "\n";

    for (const std::unique_ptr<Node>& child : tree->children) {
        print(child);
    }
}

// checks for all name, type, and identifier errors
int typeError(const std::unique_ptr<Node>& tree) {
    if (tree->rule[0] == "main") {
        if (!(tree->children[3]->children[1]->rule[1] != tree->children[5]->children[1]->rule[1] &&
            tree->children[5]->children[0]->rule[1] == "INT" && tree->children[5]->children[0]->rule.size() == 2)) {
            std::cerr << "ERROR main, the two dcl's are matching or the second dcl is not an int\n";
            return 1;
        } else if (procedureTable.count(tree->rule[0])) {
            std::cerr << "ERROR duplicate procedure\n";
            return 1;
        }

        curProcedure = tree->rule[0];

        if (tree->children[3]->children[0]->rule[1] == "INT" && tree->children[3]->children[0]->rule.size() == 2) {
            procedureTable[tree->rule[0]].first.push_back(" : int");
        } else {
            procedureTable[tree->rule[0]].first.push_back(" : int*");
        }

        if (tree->children[5]->children[0]->rule[1] == "INT" && tree->children[5]->children[0]->rule.size() == 2) {
            procedureTable[tree->rule[0]].first.push_back(" : int");
        } else {
            procedureTable[tree->rule[0]].first.push_back(" : int*");
        }

    } else if (tree->rule[0] == "procedure") {
        if (procedureTable.count(tree->children[1]->rule[1])) {
            std::cerr << "ERROR duplicate procedure\n";
            return 1;
        }

        curProcedure = tree->children[1]->rule[1];
        Node* tempNode = tree->children[3].get();

        if (tempNode->rule[1] != ".EMPTY") {
            tempNode = tempNode->children[0].get();
            while (true) {
                if (tempNode->children[0]->children[0]->rule.size() > 2) {
                    procedureTable[curProcedure].first.push_back(" : int*");
                } else {
                    procedureTable[curProcedure].first.push_back(" : int");
                }

                if (tempNode->rule.size() == 2) {
                    break;
                } else {
                    tempNode = tempNode->children[2].get();
                }
            }
        }
        procedureTable[tree->children[1]->rule[1]];

    } else if (tree->rule[0] == "dcl") {
        if (procedureTable[curProcedure].second.count(tree->children[1]->rule[1])) {
            std::cerr << "ERROR dcl, duplicate ID\n";
            return 1; 
        } else if (tree->children[0]->rule.size() > 2) {
            procedureTable[curProcedure].second[tree->children[1]->rule[1]] = "int*";
        } else {
            procedureTable[curProcedure].second[tree->children[1]->rule[1]] = "int";
        }
        tree->children[1]->type = " : " + procedureTable[curProcedure].second[tree->children[1]->rule[1]];

    } else if (tree->rule[0] == "dcls" && tree->children.size() == 5 && tree->rule[3] == "BECOMES") {
        if ((tree->children[1]->children[0]->rule[1] == "INT" && tree->children[1]->children[0]->rule.size() == 2 && tree->rule[4] == "NULL") || 
            (tree->children[1]->children[0]->rule[1] == "INT" && tree->children[1]->children[0]->rule.size() == 3 && tree->rule[4] == "NUM")) {
            std::cerr << "ERROR dcls, types not matching\n";
            return 1;
        }
    }

    for (const std::unique_ptr<Node>& child : tree->children) {
        if (typeError(child)) {
            return 1;
        }
    }

    if (tree->rule[0] == "NUM") {
        tree->type = " : int";

    } else if (tree->rule[0] == "NULL") {
        tree->type = " : int*";

    } else if (tree->rule[0] == "ID") {
        if (procedureTable[curProcedure].second.count(tree->rule[1])) {
            tree->type = " : " + procedureTable[curProcedure].second[tree->rule[1]];
        } else if (procedureTable.count(tree->rule[1])) {
            tree->type = "";
        } else {
            std::cerr << "ERROR ID, not declared\n";
            return 1;
        }

    } else if (tree->rule[0] == "expr") {
        if (tree->rule.size() == 2 && tree->rule[1] == "term") {
            tree->type = tree->children[0]->type;
        } else if (tree->rule.size() == 4 && (tree->rule[2] == "PLUS" || tree->rule[2] == "MINUS")) {
            if (tree->children[0]->type == " : int" && tree->children[2]->type == " : int") {
                tree->type = " : int";
            } else if (tree->children[0]->type == " : int*" && tree->children[2]->type == " : int") {
                tree->type = " : int*";
            } else if (tree->children[0]->type == " : int" && tree->children[2]->type == " : int*" && tree->rule[2] == "PLUS") {
                tree->type = " : int*";
            } else if (tree->children[0]->type == " : int*" && tree->children[2]->type == " : int*" && tree->rule[2] == "MINUS") {
                tree->type = " : int";
            } else {
                std::cerr << "ERROR expr -> expr PLUS/MINUS term, expr and term have some int or int* problem\n";
                return 1;
            }
        } else {
            std::cerr << "ERROR expr, term issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "term") {
        if (tree->rule.size() == 2 && tree->rule[1] == "factor") {
            tree->type = tree->children[0]->type;
        } else if (tree->rule.size() == 4 && (tree->rule[2] == "STAR" || tree->rule[2] == "SLASH" || tree->rule[2] == "PCT")) {
            if (tree->children[0]->type == " : int" && tree->children[2]->type == " : int") {
                tree->type = " : int";
            } else {
                std::cerr << "ERROR term -> term STAR/SLASH/PCT factor, term and/or factor is not an int\n";
                return 1;
            }
        } else {
            std::cerr << "ERROR term, factor issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "factor") {
        if (tree->rule.size() == 2 && (tree->rule[1] == "NUM" || tree->rule[1] == "NULL" || tree->rule[1] == "ID")) {
            tree->type = tree->children[0]->type;
        } else if (tree->rule.size() == 4 && tree->rule[1] == "LPAREN") {
            tree->type = tree->children[1]->type;
        } else if (tree->rule.size() == 3 && tree->rule[1] == "AMP") {
            if (tree->children[1]->type == " : int") {
                tree->type = " : int*";
            } else {
                std::cerr << "ERROR factor -> AMP lvalue, lvalue is not an int\n";
                return 1;
            }
        } else if (tree->rule.size() == 3 && tree->rule[1] == "STAR") {
            if (tree->children[1]->type == " : int*") {
                tree->type = " : int";
            } else {
                std::cerr << "ERROR factor -> STAR factor, factor is not an int*\n";
                return 1;
            }
        } else if (tree->rule.size() == 6 && tree->rule[1] == "NEW") {
            if (tree->children[3]->type == " : int") {
                tree->type = " : int*";
            } else {
                std::cerr << "ERROR factor -> NEW INT LBRACK expr RBRACK, expr is not an int\n";
                return 1;
            }
        } else if (tree->rule.size() == 4 && tree->rule[1] == "ID") {
            if (!tree->children[0]->type.empty() || !procedureTable.count(tree->children[0]->rule[1])) {
                std::cerr << "ERROR factor -> ID LPAREN RPAREN, ID is not a procedure\n";
                return 1;
            } else if (!procedureTable[tree->children[0]->rule[1]].first.empty()) {
                std::cerr << "ERROR factor -> ID LPAREN RPAREN, ID is a procedure with a non empty signature\n";
                return 1;
            } else {
                tree->type = " : int";
            }
        } else if (tree->rule.size() == 5 && tree->rule[1] == "ID") {
            if (!tree->children[0]->type.empty() || !procedureTable.count(tree->children[0]->rule[1])) {
                std::cerr << "ERROR factor -> ID LPAREN RPAREN, ID is not a procedure\n";
                return 1;
            } else if (procedureTable[tree->children[0]->rule[1]].first.empty()) {
                std::cerr << "ERROR factor -> ID LPAREN RPAREN, ID is a procedure with a empty signature\n";
                return 1;
            } else if (procedureTable[tree->children[0]->rule[1]].first != tree->children[2]->arglist) {
                std::cerr << "ERROR factor -> ID LPAREN RPAREN, ID is a procedure with a empty signature\n";
                return 1;
            } else {
                tree->type = " : int";
            }
        } else if (tree->rule.size() == 4 && tree->rule[1] == "GETCHAR") {
            tree->type = " : int";
        } else {
            std::cerr << "ERROR factor, either NUM, NULL, ID, or LPAREN issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "lvalue") {
        if (tree->rule.size() == 2 && tree->rule[1] == "ID") {
            tree->type = tree->children[0]->type;
        } else if (tree->rule.size() == 3 && tree->rule[1] == "STAR") {
            if (tree->children[1]->type == " : int*") {
                tree->type = " : int";
            } else {
                std::cerr << "ERROR lvalue -> STAR factor, factor is not an int*\n";
                return 1;
            }
        } else if (tree->rule.size() == 4 && tree->rule[1] == "LPAREN") {
            tree->type = tree->children[1]->type;
        } else {
            std::cerr << "ERROR lvalue, ID or LPAREN issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "arglist") {
        if (tree->rule.size() == 2 && tree->rule[1] == "expr") {
            tree->arglist.push_back(tree->children[0]->type);
        } else if (tree->rule.size() == 4 && tree->rule[1] == "expr") {
            tree->arglist = tree->children[2]->arglist;
            tree->arglist.insert(tree->arglist.begin(), tree->children[0]->type);
        } else {
            std::cerr << "ERROR lvalue, ID or LPAREN issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "statement") {
        if (tree->rule.size() == 5 && tree->rule[1] == "lvalue") {
            if (tree->children[0]->type != tree->children[2]->type) {
                std::cerr << "ERROR lvalue statement, lvalue and expr do not share a type\n";
                return 1;
            }
        } else if (tree->rule.size() == 6 && tree->rule[1] == "PRINTLN") {
            if (tree->children[2]->type != " : int") {
                std::cerr << "ERROR PRINTLN statement, expr is not an int\n";
                return 1;
            }
        } else if (tree->rule.size() == 6 && tree->rule[1] == "PUTCHAR") {
            if (tree->children[2]->type != " : int") {
                std::cerr << "ERROR PUTCHAR statement, expr is not an int\n";
                return 1;
            }
        } else if (tree->rule.size() == 6 && tree->rule[1] == "DELETE") {
            if (tree->children[3]->type != " : int*") {
                std::cerr << "ERROR DELETE statement, expr is not an int\n";
                return 1;
            }
        } else if (tree->rule[1] == "IF" || tree->rule[1] == "WHILE") {
            // hi, I hope this program works
        } else {
            std::cerr << "ERROR statement, some issue\n";
            return 1;
        }

    } else if (tree->rule[0] == "test") {
        if (tree->children[0]->type != tree->children[2]->type) {
            std::cerr << "ERROR test, exprs are not equal\n";
            return 1;
        }
    }

    if (tree->rule[0] == "main") {
        if (tree->children[11]->type != " : int") {
            std::cerr << "ERROR main RETURN expr is not an int\n";
            return 1;
        }
    } else if (tree->rule[0] == "procedure") {
        if (tree->children[9]->type != " : int") {
            std::cerr << "ERROR procedure RETURN expr is not an int\n";
            return 1;
        }
    }

    return 0;
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

    // processes wlpi tree
    std::unique_ptr<Node> wlp4iTree = wlp4iParse(input);
    if (wlp4iTree == nullptr) {
        std::cerr << "ERROR wlp4 not parsed correctly\n";
        return 1;
    }

    // checks for semantic errors
    if (typeError(wlp4iTree)) {
        return 1;
    }

    // self explanatory print
    print(wlp4iTree);
    return 0;
}
