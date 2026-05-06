#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include "wlp4data.h"

struct Node {
    std::vector<std::string> value;
    std::vector<std::unique_ptr<Node>> children;
};
 
void print(const std::unique_ptr<Node>& tree) {
    for (size_t i = 0; i < tree->value.size(); i++) {
        if (i != 0) {
            std::cout << " ";
        }
        std::cout << tree->value[i];
    }
    std::cout << "\n";
    for (const std::unique_ptr<Node>& child : tree->children) {
        print(child);
    }
}

int main() {
    std::istream& in = std::cin;
    std::string s;

    std::vector<std::pair<std::string, std::vector<std::string>>> cfgRules; 
    std::map<int, std::map<std::string, int>> cfgTransitions; 
    std::map<int, std::map<std::string, int>> cfgReductions; 
    std::vector<std::string> inputLexemes = {"BOF"};
    std::vector<std::string> input = {"BOF"};
    size_t symbolsRead = 1;
    size_t curState = 0;
    std::vector<int> stateStack;
    std::vector<std::unique_ptr<Node>> symbolStack;

    std::string cfg = WLP4_CFG;
    std::string transitions = WLP4_TRANSITIONS;
    std::string reductions = WLP4_REDUCTIONS;
    std::string tables = WLP4_COMBINED;

    // CFG Component
    std::istringstream iss(cfg);
    std::getline(iss, s); // skip CFG header R"END(.CFG
    while (std::getline(iss, s)) {
        if (s == ")END") {
            break;
        }
        bool lhsSymbol = true;
        bool rhsSymbol = false;
        std::string lhs;
        std::vector<std::string> rhs;
        std::string temp;

        while (rhsSymbol == false) {
            if (lhsSymbol && s[0] == ' ') {
                lhsSymbol = false;
            } else if (!lhsSymbol && s[0] == ' ') {
                continue;
            } else if (!lhsSymbol && s[0] != ' ') {
                break;
            } else {
                lhs.push_back(s[0]);
            }
            s.erase(s.begin());
        }
        
        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == ' ') {
                if (!temp.empty()) {
                    rhs.push_back(temp);
                    temp = "";
                }
            } else if (i == s.length() - 1) {
                temp.push_back(s[i]);
                rhs.push_back(temp);
                temp = "";
            } else {
                temp.push_back(s[i]);
            }
        }

        cfgRules.push_back({lhs, rhs});
    }

    // TRANSITIONS Component
    iss.str(transitions);
    iss.clear();
    std::getline(iss, s); // skip transition header R"END(.TRANSITIONS
    while (std::getline(iss, s)) {
        if (s == ")END") {
            break;
        }
        int from = -1;
        std::string symbol;
        int to = 0;

        while (true) {
            if (s[0] == ' ') {
                if (from == -1) {
                    s.erase(s.begin());
                } else {
                    break;
                }
            } else {
                if (from == -1) {
                    from = 0;
                }
                from *= 10;
                from += (s[0] - '0');
                s.erase(s.begin());
            }
        }

        while (true) {
            if (s[0] == ' ') {
                if (symbol.empty()) {
                    s.erase(s.begin());
                } else {
                    break;
                }
            } else {
                symbol.push_back(s[0]);
                s.erase(s.begin());
            }
        }

        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == ' ') {
                continue;
            } else {
                to *= 10;
                to += (s[i] - '0');
            }
        }

        cfgTransitions[from][symbol] = to;
    }

    // REDUCTIONS Component
    iss.str(reductions);
    iss.clear();
    std::getline(iss, s); // skip reductions header R"END(.REDUCTIONS
    while (std::getline(iss, s)) {
        if (s == ")END") {
            break;
        }
        int state = -1;
        int rule = -1;
        std::string tag;

        while (true) {
            if (s[0] == ' ') {
                if (state == -1) {
                    s.erase(s.begin());
                } else {
                    break;
                }
            } else {
                if (state == -1) {
                    state = 0;
                }
                state *= 10;
                state += (s[0] - '0');
                s.erase(s.begin());
            }
        }

        while (true) {
            if (s[0] == ' ') {
                if (rule == -1) {
                    s.erase(s.begin());
                } else {
                    break;
                }
            } else {
                if (rule == -1) {
                    rule = 0;
                }
                rule *= 10;
                rule += (s[0] - '0');
                s.erase(s.begin());
            }
        }

        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == ' ') {
                if (tag.empty()) {
                    continue;
                } else {
                    break;
                }
            } else {
                tag.push_back(s[i]);
            }
        }

        cfgReductions[state][tag] = rule;
    }

    // Input Component
    while(std::getline(in, s)) {
        std::string temp;
        while (s[0] != ' ') {
            temp.push_back(s[0]);
            s.erase(s.begin());
        }
        s.erase(s.begin());
        input.push_back(temp);
        inputLexemes.push_back(s);
    }
    input.push_back("EOF");
    inputLexemes.push_back("EOF");

    // Output Component
    stateStack.push_back(0);
    for (size_t lookahead = 0; lookahead <= input.size(); lookahead++) {
        std::string lookaheadSymbol;
        if (lookahead == input.size()) {
            lookaheadSymbol = ".ACCEPT";
        } else {
            lookaheadSymbol = input[lookahead];
        }

        while (true) {
            curState = stateStack.back();
            if (cfgReductions[curState].count(lookaheadSymbol)) {
                int reductionRule = cfgReductions[curState][lookaheadSymbol];
                std::vector<std::unique_ptr<Node>> children;
                if (!(cfgRules[reductionRule].second.size() == 1 && cfgRules[reductionRule].second[0] == ".EMPTY")) {
                    children.resize(cfgRules[reductionRule].second.size());
                    for (int i = static_cast<int>(cfgRules[reductionRule].second.size()) - 1; i >= 0; i--) {
                        children[i] = std::move(symbolStack.back());
                        symbolStack.pop_back();
                        stateStack.pop_back();
                    }
                }
                std::vector<std::string> nodeValue;
                nodeValue.push_back(cfgRules[reductionRule].first);
                nodeValue.insert(nodeValue.end(), cfgRules[reductionRule].second.begin(), cfgRules[reductionRule].second.end());
                symbolStack.push_back(std::make_unique<Node>(nodeValue, std::move(children)));
                stateStack.push_back(cfgTransitions[stateStack.back()][cfgRules[reductionRule].first]);
                if (lookaheadSymbol == ".ACCEPT") {
                    print(symbolStack.back());
                    return 0;
                }
            } else if (cfgTransitions[curState].count(lookaheadSymbol)) {
                break;
            } else {
                std::cerr << "ERROR at " << symbolsRead << "\n";
                return 1;
            }
        }
        if (cfgTransitions[stateStack.back()].count(lookaheadSymbol)) {
            std::vector<std::string> nodeValue = {input[lookahead], inputLexemes[lookahead]};
            symbolStack.push_back(std::make_unique<Node>(nodeValue));
            stateStack.push_back(cfgTransitions[stateStack.back()][lookaheadSymbol]);
            if (input[lookahead] != "BOF" && input[lookahead] != "EOF") {
                symbolsRead++;
            }
        } else {
            std::cerr << "ERROR at " << symbolsRead << "\n";
            return 1;
        }
    }
}
