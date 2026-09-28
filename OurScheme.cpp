#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <memory>
#include <functional>
using namespace std;

#define DEBUG_MODE false

// Token 類型
enum TokenType {
    LEFT_PAREN,
    RIGHT_PAREN,
    INT,
    FLOAT,
    STRING,
    DOT,
    NIL,
    T,
    QUOTE,
    SYMBOL,
    NONE,
    END_OF_FILE,
    EXIT
};

enum ErrorType {
    NO_ERROR,
    UNBOUND_SYMBOL,
    INCORRECT_ARGS,
    TYPE_ERROR,
    DIVISION_BY_ZERO,
    DEFINE_FORMAT,
    NO_RETURN_VALUE,
    ATTEMPT_TO_APPLY_NON_FUNCTION,
    NON_LIST,
    COND_FORMAT
};

class Node;
typedef shared_ptr<Node> NodePtr;

struct ErrorHandler {
    bool hasError;
    string errorType;
    string errorMessage;
    string errorFuncName;
    NodePtr errorExpr;  // 新增：用來儲存引發錯誤的表達式
    
    // 建構函數
    ErrorHandler() : hasError(false), errorExpr(nullptr) {}
    
    // 設定錯誤，新增參數用於傳遞問題表達式
    void SetError(const string& type, const string& message, const string& funcName = "", NodePtr errorNode = nullptr) {
        hasError = true;
        errorType = type;
        errorMessage = message;
        errorFuncName = funcName;
        errorExpr = errorNode; 
    }
    

    // 清除錯誤
    void ClearError() {
        hasError = false;
        errorType = "";
        errorMessage = "";
        errorFuncName = "";
        errorExpr = nullptr;  // 清除問題表達式
    }
    
    // 檢查是否有錯誤
    bool HasError() const {
        return hasError;
    }
    
    // 取得錯誤訊息，修改為包含問題表達式
    string GetErrorMessage() const {
        if (!hasError) return "";
    
        // 處理 "level of" 相關錯誤
        if (errorType.find("level of") == 0) {
            return errorType + ")";
        }
        
        // 處理 "incorrect number of arguments" 錯誤
        if (errorType == "incorrect number of arguments" && !errorFuncName.empty()) {
            return "incorrect number of arguments) : " + errorFuncName;
        }
        
        // 處理 "with incorrect argument type" 錯誤
        if (errorType.find("with incorrect argument type") != string::npos) {
            string result = errorType;
            // 確保有右括號
            if (result.find(")") == string::npos) {
                result += ")";
            }
            
            // 如果有錯誤訊息，添加它
            if (!errorMessage.empty()) {
                result += " : " + errorMessage;
            }
            return result;
        }
        
        // 處理其他錯誤類型
        string msg = errorType;
        
        // 確保有右括號
        if (msg.find(")") == string::npos) {
            msg += ")";
        }
        
        // 加入函數名或錯誤訊息
        if (!errorFuncName.empty()) {
            msg += " : " + errorFuncName;
        } else if (!errorMessage.empty() || errorExpr) {
            // 對於特定錯誤類型，添加錯誤訊息
            if (errorType == "attempt to apply non-function" || 
                errorType == "car with incorrect argument type" ||
                errorType == "cdr with incorrect argument type" ||
                errorType == "+ with incorrect argument type" ||
                errorType == "* with incorrect argument type" ||
                errorType == "string>? with incorrect argument type" ||
                errorType == "string<? with incorrect argument type" ||
                errorType == "string-append with incorrect argument type" ||
                errorType == "> with incorrect argument type" ||
                errorType == ">= with incorrect argument type") {
                
                if (errorExpr && errorType == "attempt to apply non-function") {
                    // 對於 attempt to apply non-function 錯誤，如果有錯誤表達式，直接返回
                    // 主函數會負責顯示完整的錯誤表達式
                    msg += " : ";
                    // 不添加錯誤訊息，將在主函數中處理
                    return msg;
                } else if (!errorMessage.empty()) {
                    msg += " : " + errorMessage;
                }
            }
        }
        
        return msg;
    }

    NodePtr GetErrorExpr() const {
        return errorExpr;
    }
};

string TokenTypeToString(TokenType type) {
    if (type == LEFT_PAREN) return "LEFT_PAREN";
    else if (type == RIGHT_PAREN) return "RIGHT_PAREN";
    else if (type == INT) return "INT";
    else if (type == FLOAT) return "FLOAT";
    else if (type == STRING) return "STRING";
    else if (type == DOT) return "DOT";
    else if (type == NIL) return "NIL";
    else if (type == T) return "T";
    else if (type == QUOTE) return "QUOTE";
    else if (type == SYMBOL) return "SYMBOL";
    else if (type == NONE) return "NONE";
    else if (type == END_OF_FILE) return "END_OF_FILE";
    else if (type == EXIT) return "EXIT";
    else return "UNKNOWN";
}
// Token 結構
struct Token {
    TokenType type;
    string value;
    int row = 0;
    int column = 0;
};

// 節點類型
enum NodeType {
    ATOM_NODE,   // 原子節點
    CONS_NODE    // 列表節點
};

class Node;
typedef shared_ptr<Node> NodePtr;

class Node {
    public:
    virtual ~Node() {}
    virtual NodeType GetType() const = 0;
    virtual void Print(int indent = 0, int baseIndent = 0) const = 0;
    virtual NodePtr Clone() const = 0;
};

// 原子節點
class AtomNode : public Node {
    private:
    TokenType atomType;
    string value;

    public:
    // 建構函式
    AtomNode(TokenType type, const string& val) : atomType(type), value(val) {}

    // 取得節點類型
    NodeType GetType() const override { 
        return ATOM_NODE; 
    }

    // 取得原子類型
    TokenType GetAtomType() const { 
        return atomType; 
    }

    // 取得原子值
    const string& GetValue() const { 
        return value; 
    }

    // 列印原子節點
    void Print(int indent = 0, int baseIndent = 0) const override {
        if (atomType == SYMBOL) {
            if (value.find("#<procedure lambda:") == 0) {
                cout << "#<procedure lambda>";
                return;
            }
        }
        
        // 原子節點不需要縮排
        if (atomType == INT) {
            // 移除正號
            if (value[0] == '+') {
                cout << value.substr(1);
            }
            else {
                cout << value;
            }
        }
        else if (atomType == FLOAT) {
            try {
                string floatStr = value;
                
                // 移除加號
                if (floatStr[0] == '+') {
                    floatStr = floatStr.substr(1);
                }
                
                // 處理沒有整數部分的浮點數，如 ".123"
                if (floatStr[0] == '.' && floatStr.length() > 1) {
                    floatStr = "0" + floatStr;  // 將 ".123" 改為 "0.123"
                }
                
                // 處理沒有小數部分的浮點數，如 "123."
                if (floatStr.back() == '.' && floatStr.length() > 1) {
                    floatStr += "0";  // 將 "123." 改為 "123.0"
                }
                
                // 處理特殊案例 "." 或空字串
                if (floatStr == "." || floatStr.empty()) {
                    cout << value;
                }
                else {
                    // 格式化為 3 位小數
                    float val = stof(floatStr);
                    ostringstream out;
                    out << fixed << setprecision(3) << val;
                    cout << out.str();
                }
            } 
            catch (const exception& e) {
                cout << value;
            }
        }
        else if (atomType == NIL) {
            cout << "nil";
        }
        else if (atomType == T) {
            cout << "#t";
        }
        else if (atomType == STRING) {
            cout << "\"" << value << "\"";
        }
        else {
            cout << value;
        }
        
    }

    // 複製節點
    NodePtr Clone() const override {
        return make_shared<AtomNode>(atomType, value);
    }
};

// 列表節點 (Cons 節點)
class ConsNode : public Node {
private:
    NodePtr left;    // 左子節點
    NodePtr right;   // 右子節點
    bool isDotted;   // 是否為點對
    
public:
    // 建構函式
    ConsNode(NodePtr l, NodePtr r, bool dotted = false) 
        : left(l), right(r), isDotted(dotted) {}
    
    // 取得節點類型
    NodeType GetType() const override { 
        return CONS_NODE; 
    }
    
    // 取得左子節點
    NodePtr GetLeft() const { 
        return left; 
    }
    
    // 取得右子節點
    NodePtr GetRight() const { 
        return right; 
    }
    
    // 檢查是否為點對
    bool IsDotted() const { 
        return isDotted; 
    }
    
    // 判斷節點是否為 nil
    bool IsNilNode(NodePtr node) const {
        if (!node) {
            return false;
        }
        
        if (node->GetType() == ATOM_NODE) {
            auto atom = dynamic_pointer_cast<AtomNode>(node);
            if (atom && atom->GetAtomType() == NIL) {
                return true;
            }
        }
        
        return false;
    }
    void Print(int indent = 0, int baseIndent = 0) const override {
        // 保存左括號的縮進位置
        int leftBracketIndent = baseIndent;
        
        // 打印左括號
        cout << "(";
        
        // 打印第一個元素
        if (left) {
            cout << " ";
            left->Print(indent + 2, indent + 2);
        }
        
        // 判斷是基本點對還是一般列表/嵌套點對
        if (isDotted && right && right->GetType() != CONS_NODE) {
            // 基本點對：點號後是原子值，如 (3 . 5)
            // 直接打印點號和右值
            cout << endl << string(indent + 2, ' ') << "." << endl;
            cout << string(indent + 2, ' ');
            right->Print(indent + 2, 0);
        } 
        else {
            // 處理普通列表或嵌套點對
            printListOrDottedPair(right, indent + 2);
        }
        
        // 打印右括號 - 使用與左括號相同的縮進
        cout << endl << string(leftBracketIndent, ' ') << ")";
    }
    
    // 綜合處理列表和嵌套點對的方法
    void printListOrDottedPair(NodePtr current, int indent) const {
        // 如果為空或不是 cons 節點，直接返回
        if (!current || current->GetType() != CONS_NODE) return;
        
        while (current && current->GetType() == CONS_NODE) {
            auto currentCons = dynamic_pointer_cast<ConsNode>(current);
            if (!currentCons) break;
            
            // 打印當前元素
            if (currentCons->GetLeft()) {
                cout << endl << string(indent, ' ');
                
                // 特殊處理嵌套列表
                if (currentCons->GetLeft()->GetType() == CONS_NODE) {
                    currentCons->GetLeft()->Print(indent, indent);
                } else {
                    currentCons->GetLeft()->Print(indent, 0);
                }
            }
            
            // 處理點對情況
            if (currentCons->IsDotted()) {
                NodePtr rightValue = currentCons->GetRight();
                
                // 如果點號後是 nil，不顯示點號和 nil
                if (IsNilNode(rightValue)) {
                    //cout << "DEBUG";
                    break;
                }
                
                // 如果點號後是另一個 cons，嘗試以列表形式處理
                if (rightValue && rightValue->GetType() == CONS_NODE) {
                    auto rightCons = dynamic_pointer_cast<ConsNode>(rightValue);
                    if (rightCons) {
                        // 繼續處理嵌套的點對
                        current = rightValue;
                        continue;
                    }
                }
                
                // 點號後不是 cons 也不是 nil，顯示點號和值
                cout << endl << string(indent, ' ') << "." << endl;
                cout << string(indent, ' ');
                rightValue->Print(indent, 0);
                break;
            }
            
            // 處理標準列表情況
            current = currentCons->GetRight();
            
            // 檢查是否到達列表末尾
            if (IsNilNode(current)) {
                break;  // 到達 nil，結束列表處理
            }
        }
    }
    
    // 處理點對後的列表
    void processDottedList(shared_ptr<ConsNode> dotCons, int indent) const {
        if (!dotCons) {
            return;
        }
        
        // 列印左括號
        cout << "(";
        
        // 處理第一個元素
        if (dotCons->GetLeft()) {
            cout << " ";
            dotCons->GetLeft()->Print(indent + 2, 0);
        }
        
        // 處理點對或其他元素
        if (dotCons->IsDotted()) {
            cout << endl << string(indent + 2, ' ') << "." << endl;
            cout << string(indent + 2, ' ');
            
            // 特別處理點對後面是嵌套列表的情況
            if (dotCons->GetRight() && dotCons->GetRight()->GetType() == CONS_NODE) {
                auto nestedCons = dynamic_pointer_cast<ConsNode>(dotCons->GetRight());
                if (nestedCons) {
                    processNestedInDot(nestedCons, indent + 2);
                }
                else if (dotCons->GetRight()) {
                    dotCons->GetRight()->Print(indent + 2, 0);
                }
            }
            else if (dotCons->GetRight()) {
                dotCons->GetRight()->Print(indent + 2, 0);
            }
        }
        else {
            // 處理一般列表元素
            NodePtr current = dotCons->GetRight();
            
            while (current && current->GetType() == CONS_NODE) {
                auto currentCons = dynamic_pointer_cast<ConsNode>(current);
                if (!currentCons) {
                    break;
                }
                
                if (currentCons->GetLeft()) {
                    cout << endl << string(indent + 2, ' ');
                    
                    // 判斷是否為嵌套列表
                    if (currentCons->GetLeft()->GetType() == CONS_NODE) {
                        auto nestedCons = dynamic_pointer_cast<ConsNode>(currentCons->GetLeft());
                        if (nestedCons) {
                            processNestedInDot(nestedCons, indent + 2);
                        }
                    }
                    else {
                        currentCons->GetLeft()->Print(indent + 2, 0);
                    }
                }
                
                if (currentCons->IsDotted()) {
                    NodePtr rightVal = currentCons->GetRight();
                    if (rightVal && !IsNilNode(rightVal)) {
                        cout << endl << string(indent + 2, ' ') << "." << endl;
                        cout << string(indent + 2, ' ');
                        rightVal->Print(indent + 2, 0);
                    }
                    break;
                }
                
                current = currentCons->GetRight();
            }
        }
        
        // 右括號與左括號對齊
        cout << endl << string(indent, ' ') << ")";
    }
    void processNestedInDot(shared_ptr<ConsNode> nestedCons, int indent) const {
        if (!nestedCons) {
            return;
        }
        
        // 列印左括號
        cout << "(";
        
        // 處理第一個元素
        if (nestedCons->GetLeft()) {
            cout << " ";
            nestedCons->GetLeft()->Print(indent + 2, 0);
        }
        
        // 處理點對或其他元素
        if (nestedCons->IsDotted()) {
            cout << endl << string(indent + 2, ' ') << "." << endl;
            cout << string(indent + 2, ' ');
            
            if (nestedCons->GetRight()) {
                nestedCons->GetRight()->Print(indent + 2, 0);
            }
        }
        else {
            // 處理列表元素
            NodePtr current = nestedCons->GetRight();
            
            while (current && current->GetType() == CONS_NODE) {
                auto currentCons = dynamic_pointer_cast<ConsNode>(current);
                if (!currentCons) {
                    break;
                }
                
                if (currentCons->GetLeft()) {
                    cout << endl << string(indent + 2, ' ');
                    currentCons->GetLeft()->Print(indent + 2, 0);
                }
                
                if (currentCons->IsDotted()) {
                    NodePtr rightVal = currentCons->GetRight();
                    if (rightVal && !IsNilNode(rightVal)) {
                        cout << endl << string(indent + 2, ' ') << "." << endl;
                        cout << string(indent + 2, ' ');
                        rightVal->Print(indent + 2, 0);
                    }
                    break;
                }
                
                current = currentCons->GetRight();
            }
        }
        
        // 右括號與左括號對齊
        cout << endl << string(indent, ' ') << ")";
    }
    // 複製節點
    NodePtr Clone() const override {
        NodePtr newLeft = left ? left->Clone() : nullptr;
        NodePtr newRight = right ? right->Clone() : nullptr;
        return make_shared<ConsNode>(newLeft, newRight, isDotted);
    }
};

// 節點標準化類別
class NodeNormalizer {
private:
    // 判斷節點是否為 nil
    static bool IsNilNode(NodePtr node) {
        if (!node) {
            return false;
        }
        
        if (node->GetType() == ATOM_NODE) {
            auto atom = dynamic_pointer_cast<AtomNode>(node);
            if (atom && atom->GetAtomType() == NIL) {
                return true;
            }
        }
        
        return false;
    }

    // 創建原子節點
    static NodePtr CreateAtomNode(TokenType type, const string& value) {
        return make_shared<AtomNode>(type, value);
    }

    // 創建Cons節點
    static NodePtr CreateConsNode(NodePtr left, NodePtr right, bool isDotted) {
        return make_shared<ConsNode>(left, right, isDotted);
    }
     
public:

    static bool IsListNode(NodePtr node) {

        if (!node) {
            return false;
        }
        
        if (node->GetType() == ATOM_NODE) {
            auto atom = dynamic_pointer_cast<AtomNode>(node);
            if (atom && atom->GetAtomType() == NIL) {
                return true;
            }
            return false;
        }
        
        if (node->GetType() == CONS_NODE) {
            auto cons = dynamic_pointer_cast<ConsNode>(node);
            if (!cons) {
                return false;
            }
            
            if (!IsNilNode(cons->GetRight())) {

                return IsListNode(cons->GetRight());
            } else {

                return true;
            }
        }
        
        return false;
    }
};

class Scanner {
    private:
        char currentChar, peekChar;
        int currentRow = 1;
        int currentColumn = 0;
        
        string numtostr;
        bool isEOF = false;
    
        // 取得下一個字元
        void GetNextChar() { 
            if (cin.eof()) {
                isEOF = true;
                return;
            }
            
            currentChar = cin.get();
            
            if (cin.eof()) {
                isEOF = true;
                return;
            }
            
            peekChar = cin.peek();
            
            if (currentChar == '\n') {
                currentRow++;
                currentColumn = 0;
            } 
            else {
                currentColumn++;
            }
        }
    
        // 取得數字記號
        Token GetNumber() {
            Token token;
            token.row = currentRow;
            token.column = currentColumn;
            string numStr = "";
            string digits = "";
            bool isFloat = false;
    
            // 處理正負號
            if (currentChar == '+' || currentChar == '-') {
                numStr += currentChar;
                while (isdigit(peekChar)) {
                    GetNextChar();
                    digits += currentChar;
                }
            }
            // 處理數字
            else if (isdigit(currentChar) && !isEOF) {
                digits += currentChar;
                while (isdigit(peekChar)) {
                    GetNextChar();
                    digits += currentChar;
                }
            }
            numStr += digits;
        
            // 處理小數部分
            if (peekChar == '.') {
                GetNextChar();
                numStr += '.';
                isFloat = true;
                
                digits = "";
                
                if (isdigit(peekChar) && !isEOF) {
                    while (isdigit(peekChar)) {
                        GetNextChar();
                        digits += currentChar;
                    }
                }
                numStr += digits;
            }
            
            // 檢查數字後是否有非數字、非空白字元
            if (!isspace(peekChar) && !isEOF && 
                peekChar != '(' && peekChar != ')' && 
                peekChar != '\'' && peekChar != '"' && 
                peekChar != ';') {
                // 如果有，整個記號應視為符號
                return GetSymbol();
            }
            
            token.type = isFloat ? FLOAT : INT;
            token.value = numStr;
            
            return token;
        }
        
        // 取得字串記號
        Token GetString() {
            Token token;
            token.type = STRING;
            token.row = currentRow;
            token.column = currentColumn;
            string strValue = "";
    
            // 取得字元直到結束引號
            while (peekChar != '"' && !isEOF) { 
                if (peekChar == '\n' || peekChar == EOF || isEOF) {
                    // 字串未正確關閉
                    cout << "ERROR (no closing quote) : END-OF-LINE encountered at Line " 
                            << currentRow << " Column " << currentColumn + 1 << endl << endl;
                    token.type = NONE;
                    SkipToEndOfLine();
                    return token;
                }
                
                GetNextChar();
                
                // 處理跳脫序列
                if (currentChar == '\\') {
                    if (peekChar == 'n') {
                        GetNextChar();
                        strValue += '\n';
                    } 
                    else if (peekChar == 't') {
                        GetNextChar();
                        strValue += '\t';
                    } 
                    else if (peekChar == '\\') {
                        GetNextChar();
                        strValue += '\\';
                    } 
                    else if (peekChar == '"') {
                        GetNextChar();
                        strValue += '"';
                    } 
                    else {
                        // 其他字元在\後保持原樣
                        strValue += '\\';
                        strValue += peekChar;
                        GetNextChar();
                    }
                } 
                else {
                    strValue += currentChar;
                }
            }
    
            // 取得結束引號
            if (peekChar == '"') {
                GetNextChar();
            }
            
            token.value = strValue;
            
            if (DEBUG_MODE) {
                cout << "[DEBUG] GetString: " << TokenTypeToString(token.type) 
                        << " with value: " << token.value << endl;
            }
            
            return token;
        }
        
        // 取得符號記號
        Token GetSymbol() {
            Token token;
            token.type = SYMBOL;
            token.row = currentRow;
            token.column = currentColumn;
            string symValue = "";
    
            symValue += currentChar;
    
            // 取得字元直到空白或特殊字元
            while (!isspace(peekChar) && !isEOF && 
                    peekChar != '(' && peekChar != ')' && 
                    peekChar != '\'' && peekChar != '"' &&
                    peekChar != ';') {
                GetNextChar();
                symValue += currentChar;
            }
    
            // 檢查特殊符號
            if (symValue == "nil" || symValue == "#f") {
                token.type = NIL;
                token.value = "nil";
            } 
            else if (symValue == "t" || symValue == "#t") {
                token.type = T;
                token.value = "#t";
            } 
            else {
                token.value = symValue;
            }
            
            if (DEBUG_MODE) {
                cout << "[DEBUG] GetSymbol: " << TokenTypeToString(token.type) 
                        << " with value: " << token.value << endl;
            }
            
            return token;
        }
        
    public:
        int countline = 0, countspace = 0;
        bool completedSExp = false;
    
        // 初始化掃描器
        void Initialize() {
            currentRow = 1;
            currentColumn = 0;
            countline = 0;
            isEOF = false;
        }
    
        // 跳過至當前行結尾
        void SkipToEndOfLine() {
            if (isEOF || peekChar == EOF) {
                return;
            }
            
            while (peekChar != '\n' && peekChar != EOF && !cin.eof() && !isEOF) {
                GetNextChar();
                
                if (cin.eof() || isEOF) {
                    break;
                }
            }
            
            if (peekChar == '\n' && !isEOF) {
                GetNextChar();
            }
        }
    
        // 跳過空白和註解
        void SkipWhiteSpaceAndComment() {
            countspace = 0;
            int whitepluslinecomment = 0;
            bool allwhite = false;
            
            while (!isEOF && (isspace(currentChar) || currentChar == ';')) {
                if (isspace(currentChar)) { 
                    countspace++;
                    
                    if (currentChar != '\n' && peekChar == '\n') {
                        if (currentColumn == countspace && completedSExp) {
                            GetNextChar();
                            currentRow = 1;        
                            currentColumn = 0;   
                            completedSExp = false;
                        }
                    }
                    else if (currentChar == '\n' && completedSExp) {
                        currentRow = 1;        
                        currentColumn = 0; 
                        completedSExp = false;
                    }
                    
                    GetNextChar();
                    
                    // 若遇到 EOF，立即返回
                    if (isEOF) {
                        return;
                    }
                }
                else if (currentChar == ';') {
                    GetNextChar();
                    
                    // 處理註解
                    while (!isEOF && currentChar != '\n') {
                        GetNextChar();
                    }
                    
                    if (!isEOF && currentChar == '\n' && completedSExp) {
                        currentRow = 1;        
                        currentColumn = 0;
                        completedSExp = false;
                    }
                    
                    if (!isEOF) {
                        GetNextChar(); 
                    }
                }
            }
        }
    
        // 檢查是否已到文檔結尾
        bool IsEOF() const {
            return isEOF;
        }
        
        // 取得下一個記號
        Token GetToken() {
            Token token;
            token.row = currentRow;
            token.column = currentColumn;
            
            GetNextChar(); 
            
            // 檢查是否是 EOF
            if (isEOF) {
                token.type = END_OF_FILE;
                
                if (DEBUG_MODE) {
                    cout << "[DEBUG] GetToken: END_OF_FILE" << endl;
                }
                
                return token;
            }
            
            // 跳過空白和註解
            SkipWhiteSpaceAndComment();
            
            // 再次檢查 EOF
            if (isEOF) {
                token.type = END_OF_FILE;
                
                if (DEBUG_MODE) {
                    cout << "[DEBUG] GetToken: END_OF_FILE" << endl;
                }
                
                return token;
            }
            
            // 更新記號位置
            token.row = currentRow;
            token.column = currentColumn;
            
            if (currentChar == '(') {
                token.type = LEFT_PAREN;
                token.value = "(";
                token.row = currentRow;
                token.column = currentColumn;
                completedSExp = false;
    
                // 檢查空列表
                while (isspace(cin.peek())) {
                    GetNextChar();
                }
                
                if (peekChar == ')') {
                    GetNextChar();
                    token.type = NIL;
                    token.value = "nil";
                    token.row = currentRow;
                    token.column = currentColumn;
                }
    
                if (DEBUG_MODE) {
                    cout << "[DEBUG] GetToken: LEFT_PAREN" << endl;
                }
                
                return token;     
            } 
            else if (currentChar == ')') {
                token.type = RIGHT_PAREN;
                token.value = ")";
                token.row = currentRow;
                token.column = currentColumn;
                
                if (DEBUG_MODE) {
                    cout << "[DEBUG] GetToken: RIGHT_PAREN" << endl;
                }
    
                return token;
            } 
            else if (currentChar == '.') {
                if (isdigit(peekChar)) {
                    // 浮點數
                    string numStr = ".";
                    
                    while (isdigit(peekChar) && !isEOF) {
                        GetNextChar();
                        numStr += currentChar;
                    }
    
                    token.type = FLOAT;
                    token.value = numStr;
                    
                    // 檢查是否為符號
                    if (!isspace(peekChar) && !isEOF && 
                        peekChar != '(' && peekChar != ')' && 
                        peekChar != '\'' && peekChar != '"' && 
                        peekChar != ';') {
                        
                        while (!isspace(peekChar) && !isEOF && 
                                peekChar != '(' && peekChar != ')' && 
                                peekChar != '\'' && peekChar != '"' && 
                                peekChar != ';') {
                            GetNextChar();
                            numStr += currentChar;
                        }
    
                        token.type = SYMBOL;
                        token.value = numStr;
                    }
                    
                    if (DEBUG_MODE) {
                        cout << "[DEBUG] GetToken (dot): " << TokenTypeToString(token.type) 
                                << " with value: " << token.value << endl;
                    }
                    
                    return token;
                } 
                else if (isspace(peekChar) || peekChar == '(' || 
                            peekChar == ')' || peekChar == '\'' || 
                            peekChar == '"' || peekChar == ';' || peekChar == '\n') {
                    // 語法的點
                    token.type = DOT;
                    token.value = ".";
                    token.row = currentRow;
                    token.column = currentColumn;
    
                    if (DEBUG_MODE) {
                        cout << "[DEBUG] GetToken: DOT" << endl;
                    }
                    
                    return token;
                } 
                else {
                    // 符號
                    return GetSymbol();
                }
            } 
            else if (currentChar == '\'') {
                token.type = QUOTE;
                token.value = "quote";
                token.row = currentRow;
                token.column = currentColumn;
                
                if (DEBUG_MODE) {
                    cout << "[DEBUG] GetToken: QUOTE" << endl;
                }
                
                return token;
            } 
            else if (currentChar == '"') {
                return GetString();
            } 
            else if (currentChar == '.' || isdigit(currentChar) || 
                ((currentChar == '+' || currentChar == '-') && 
                (isdigit(peekChar) || peekChar == '.'))) {
    
                // 先取得整個記號
                string tokenStr = "";
                tokenStr += currentChar;
    
                // 繼續取得直到遇到空白或特殊符號
                while (!isspace(peekChar) && !isEOF && 
                    peekChar != '(' && peekChar != ')' && 
                    peekChar != '\'' && peekChar != '"' && 
                    peekChar != ';') {
                    GetNextChar();
                    tokenStr += currentChar;
                }
    
                // 特殊情況處理: '-.'、'+.' 等
                if (tokenStr == "-." || tokenStr == "+.") {
                    token.type = SYMBOL;
                    token.value = tokenStr;
                    return token;
                }
    
                // 嘗試判斷是否為有效數字
                bool isNumber = true;
                bool hasDot = false;
                bool hasDigit = false;
    
                for (size_t i = 0; i < tokenStr.length(); i++) {
                    char c = tokenStr[i];
                    
                    if (i == 0 && (c == '+' || c == '-')) {
                        // 允許開頭的正負號
                        continue;
                    } 
                    else if (c == '.') {
                        if (hasDot) {
                            isNumber = false;  // 多個小數點
                            break;
                        }
                        hasDot = true;
                    } 
                    else if (isdigit(c)) {
                        hasDigit = true;
                    } 
                    else {
                        isNumber = false;  // 含非數字字元
                        break;
                    }
                }
    
                // 必須包含數字才是有效數字
                if (!hasDigit) {
                    isNumber = false;
                }
    
                if (isNumber) {
                    // 是有效數字
                    if (hasDot) {
                        token.type = FLOAT;
                    } 
                    else {
                        token.type = INT;
                    }
                } 
                else {
                    // 是符號
                    token.type = SYMBOL;
                }
    
                token.value = tokenStr;
                return token;
            }
            else if (currentChar == '\'') {
                token.type = QUOTE;
                token.value = "quote";
                return token;
            }
            else if (currentChar == '"') {
                return GetString();
            }
            else {
                // 其他字元，可能是符號
                return GetSymbol();
            }
        }
    };


class Parser {
private:
    Scanner scanner;
    Token currentToken;
    bool dont_get = false;
    bool isTopLevelSExp = false;
    bool insideList = false; // 新增：追蹤是否在解析列表中間
    
    // 取得下一個記號
    void GetNextToken() {
        currentToken = scanner.GetToken(); 

        if (DEBUG_MODE) {
            cout << "[DEBUG] Parser got token: " << TokenTypeToString(currentToken.type) 
                    << " with value: " << currentToken.value << endl;
        }
    }
    
    // 跳過當前行
    void SkipToEndOfLine() {
        scanner.SkipToEndOfLine();
    }

    // 輸出錯誤信息
    void PrintError(const string& expected) {
        int adjustedColumn = currentToken.column;
        
        cout << "ERROR (unexpected token) : " << expected << " expected when token at Line "
                << currentToken.row << " Column " << adjustedColumn << " is >>"
                << currentToken.value << "<<" << endl << endl;
    }
    
    // 創建原子節點
    NodePtr CreateAtomNode(TokenType type, const string& value) {
        return make_shared<AtomNode>(type, value);
    }
    
    // 創建Cons節點
    NodePtr CreateConsNode(NodePtr left, NodePtr right, bool isDotted = false) {
        return make_shared<ConsNode>(left, right, isDotted);
    }
    
    // 解析原子
    NodePtr ParseAtom() {
        NodePtr atom = CreateAtomNode(currentToken.type, currentToken.value);
        if (DEBUG_MODE) {
            cout << "[DEBUG] ParseAtom: " << TokenTypeToString(currentToken.type) 
                    << " with value: " << currentToken.value << endl;
        }
        return atom;
    }
    
    // 解析列表
    NodePtr ParseList(bool isTopLevel = false) {
        // 記錄已進入列表解析
        bool oldInsideList = insideList;
        insideList = true;
        
        // 處理空列表
        if (currentToken.type == RIGHT_PAREN) {
            insideList = oldInsideList; // 恢復原狀態
            return CreateAtomNode(NIL, "nil");
        }
        
        // 解析第一個元素
        dont_get = true;
        NodePtr first = ParseExpr(false);
        if (first == nullptr) {
            insideList = oldInsideList; // 恢復原狀態
            return nullptr;
        }
        
        GetNextToken();
        
        // 檢查第一個元素是否為 exit
        bool isExitSymbol = false;
        if (first->GetType() == ATOM_NODE) {
            auto atom = dynamic_pointer_cast<AtomNode>(first);
            if (atom && atom->GetAtomType() == SYMBOL && atom->GetValue() == "exit") {
                isExitSymbol = true;
            }
        }
        
        // 點號處理
        if (currentToken.type == DOT) {

            GetNextToken();
            dont_get = true;
            NodePtr second = ParseExpr(false);
            if (second == nullptr) {
                insideList = oldInsideList; // 恢復原狀態
                return nullptr;
            }
            
            // 檢查點號後是否為 nil
            bool isNilValue = false;
            if (second->GetType() == ATOM_NODE) {
                auto nilAtom = dynamic_pointer_cast<AtomNode>(second);
                if (nilAtom && nilAtom->GetAtomType() == NIL) {
                    isNilValue = true;
                }
            }
            
            GetNextToken();
            if (currentToken.type != RIGHT_PAREN) {
                int adjustedColumn = currentToken.column;
                
                cout << "ERROR (unexpected token) : ')' expected when token at Line "
                    << currentToken.row << " Column " << adjustedColumn << " is >>"
                    << currentToken.value << "<<" << endl << endl;
                
                SkipToEndOfLine();
                insideList = oldInsideList; // 恢復原狀態
                return nullptr;
            }
            
            // 檢查是否為 (exit . nil) 且在頂層
            if (isTopLevel && isExitSymbol && isNilValue) {
                exit = true;
                cout << endl;
            }
            
            // 記錄為點對形式
            insideList = oldInsideList; // 恢復原狀態
            return CreateConsNode(first, second, true);
        } 
        else if (currentToken.type == RIGHT_PAREN) {
            // 檢查是否為 (exit) 且在頂層
            if (isTopLevel && isExitSymbol) {
                exit = true;
                cout << endl;
            }
            
            // 列表結束，連接 nil
            insideList = oldInsideList; // 恢復原狀態
            return CreateConsNode(first, CreateAtomNode(NIL, "nil"), false);
        } 
        else {
            // 繼續處理列表
            dont_get = true;
            NodePtr rest = ParseList(false);
            if (rest == nullptr) {
                insideList = oldInsideList; // 恢復原狀態
                return nullptr;
            }
            
            // 不記錄為點對形式
            insideList = oldInsideList; // 恢復原狀態
            
            // 如果是 exit 命令且在頂層，不要設置 exit 標誌，因為有額外參數
            // exit 命令應該沒有參數，所以這是一個錯誤情況
            
            return CreateConsNode(first, rest, false);
        }
    }
        
    // 解析S表達式
    NodePtr ParseExpr(bool isTopLevel = false) {
        if (DEBUG_MODE) {
            cout << "[DEBUG] ParseExpr: Starting with token type " 
                    << TokenTypeToString(currentToken.type) << endl;
        }

        // 保存並設置頂層記號
        bool oldIsTopLevelSExp = isTopLevelSExp;
        isTopLevelSExp = isTopLevel;
        
        if (currentToken.type == END_OF_FILE) {
            isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
            return nullptr;
        }
        
        if (!dont_get) {
            GetNextToken();
        } else {
            dont_get = false;
        }
        
        // 檢查是否為 EOF
        if (currentToken.type == END_OF_FILE) {
            isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
            return nullptr;
        }
        
        // 處理右括號錯誤情況
        if (currentToken.type == RIGHT_PAREN) {
            PrintError("atom or '('");
            // 輸出錯誤後，跳過當前行剩餘內容
            SkipToEndOfLine();
            scanner.completedSExp = false;
            isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
            return nullptr;
        }
        
        // 處理點號錯誤情況
        if (currentToken.type == DOT) {
            PrintError("atom or '('");
            
            SkipToEndOfLine();
            scanner.completedSExp = false;
            isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
            return nullptr;
        }
        
        // 處理原子
        if (currentToken.type == INT || 
            currentToken.type == FLOAT || 
            currentToken.type == STRING || 
            currentToken.type == SYMBOL || 
            currentToken.type == NIL || 
            currentToken.type == T) {
                NodePtr result = ParseAtom();
                isTopLevelSExp = oldIsTopLevelSExp;  // 恢復
                return result;
        } 
        // 處理列表
        else if (currentToken.type == LEFT_PAREN) {
            GetNextToken();
            scanner.completedSExp = false;
            
            // 處理空列表
            if (currentToken.type == RIGHT_PAREN) {
                isTopLevelSExp = oldIsTopLevelSExp; 
                return CreateAtomNode(NIL, "nil");
            }
            
            dont_get = true;
            NodePtr list = ParseList(isTopLevel);
            
            // 確保列表正確結束
            if (list == nullptr) {
                isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
                return nullptr;
            }
            isTopLevelSExp = oldIsTopLevelSExp; // 恢復原狀態
            return list;
        }
        // 處理引用表達式 'expr
        else if (currentToken.type == QUOTE) {
            GetNextToken();
            
            dont_get = true;  // 確保遞迴呼叫不會再次取得 token
            NodePtr quotedExpr = ParseExpr();
            if (quotedExpr == nullptr) {
                isTopLevelSExp = oldIsTopLevelSExp; 
                return nullptr;
            }
            
            NodePtr quoteSymbol = CreateAtomNode(SYMBOL, "quote");
            // 創建 (quote . (expr . nil)) 結構
            NodePtr quotedList = CreateConsNode(quotedExpr, CreateAtomNode(NIL, "nil"), false);
            isTopLevelSExp = oldIsTopLevelSExp; 

            return CreateConsNode(quoteSymbol, quotedList, false);
        }
        // 處理其他錯誤情況
        else {
            if (currentToken.type == NONE) {
                isTopLevelSExp = oldIsTopLevelSExp; 
                return nullptr;
            }
            
            PrintError("atom or '('");
            SkipToEndOfLine();
            scanner.completedSExp = false;
            isTopLevelSExp = oldIsTopLevelSExp; 
            return nullptr;
        }
    }
    
public:
    bool exit = false;

    // 初始化解析器
    void Initialize() {
        scanner.Initialize();
        dont_get = false;
        exit = false;
        isTopLevelSExp = false;
        insideList = false;
    }

    // 讀取並解析一個S表達式
    NodePtr ReadSExp() {
        isTopLevelSExp = true;  // 設置為頂層表達式
        NodePtr expr = ParseExpr(true);
        isTopLevelSExp = false;  // 解析完成後重置
        
        if (expr == nullptr) {
            return nullptr;
        }

        scanner.completedSExp = true;
        

        return expr;
    }
    
    // 檢查是否已到文檔結尾
    bool IsEOF() {
        return scanner.IsEOF();
    }
};

class Environment {
    private:
        map<string, NodePtr> binding;
        set<string> primitives;
        ErrorHandler errorHandler;

    public:
        Environment(){
            primitives.insert("cons");
            primitives.insert("list");
            primitives.insert("quote");
            primitives.insert("car");
            primitives.insert("cdr");
            primitives.insert("atom?");
            primitives.insert("pair?");
            primitives.insert("list?");
            primitives.insert("null?");
            primitives.insert("integer?");
            primitives.insert("real?");
            primitives.insert("number?");
            primitives.insert("string?");
            primitives.insert("boolean?");
            primitives.insert("symbol?");
            primitives.insert("+");
            primitives.insert("-");
            primitives.insert("*");
            primitives.insert("/");
            primitives.insert("not");
            primitives.insert("and");
            primitives.insert("or");
            primitives.insert(">");
            primitives.insert(">=");
            primitives.insert("<");
            primitives.insert("<=");
            primitives.insert("=");
            primitives.insert("string-append");
            primitives.insert("string>?");
            primitives.insert("string<?");
            primitives.insert("string=?");
            primitives.insert("eqv?");
            primitives.insert("equal?");
            primitives.insert("begin");
            primitives.insert("if");
            primitives.insert("cond");
            primitives.insert("clean-environment");
            primitives.insert("let");
            primitives.insert("lambda");
            primitives.insert("verbose");
            primitives.insert("verbose?");
        }
        map<string, NodePtr> GetAllBindings() const {
            return binding;
        }
        
        // 設置環境綁定
        void SetBindings(const map<string, NodePtr>& bindings) {
            binding = bindings;
        }
        void Remove(const string& symbol) {
            binding.erase(symbol);
        }
        
        // 檢查符號是否存在
        bool Exists(const string& symbol) const {
            return binding.find(symbol) != binding.end();
        }
        map<string, NodePtr> SaveState() const {
            return binding;  // 返回當前所有綁定的副本
        }
        
        // 恢復完整環境狀態  
        void RestoreState(const map<string, NodePtr>& state) {
            binding = state;  // 完全替換當前綁定
        }

        NodePtr Define(const string& symbol, NodePtr value, ErrorHandler& err) {
            if (primitives.find(symbol) != primitives.end()) {
                err.SetError("DEFINE format)", "", "define");
                return nullptr;
            }
            
            // 增加或更新綁定
            binding[symbol] = value;
            
            // 返回符號定義成功的訊息
            return make_shared<AtomNode>(SYMBOL, symbol + " defined");
        }

        NodePtr LookUp(const string& symbol, ErrorHandler& err) {
            auto it = binding.find(symbol);
            if (it != binding.end()) {
                return it->second;
            }
            // 檢查原始函數
            if (primitives.find(symbol) != primitives.end()) {
                // 創建特殊的原始函數節點
                return make_shared<AtomNode>(SYMBOL, "#<procedure " + symbol + ">");
            }
            
            // 未找到符號
            NodePtr symbolNode = make_shared<AtomNode>(SYMBOL, symbol);  // 創建表示該符號的節點
            err.SetError("unbound symbol", symbol, "", symbolNode);  // 傳遞符號節點作為問題表達式
            return nullptr;
        }

        bool IsPrimitive(const string& symbol) {
            return primitives.find(symbol) != primitives.end();
        }

        void Clean(){
            binding.clear();
        }
};
// 定義一個函數類型，表示原始函數
using PrimitiveFunc = function<NodePtr(const vector<NodePtr>&, ErrorHandler&, NodePtr)>;

// 原始函數信息結構
struct PrimitiveInfo {
    PrimitiveFunc func;
    int minArgs;
    int maxArgs;
    string name; 
};

// 完整的 Primitives 命名空間
namespace Primitives {
    // cons 函數
    NodePtr Cons(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {

        bool isDotted = true;
        if (NodeNormalizer::IsListNode(args[1])) {
            isDotted = false;
        }
        
        return make_shared<ConsNode>(args[0], args[1], isDotted);
    }
    
    // car 函數
    NodePtr Car(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "car");
            return nullptr;
        }
        
        if (args[0]->GetType() != CONS_NODE) {
            if (args[0]->GetType() == ATOM_NODE) {
                AtomNode* atom = static_cast<AtomNode*>(args[0].get());
                string value = atom->GetValue();
                
                if (atom->GetAtomType() == FLOAT) {
                    // 對浮點數進行格式化
                    try {
                        double val = stod(value);
                        ostringstream out;
                        out << fixed << setprecision(3) << val;
                        err.SetError("car with incorrect argument type", out.str());
                    } catch (...) {
                        // 如果格式化失敗，使用原始值
                        err.SetError("car with incorrect argument type", value);
                    }
                    return nullptr;
                }
                
                // 如果是字串類型，確保有引號
                if (atom->GetAtomType() == STRING) {
                    value = "\"" + value + "\"";
                }
                
                err.SetError("car with incorrect argument type", value);
            } else {
                err.SetError("car with incorrect argument type", "");
            }
            return nullptr;
        }
        
        ConsNode* cons = (ConsNode*)args[0].get();
        return cons->GetLeft();
    }

    // cdr 函數
    NodePtr Cdr(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "cdr");
            return nullptr;
        }
        
        if (args[0]->GetType() != CONS_NODE) {
            // 直接傳遞整個節點而不是它的值
            err.SetError("cdr with incorrect argument type)", "", "", args[0]);
            return nullptr;
        }
    
        ConsNode* cons = (ConsNode*)args[0].get();
        return cons->GetRight();
    }
    
    // 加法函數
    NodePtr Add(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "+");
            return nullptr;
        }
        
        // 檢查參數類型並計算
        bool isFloat = false;
        double sum = 0.0;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("+ with incorrect argument type", "", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
            
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";  // 為字串添加引號
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("+ with incorrect argument type", valueStr);
                return nullptr;
            }
            
            if (atom->GetAtomType() == FLOAT) {
                isFloat = true;
                sum += stod(atom->GetValue());
            } else {
                sum += stoi(atom->GetValue());
            }
        }
        
        // 創建結果節點
        if (isFloat) {
            ostringstream ss;
            ss << fixed << setprecision(3) << sum;
            return make_shared<AtomNode>(FLOAT, ss.str());
        } else {
            return make_shared<AtomNode>(INT, to_string((int)sum));
        }
    }
    
    // 減法函數
    NodePtr Subtract(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "-");
            return nullptr;
        }
        
        // 檢查參數類型
        bool isFloat = false;
        double result = 0.0;
        
        // 處理第一個參數
        if (args[0]->GetType() != ATOM_NODE) {
            err.SetError("- with incorrect argument type", "");
            return nullptr;
        }
        
        AtomNode* firstAtom = (AtomNode*)args[0].get();
        if (firstAtom->GetAtomType() != INT && firstAtom->GetAtomType() != FLOAT) {
            string valueStr;
            
            if (firstAtom->GetAtomType() == NIL) {
                valueStr = "nil";
            } else if (firstAtom->GetAtomType() == T) {
                valueStr = "#t";
            } else if (firstAtom->GetAtomType() == STRING) {
                valueStr = "\"" + firstAtom->GetValue() + "\"";  // 為字串添加引號
            } else {
                valueStr = firstAtom->GetValue();
            }
            
            err.SetError("+ with incorrect argument type", valueStr);
            return nullptr;
        }
        
        if (firstAtom->GetAtomType() == FLOAT) {
            isFloat = true;
            result = stod(firstAtom->GetValue());
        } else {
            result = stoi(firstAtom->GetValue());
        }
        
        // 減去後續參數
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i]->GetType() != ATOM_NODE) {
                err.SetError("- with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)args[i].get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                err.SetError("- with incorrect argument type", atom->GetValue());
                return nullptr;
            }
            
            if (atom->GetAtomType() == FLOAT) {
                isFloat = true;
                result -= stod(atom->GetValue());
            } else {
                result -= stoi(atom->GetValue());
            }
        }
        
        // 創建結果節點
        if (isFloat) {
            ostringstream ss;
            ss << fixed << setprecision(3) << result;
            return make_shared<AtomNode>(FLOAT, ss.str());
        } else {
            return make_shared<AtomNode>(INT, to_string((int)result));
        }
    }
    
    // 乘法函數
    NodePtr Multiply(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "*");
            return nullptr;
        }
        
        // 檢查參數類型並計算
        bool isFloat = false;
        double product = 1.0;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("* with incorrect argument type", "", "", arg);
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
            
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";  // 為字串添加引號
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("* with incorrect argument type", valueStr);
                return nullptr;
            }
            
            
            if (atom->GetAtomType() == FLOAT) {
                isFloat = true;
                product *= stod(atom->GetValue());
            } else {
                product *= stoi(atom->GetValue());
            }
        }
        
        // 創建結果節點
        if (isFloat) {
            ostringstream ss;
            ss << fixed << setprecision(3) << product;
            return make_shared<AtomNode>(FLOAT, ss.str());
        } else {
            return make_shared<AtomNode>(INT, to_string((int)product));
        }
    }
    
    // 除法函數
    NodePtr Divide(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "/");
            return nullptr;
        }
        
        // 檢查參數類型
        bool isFloat = false;
        double result = 0.0;
        
        // 處理第一個參數
        if (args[0]->GetType() != ATOM_NODE) {
            err.SetError("/ with incorrect argument type", "");
            return nullptr;
        }
        
        AtomNode* firstAtom = (AtomNode*)args[0].get();
        if (firstAtom->GetAtomType() != INT && firstAtom->GetAtomType() != FLOAT) {
            string valueStr;
            
            if (firstAtom->GetAtomType() == NIL) {
                valueStr = "nil";
            } else if (firstAtom->GetAtomType() == T) {
                valueStr = "#t";
            } else if (firstAtom->GetAtomType() == STRING) {
                valueStr = "\"" + firstAtom->GetValue() + "\"";  // 為字串添加引號
            } else {
                valueStr = firstAtom->GetValue();
            }
            
            err.SetError("+ with incorrect argument type", valueStr);
            return nullptr;
        }
        
        if (firstAtom->GetAtomType() == FLOAT) {
            isFloat = true;
            result = stod(firstAtom->GetValue());
        } else {
            result = stoi(firstAtom->GetValue());
        }
        
        // 除以後續參數
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i]->GetType() != ATOM_NODE) {
                err.SetError("/ with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)args[i].get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                err.SetError("/ with incorrect argument type", atom->GetValue());
                return nullptr;
            }
            
            double divisor;
            if (atom->GetAtomType() == FLOAT) {
                isFloat = true;
                divisor = stod(atom->GetValue());
            } else {
                divisor = stoi(atom->GetValue());
            }
            
            // 檢查除以零
            if (divisor == 0) {
                err.SetError("division by zero", "", "/");
                return nullptr;
            }
            
            result /= divisor;
        }
        
        // 創建結果節點
        if (isFloat) {
            ostringstream ss;
            ss << fixed << setprecision(3) << result;
            return make_shared<AtomNode>(FLOAT, ss.str());
        } else {
            return make_shared<AtomNode>(INT, to_string((int)result));
        }
    }
    
    // list 函數
    NodePtr List(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.empty()) {
            return make_shared<AtomNode>(NIL, "nil");
        }
        
        // 構建列表
        NodePtr result = make_shared<AtomNode>(NIL, "nil");
        
        // 從後向前構建列表
        for (int i = args.size() - 1; i >= 0; i--) {
            result = make_shared<ConsNode>(args[i], result, false);
        }
        
        return result;
    }
    
    // atom? 函數
    NodePtr IsAtom(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "atom?");
            return nullptr;
        }
        
        // 檢查是否為原子
        bool isAtom = args[0]->GetType() == ATOM_NODE;
        
        return isAtom ? 
               make_shared<AtomNode>(T, "#t") : 
               make_shared<AtomNode>(NIL, "nil");
    }

    // not 函數
    NodePtr Not(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "not");
            return nullptr;
        }
        
        // 檢查是否為 nil
        bool isNil = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isNil = (atom->GetAtomType() == NIL);
        }
        
        return isNil ? 
               make_shared<AtomNode>(T, "#t") : 
               make_shared<AtomNode>(NIL, "nil");
    }

    // pair? 函數
    NodePtr IsPair(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "pair?");
            return nullptr;
        }
        
        bool isPair = args[0]->GetType() == CONS_NODE;
        
        return isPair ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // null? 函數
    NodePtr IsNull(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "null?");
            return nullptr;
        }
        
        bool isNull = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isNull = atom->GetAtomType() == NIL;
        }
        
        return isNull ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // integer? 函數
    NodePtr IsInteger(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "integer?");
            return nullptr;
        }
        
        bool isInteger = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isInteger = atom->GetAtomType() == INT;
        }
        
        return isInteger ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // real? 函數
    NodePtr IsReal(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "real?");
            return nullptr;
        }
        
        bool isReal = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isReal = (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT);
        }
        
        return isReal ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // number? 函數
    NodePtr IsNumber(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "number?");
            return nullptr;
        }
        
        bool isNumber = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isNumber = (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT);
        }
        
        return isNumber ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // string? 函數
    NodePtr IsString(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "string?");
            return nullptr;
        }
        
        bool isString = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isString = (atom->GetAtomType() == STRING);
        }
        
        return isString ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // boolean? 函數
    NodePtr IsBoolean(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "boolean?");
            return nullptr;
        }
        
        bool isBoolean = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isBoolean = (atom->GetAtomType() == T || atom->GetAtomType() == NIL);
        }
        
        return isBoolean ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // symbol? 函數
    NodePtr IsSymbol(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "symbol?");
            return nullptr;
        }
        
        bool isSymbol = false;
        if (args[0]->GetType() == ATOM_NODE) {
            AtomNode* atom = (AtomNode*)args[0].get();
            isSymbol = (atom->GetAtomType() == SYMBOL);
        }
        
        return isSymbol ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    // > 函數
    NodePtr GreaterThan(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", ">");
            return nullptr;
        }
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("> with incorrect argument type", "", "", arg);
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();

            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
                
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("> with incorrect argument type", valueStr);
                return nullptr;
            }
        }
            
        double prevValue = 0;
        bool isFirst = true;
        
        for (const auto& arg : args) {
            AtomNode* atom = (AtomNode*)arg.get();
            double value;
            if (atom->GetAtomType() == INT) {
                value = stoi(atom->GetValue());
            } else {
                value = stod(atom->GetValue());
            }
            
            if (isFirst) {
                prevValue = value;
                isFirst = false;
                continue;
            }
            
            if (prevValue <= value) {
                return make_shared<AtomNode>(NIL, "nil");
            }
            
            prevValue = value;
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // >= 函數
    NodePtr GreaterThanOrEqual(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", ">=");
            return nullptr;
        }
        
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError(">= with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";
                } else if (atom->GetAtomType() == SYMBOL) {
                    valueStr = atom->GetValue();
                } else {
                    valueStr = atom->GetValue();
                }
                

                err.SetError(">= with incorrect argument type", valueStr);
                return nullptr;
            }
        }
        double prevValue = 0;
        bool isFirst = true;
        
        for (const auto& arg : args) {
            AtomNode* atom = (AtomNode*)arg.get();
            double value;
            if (atom->GetAtomType() == INT) {
                value = stoi(atom->GetValue());
            } else {
                value = stod(atom->GetValue());
            }
            
            if (isFirst) {
                prevValue = value;
                isFirst = false;
                continue;
            }
            
            if (prevValue < value) {
                return make_shared<AtomNode>(NIL, "nil");
            }
        
            prevValue = value;
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // < 函數
    NodePtr LessThan(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "<");
            return nullptr;
        }
        
        double prevValue = 0;
        bool isFirst = true;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("< with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
            
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("< with incorrect argument type", valueStr);
                return nullptr;
            }
            
            double value;
            if (atom->GetAtomType() == INT) {
                value = stoi(atom->GetValue());
            } else {
                value = stod(atom->GetValue());
            }
            
            if (isFirst) {
                prevValue = value;
                isFirst = false;
                continue;
            }
            
            if (prevValue >= value) {
                return make_shared<AtomNode>(NIL, "nil");
            }
            
            prevValue = value;
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // <= 函數
    NodePtr LessThanOrEqual(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "<=");
            return nullptr;
        }
        
        double prevValue = 0;
        bool isFirst = true;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("<= with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
                
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("<= with incorrect argument type", valueStr);
                return nullptr;
            }
            
            double value;
            if (atom->GetAtomType() == INT) {
                value = stoi(atom->GetValue());
            } else {
                value = stod(atom->GetValue());
            }
            
            if (isFirst) {
                prevValue = value;
                isFirst = false;
                continue;
            }
            
            if (prevValue > value) {
                return make_shared<AtomNode>(NIL, "nil");
            }
            
            prevValue = value;
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // = 函數
    NodePtr EqualNum(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "=");
            return nullptr;
        }
        
        double firstValue = 0;
        bool valueSet = false;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("= with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != INT && atom->GetAtomType() != FLOAT) {
                string valueStr;
            
                if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else if (atom->GetAtomType() == STRING) {
                    valueStr = "\"" + atom->GetValue() + "\"";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("= with incorrect argument type", valueStr);
                return nullptr;
            }
            
            double value;
            if (atom->GetAtomType() == INT) {
                value = stoi(atom->GetValue());
            } else {
                value = stod(atom->GetValue());
            }
            
            if (!valueSet) {
                firstValue = value;
                valueSet = true;
                continue;
            }
            
            if (firstValue != value) {
                return make_shared<AtomNode>(NIL, "nil");
            }
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // string-append 函數
    NodePtr StringAppend(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 1) {
            err.SetError("incorrect number of arguments", "", "string-append");
            return nullptr;
        }
        
        string result = "";
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("string-append with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = dynamic_cast<AtomNode*>(arg.get());
            if (atom->GetAtomType() != STRING) {
                string valueStr;
                
                if (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT) {
                    valueStr = atom->GetValue();
                } else if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("string-append with incorrect argument type", valueStr);
                return nullptr;
            }
            
            result += atom->GetValue();
        }
        
        return make_shared<AtomNode>(STRING, result);
    }
    
    // string>? 函數
    NodePtr StringGreaterThan(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "string>?");
            return nullptr;
        }

        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("string<? with incorrect argument type", "", "", arg);
                return nullptr;
            }
            
            AtomNode* atom = dynamic_cast<AtomNode*>(arg.get());
            if (atom->GetAtomType() != STRING) {
                string valueStr;
                
                if (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT) {
                    valueStr = atom->GetValue();
                } else if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("string>? with incorrect argument type", valueStr);
                return nullptr;
            }
        }
            
        string prevStr = "";
        bool isFirst = true;
        
        for (const auto& arg : args) {
            AtomNode* atom = (AtomNode*)arg.get();
            string str = atom->GetValue();
            
            if (isFirst) {
                prevStr = str;
                isFirst = false;
                continue;
            }
            
            if (prevStr <= str) {
                return make_shared<AtomNode>(NIL, "nil");
            }
            
            prevStr = str;
        }
        
        return make_shared<AtomNode>(T, "#t");
        
    }
    


        // string<? 函數
    NodePtr StringLessThan(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "string<?");
            return nullptr;
        }
        
        // 第一階段：檢查所有參數是否都是字串
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("string<? with incorrect argument type", "");
                return nullptr;
            }
            
            AtomNode* atom = (AtomNode*)arg.get();
            if (atom->GetAtomType() != STRING) {
                string valueStr;
                
                if (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT) {
                    valueStr = atom->GetValue();
                } else if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("string<? with incorrect argument type", valueStr);
                return nullptr;
            }
        }
        
        // 第二階段：所有參數都是字串，執行比較邏輯
        string prevStr = "";
        bool isFirst = true;
        
        for (const auto& arg : args) {
            AtomNode* atom = (AtomNode*)arg.get();
            string str = atom->GetValue();
            
            if (isFirst) {
                prevStr = str;
                isFirst = false;
                continue;
            }
            
            if (prevStr >= str) {
                return make_shared<AtomNode>(NIL, "nil");
            }
            
            prevStr = str;
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // string=? 函數
    NodePtr StringEqual(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() < 2) {
            err.SetError("incorrect number of arguments", "", "string=?");
            return nullptr;
        }
        
        string firstStr = "";
        bool valueSet = false;
        
        for (const auto& arg : args) {
            if (arg->GetType() != ATOM_NODE) {
                err.SetError("string<? with incorrect argument type", "", "", arg);
                return nullptr;
            }
            
            AtomNode* atom = dynamic_cast<AtomNode*>(arg.get());
            if (atom->GetAtomType() != STRING) {
                string valueStr;
                
                if (atom->GetAtomType() == INT || atom->GetAtomType() == FLOAT) {
                    valueStr = atom->GetValue();
                } else if (atom->GetAtomType() == NIL) {
                    valueStr = "nil";
                } else if (atom->GetAtomType() == T) {
                    valueStr = "#t";
                } else {
                    valueStr = atom->GetValue();
                }
                
                err.SetError("string=? with incorrect argument type", valueStr);
                return nullptr;
            }
            
            string str = atom->GetValue();
            
            if (!valueSet) {
                firstStr = str;
                valueSet = true;
                continue;
            }
            
            if (firstStr != str) {
                return make_shared<AtomNode>(NIL, "nil");
            }
        }
        
        return make_shared<AtomNode>(T, "#t");
    }

    // eqv? 函數
    NodePtr Eqv(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 2) {
            err.SetError("incorrect number of arguments", "", "eqv?");
            return nullptr;
        }
        
        // 如果是相同的對象（記憶體位址相同）
        if (args[0] == args[1]) {
            return make_shared<AtomNode>(T, "#t");
        }
        
        // 如果兩個都是原子
        if (args[0]->GetType() == ATOM_NODE && args[1]->GetType() == ATOM_NODE) {
            AtomNode* atom1 = (AtomNode*)args[0].get();
            AtomNode* atom2 = (AtomNode*)args[1].get();
            
            // 字符串特殊處理：字符串必須是相同對象才返回 #t
            if (atom1->GetAtomType() == STRING && atom2->GetAtomType() == STRING) {
                return make_shared<AtomNode>(NIL, "nil"); // 不同對象的字符串返回 nil
            }
            
            // 對於其他類型的原子，如果類型和值相同，返回 #t
            if (atom1->GetAtomType() == atom2->GetAtomType() && 
                atom1->GetValue() == atom2->GetValue()) {
                return make_shared<AtomNode>(T, "#t");
            }
        }
        
        // 特殊處理：如果兩個都是 CONS_NODE，即使結構相同也返回 nil
        if (args[0]->GetType() == CONS_NODE && args[1]->GetType() == CONS_NODE) {
            return make_shared<AtomNode>(NIL, "nil");
        }
        
        // 如果不是相同類型或相同值，返回 nil
        return make_shared<AtomNode>(NIL, "nil");
    }
    
    // 遞歸檢查兩個節點是否相等
    bool AreEqual(NodePtr node1, NodePtr node2) {
        if (node1 == node2) {
            return true;
        }
        
        // 如果類型不同
        if (node1->GetType() != node2->GetType()) {
            return false;
        }
        
        // 如果都是原子
        if (node1->GetType() == ATOM_NODE) {
            AtomNode* atom1 = (AtomNode*)node1.get();
            AtomNode* atom2 = (AtomNode*)node2.get();
            
            // 如果類型不同或值不同
            if (atom1->GetAtomType() != atom2->GetAtomType() || 
                atom1->GetValue() != atom2->GetValue()) {
                return false;
            }
            
            return true;
        }
        
        if (node1->GetType() == CONS_NODE) {
            ConsNode* cons1 = (ConsNode*)node1.get();
            ConsNode* cons2 = (ConsNode*)node2.get();
            
            return AreEqual(cons1->GetLeft(), cons2->GetLeft()) && 
                AreEqual(cons1->GetRight(), cons2->GetRight());
        }
        
        return false;
    }
    
    // equal? 函數
    NodePtr Equal(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 2) {
            err.SetError("incorrect number of arguments", "", "equal?");
            return nullptr;
        }
        
        bool areEqual = AreEqual(args[0], args[1]);
        
        return areEqual ? 
            make_shared<AtomNode>(T, "#t") : 
            make_shared<AtomNode>(NIL, "nil");
    }

    NodePtr IsList(const vector<NodePtr>& args, ErrorHandler& err, NodePtr originalExpr = nullptr) {
        if (args.size() != 1) {
            err.SetError("incorrect number of arguments", "", "list?");
            return nullptr;
        }
        
        // 檢查是否為表
        bool isList = NodeNormalizer::IsListNode(args[0]);
        
        return isList ? 
               make_shared<AtomNode>(T, "#t") : 
               make_shared<AtomNode>(NIL, "nil");
    }
    
}
class Evaluator {
    private:
        struct LambdaInfo {
            vector<string> parameters;
            vector<NodePtr> bodyExpressions;
            
            string lambdaId;
        };
    
        
        map<string, shared_ptr<LambdaInfo>> lambdaFunctions;
        int lambdaCounter = 0;
        bool verboseMode = true;
        Environment env;
        ErrorHandler errorHandler;
        map<string, PrimitiveInfo> primitives;
        // 註冊所有原始函數
        void RegisterPrimitives() {
            primitives["cons"] = {Primitives::Cons, 2, 2, "cons"};
            primitives["car"] = {Primitives::Car, 1, 1, "car"};
            primitives["cdr"] = {Primitives::Cdr, 1, 1, "cdr"};
            primitives["+"] = {Primitives::Add, 2, -1, "+"};
            primitives["-"] = {Primitives::Subtract, 2, -1, "-"};
            primitives["*"] = {Primitives::Multiply, 2, -1, "*"};
            primitives["/"] = {Primitives::Divide, 2, -1, "/"};
            primitives["list?"] = {Primitives::IsList, 1, 1, "list?"};
            primitives["atom?"] = {Primitives::IsAtom, 1, 1, "atom?"};
            primitives["not"] = {Primitives::Not, 1, 1, "not"};
            primitives["pair?"] = {Primitives::IsPair, 1, 1, "pair?"};
            primitives["null?"] = {Primitives::IsNull, 1, 1, "null?"};
            primitives["integer?"] = {Primitives::IsInteger, 1, 1, "integer?"};
            primitives["real?"] = {Primitives::IsReal, 1, 1, "real?"};
            primitives["number?"] = {Primitives::IsNumber, 1, 1, "number?"};
            primitives["string?"] = {Primitives::IsString, 1, 1, "string?"};
            primitives["boolean?"] = {Primitives::IsBoolean, 1, 1, "boolean?"};
            primitives["symbol?"] = {Primitives::IsSymbol, 1, 1, "symbol?"};
            primitives[">"] = {Primitives::GreaterThan, 2, -1, ">"};
            primitives[">="] = {Primitives::GreaterThanOrEqual, 2, -1, ">="};
            primitives["<"] = {Primitives::LessThan, 2, -1, "<"};
            primitives["<="] = {Primitives::LessThanOrEqual, 2, -1, "<="};
            primitives["="] = {Primitives::EqualNum, 2, -1, "="};
            primitives["string-append"] = {Primitives::StringAppend, 2, -1, "string-append"};
            primitives["string>?"] = {Primitives::StringGreaterThan, 2, -1, "string>?"};
            primitives["string<?"] = {Primitives::StringLessThan, 2, -1, "string<?"};
            primitives["string=?"] = {Primitives::StringEqual, 2, -1, "string=?"};
            primitives["eqv?"] = {Primitives::Eqv, 2, 2, "eqv?"};
            primitives["equal?"] = {Primitives::Equal, 2, 2, "equal?"};
        }
        bool IsSpecialForm(NodePtr expr) {
            if (expr->GetType() == CONS_NODE) {
                ConsNode* cons = (ConsNode*)expr.get();
                NodePtr op = cons->GetLeft();
                
                if (op->GetType() == ATOM_NODE) {
                    AtomNode* atom = (AtomNode*)op.get();
                    if (atom->GetAtomType() == SYMBOL) {
                        string opName = atom->GetValue();
                        return (opName == "define" || opName == "clean-environment");
                    }
                }
            }
            return false;
        }
        vector<NodePtr> EvaluateArgs(NodePtr argList, NodePtr originalExpr = nullptr) {
            vector<NodePtr> evaluatedArgs;
        
            // Check if argList is nil (empty list)
            if (argList && argList->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(argList.get());
                if (atom && atom->GetAtomType() == NIL) {
                    return evaluatedArgs; // Return empty vector for empty argument list
                }
            }
            
            // Check if argList is not a list
            if (argList && argList->GetType() == CONS_NODE) {
                if (!NodeNormalizer::IsListNode(argList)) {
                    errorHandler.SetError("non-list", "", "", originalExpr);
                    return evaluatedArgs;
                }
            }
            
            while (argList && argList->GetType() == CONS_NODE && !errorHandler.HasError()) {
                ConsNode* cons = dynamic_pointer_cast<ConsNode>(argList).get();
                NodePtr arg = cons->GetLeft();
                
                // Evaluate the argument
                NodePtr argValue = Eval(arg, originalExpr); // Pass arg as originalExpr to preserve arg expression for error messages
                
                if (errorHandler.HasError()) {
                    return evaluatedArgs;  // Return what we have so far if error occurred
                }
                
                evaluatedArgs.push_back(argValue);
                argList = cons->GetRight();
            }
            
            return evaluatedArgs;
        }
        // 判斷是否為nil值
        bool IsNil(NodePtr node) const {
            if (node && node->GetType() == ATOM_NODE) {
                auto atom = dynamic_pointer_cast<AtomNode>(node);
                return atom->GetAtomType() == NIL;
            }
            return false;
        }
        bool IsNilNode(NodePtr node) const {
            if (!node) {
                return false;
            }
            
            if (node->GetType() == ATOM_NODE) {
                auto atom = dynamic_pointer_cast<AtomNode>(node);
                if (atom && atom->GetAtomType() == NIL) {
                    return true;
                }
            }
            
            return false;
        }

        vector<NodePtr> GetArgList(NodePtr argList) {
            vector<NodePtr> args;
            
            while (argList && argList->GetType() == CONS_NODE) {
                ConsNode* cons = dynamic_pointer_cast<ConsNode>(argList).get();
                args.push_back(cons->GetLeft());
                argList = cons->GetRight();
            }
            
            return args;
        }
        bool validateNoNestedDefines(NodePtr node, ErrorHandler& errorHandler, NodePtr originalExpr) {
            if (!node) {
                return true;
            }
            
            if (node->GetType() == ATOM_NODE) {
                return true; // Atoms can't contain nested defines
            }
            
            if (node->GetType() == CONS_NODE) {
                ConsNode* cons = dynamic_cast<ConsNode*>(node.get());
                if (!cons) return true;
                
                // Check if this is a define expression
                NodePtr first = cons->GetLeft();
                if (first && first->GetType() == ATOM_NODE) {
                    AtomNode* atom = dynamic_cast<AtomNode*>(first.get());
                    if (atom && atom->GetAtomType() == SYMBOL) {
                        string name = atom->GetValue();
                        
                        // Check for special forms that shouldn't be nested
                        if (name == "define") {
                            errorHandler.SetError("level of DEFINE", "", "", originalExpr);
                            return false;
                        } else if (name == "clean-environment") {
                            errorHandler.SetError("level of CLEAN-ENVIRONMENT", "", "", originalExpr);
                            return false;
                        } else if (name == "exit") {
                            errorHandler.SetError("level of EXIT", "", "", originalExpr);
                            return false;
                        }
                    }
                }
                
                // Recursively check left and right branches
                if (!validateNoNestedDefines(cons->GetLeft(), errorHandler, originalExpr)) {
                    return false;
                }
                
                return validateNoNestedDefines(cons->GetRight(), errorHandler, originalExpr);
            }
            
            return true;
        }
        NodePtr ApplyUserFunction(NodePtr funcValue, NodePtr args, NodePtr originalExpr, const string& funcName = "") {
            shared_ptr<LambdaInfo> lambdaInfo = nullptr;
            
            // 查找 lambda 函數
            if (funcValue && funcValue->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(funcValue.get());
                string value = atom->GetValue();
                
                if (value.find("#<procedure lambda:") == 0) {
                    size_t startPos = value.find("lambda:") + 7;
                    size_t endPos = value.find(">", startPos);
                    if (endPos != string::npos) {
                        string lambdaId = value.substr(startPos, endPos - startPos);
                        auto it = lambdaFunctions.find(lambdaId);
                        if (it != lambdaFunctions.end()) {
                            lambdaInfo = it->second;
                        }
                    }
                } else if (value == "#<procedure lambda>") {
                    for (auto it = lambdaFunctions.rbegin(); it != lambdaFunctions.rend(); ++it) {
                        if (it->first.find("lambda_") == 0) {
                            lambdaInfo = it->second;
                            break;
                        }
                    }
                } else if (!funcName.empty()) {
                    for (const auto& pair : lambdaFunctions) {
                        if (pair.first.find(funcName + "_") == 0) {
                            lambdaInfo = pair.second;
                            break;
                        }
                    }
                }
            }
            
            if (!lambdaInfo) {
                errorHandler.SetError("internal error", "function not found");
                return nullptr;
            }
            
            // 評估參數
            vector<NodePtr> evaluatedArgs = EvaluateArgs(args, originalExpr);
            if (errorHandler.HasError()) {
                return nullptr;
            }
            
            // 檢查參數數量
            if (evaluatedArgs.size() != lambdaInfo->parameters.size()) {
                errorHandler.SetError("incorrect number of arguments", "", 
                                    funcName.empty() ? "lambda expression" : funcName);
                return nullptr;
            }
            
            // **關鍵修復：保存當前環境，但不恢復到lambda定義時的環境**
            map<string, NodePtr> savedEnv = env.SaveState();
            
            // 在當前環境中綁定參數（不清空環境）
            for (size_t i = 0; i < lambdaInfo->parameters.size(); ++i) {
                env.Define(lambdaInfo->parameters[i], evaluatedArgs[i], errorHandler);
            }
            
            // 執行函數體
            NodePtr result = nullptr;
            for (size_t i = 0; i < lambdaInfo->bodyExpressions.size(); ++i) {
                result = Eval(lambdaInfo->bodyExpressions[i], originalExpr);
                
                // 如果是最後一個表達式
                if (i == lambdaInfo->bodyExpressions.size() - 1) {
                    // 最後一個表達式的任何錯誤都要傳播
                    if (errorHandler.HasError()) {
                        break;
                    }
                    // 如果最後一個表達式沒有返回值
                    if (!result) {
                        errorHandler.SetError("no return value", "", "", originalExpr);
                        break;
                    }
                } else {
                    // 不是最後一個表達式
                    if (errorHandler.HasError()) {
                        // 只有 "no return value" 錯誤可以忽略
                        if (errorHandler.GetErrorMessage().find("no return value") == 0) {
                            errorHandler.ClearError();
                        } else {
                            break;
                        }
                    }
                }
            }
            
            // **修復：只恢復參數綁定，保持其他變數的當前狀態**
            // 移除新添加的參數綁定
            for (const string& param : lambdaInfo->parameters) {
                if (savedEnv.find(param) != savedEnv.end()) {
                    env.Define(param, savedEnv[param], errorHandler);
                } else {
                    env.Remove(param);
                }
            }
            
            return result;
        }
    public:
        Evaluator() {
            RegisterPrimitives();
        }
        // 主評估函數
        NodePtr Eval(NodePtr expr, NodePtr originalExpr = nullptr) {
            if (!expr) {
                return nullptr;
            }
            
            if (!originalExpr) {
                originalExpr = expr;  // If no original expression provided, use current expression
            }
        
            // Case 1: If evaluating an atom but not a symbol, return that atom
            if (expr->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(expr.get());
                TokenType atomType = atom->GetAtomType();
                
                // Self-evaluating atoms: numbers, strings, special values
                if (atomType == INT || atomType == FLOAT || 
                    atomType == STRING || atomType == T || atomType == NIL) {
                    return expr;
                }
                // Case 2: If evaluating a symbol
                else if (atomType == SYMBOL) {
                    const string& symbolName = atom->GetValue();
                    
                    // Special case for 'exit'
                    if (symbolName == "exit") {
                        return make_shared<AtomNode>(SYMBOL, "#<procedure exit>");
                    }
                    
                    // Look up the symbol binding
                    return env.LookUp(symbolName, errorHandler);
                    // Note: LookUp will set error if symbol is unbound
                }
            }
            
            // Case 3: Evaluating a list expression (...)
            if (expr->GetType() == CONS_NODE) {
                ConsNode* cons = dynamic_cast<ConsNode*>(expr.get());
                
                // Check if it's a proper list
                if (!NodeNormalizer::IsListNode(expr)) {
                    errorHandler.SetError("non-list", "", "", expr);
                    return nullptr;
                }
                
                // Get the operator (first element)
                NodePtr firstArg = cons->GetLeft();
                
                // Case 3.1: If first element is an atom but not a symbol
                if (firstArg->GetType() == ATOM_NODE) {
                    AtomNode* atomFirstArg = dynamic_cast<AtomNode*>(firstArg.get());
                    if (atomFirstArg->GetAtomType() != SYMBOL) {
                        errorHandler.SetError("attempt to apply non-function", "", "", firstArg);
                        return nullptr;
                    }
                    
                    // Case 3.2: First element is a symbol SYM
                    string symName = atomFirstArg->GetValue();
                    
                    // Check whether SYM is the name of a function
                    bool isSpecialForm = symName == "quote" || 
                                       symName == "if" || 
                                       symName == "cond" || 
                                       symName == "define" || 
                                       symName == "and" || 
                                       symName == "or" || 
                                       symName == "begin" || 
                                       symName == "clean-environment" || 
                                       symName == "exit" ||
                                       symName == "let" ||        
                                       symName == "lambda";  
                                       
                    if (isSpecialForm) {
                        // SYM is a special form
                        
                        // Check if current level is not top level and SYM is a special form that's not allowed
                        if (originalExpr != expr && 
                            (symName == "clean-environment" || 
                             symName == "define" || 
                             symName == "exit")) {
                            if (symName == "clean-environment")
                            {
                                errorHandler.SetError("level of CLEAN-ENVIRONMENT", "", "", expr);
                                return nullptr;
                            }
                            else if (symName == "define")
                            {
                                errorHandler.SetError("level of DEFINE", "", "", expr);
                                return nullptr;
                            }
                            else{
                                errorHandler.SetError("level of EXIT", "", "", expr);
                                return nullptr;
                            }
                        
                        }
                        
                        // Handle special forms
                        if (symName == "define") {
                            NodePtr defArgs = cons->GetRight();
                            if (defArgs && defArgs->GetType() == CONS_NODE) {
                                ConsNode* defArgsCons = dynamic_cast<ConsNode*>(defArgs.get());
                                NodePtr firstArg = defArgsCons->GetLeft();
                                
                                // 如果第一個參數是列表，表示函數定義形式
                                if (firstArg && firstArg->GetType() == CONS_NODE) {
                                    return EvalFunctionDefine(defArgs, expr);
                                }
                            }
                            
                            // 否則使用原有的define處理
                            return EvalDefine(cons->GetRight(), expr);
                        } 
                        else if (symName == "cond") {
                            return EvalCond(cons->GetRight(), expr);
                        }
                        else if (symName == "if") {
                            // Get argument list without evaluating them
                            vector<NodePtr> args = GetArgList(cons->GetRight());
                            
                            // Check argument count
                            if (args.size() < 2 || args.size() > 3) {
                                errorHandler.SetError("incorrect number of arguments", "", "if", originalExpr);
                                return nullptr;
                            }
                            
                            return EvalIf(cons->GetRight(), expr);
                        }
                        else if (symName == "and") {
                            return EvalAnd(cons->GetRight(), expr);
                        }
                        else if (symName == "or") {
                            return EvalOr(cons->GetRight(), expr);
                        }
                        else if (symName == "quote") {
                            return EvalQuote(cons->GetRight(), expr);
                        }
                        else if (symName == "begin") {
                            return EvalBegin(cons->GetRight(), expr);
                        }
                        else if (symName == "clean-environment") {
                            env.Clean();
                            lambdaFunctions.clear();  // 清除所有 lambda 函數
                            lambdaCounter = 0;        // 重置計數器
                            return make_shared<AtomNode>(SYMBOL, "environment cleaned");
                        }
                        else if (symName == "exit") {
                            // Check if exit has any arguments (should have none)
                            NodePtr args = cons->GetRight();
                            if (args && args->GetType() == CONS_NODE) {
                                ConsNode* argsCons = dynamic_cast<ConsNode*>(args.get());
                                if (argsCons->GetLeft() != nullptr) {
                                    // exit has arguments, which is an error
                                    errorHandler.SetError("incorrect number of arguments", "", "exit", originalExpr);
                                    return nullptr;
                                }
                            }
                            return make_shared<AtomNode>(SYMBOL, "#<procedure exit>");
                        }
                        else if (symName == "let") {
                            return EvalLet(cons->GetRight(), expr);
                        }
                        else if (symName == "lambda") {
                            return EvalLambda(cons->GetRight(), expr);
                        }
                    }
                    else if (symName == "list") {
                        // Special case for list
                        NodePtr args = cons->GetRight();
                        vector<NodePtr> evaluatedArgs = EvaluateArgs(args, originalExpr);
                        
                        if (errorHandler.HasError()) {
                            return nullptr;
                        }
                        
                        return Primitives::List(evaluatedArgs, errorHandler, originalExpr);
                    }
                    else if (symName == "cons") {
                        // Special case for cons
                        NodePtr args = cons->GetRight();
                        vector<NodePtr> argList = GetArgList(args);
                        if (argList.size()!=2)
                        {
                            errorHandler.SetError("incorrect number of arguments", "", "cons", originalExpr);
                            return nullptr;
                        }
                        
                        vector<NodePtr> evaluatedArgs = EvaluateArgs(args, originalExpr);
                        
                        if (errorHandler.HasError()) {
                            return nullptr;
                        }
                        
                        return Primitives::Cons(evaluatedArgs, errorHandler, originalExpr);
                    }
                    else if (symName == "verbose" || symName == "verbose?") {
                        // verbose和verbose?是原語函數
                        if (symName == "verbose") {
                            return EvalVerbose(cons->GetRight(), expr);
                        } else {
                            return EvalVerboseQuery(cons->GetRight(), expr);
                        }
                    }
                    else if (env.IsPrimitive(symName)) {
                        // It's a primitive function
                        NodePtr argsNode = cons->GetRight();
                        
                        // Check argument count
                        vector<NodePtr> argList = GetArgList(argsNode);
                        
                        int minArgs = primitives[symName].minArgs;
                        int maxArgs = primitives[symName].maxArgs;
                        
                        if ((minArgs >= 0 && argList.size() < minArgs) || 
                            (maxArgs >= 0 && argList.size() > maxArgs)) {
                            errorHandler.SetError("incorrect number of arguments", "", symName, originalExpr);
                            return nullptr;
                        }
                        
                        // Evaluate arguments
                        vector<NodePtr> evalArgs = EvaluateArgs(argsNode, originalExpr);
                        if (errorHandler.HasError()) {
                            return nullptr;
                        }
                        
                        // Apply function
                        return primitives[symName].func(evalArgs, errorHandler, originalExpr);
                    }
                    else {
                        // SYM is not a special form or primitive function
                        // Check if it's bound to a user-defined value
                        
                        // Look up the symbol binding
                        NodePtr binding = env.LookUp(symName, errorHandler);
                        
                        if (errorHandler.HasError()) {
                            // Symbol is unbound - error already set by LookUp
                            return nullptr;
                        }
                        
                        // Symbol is bound, check if it's bound to a function
                        if (binding->GetType() == ATOM_NODE) {
                            AtomNode* atom = dynamic_cast<AtomNode*>(binding.get());
                            
                            if (atom->GetAtomType() == SYMBOL && 
                                atom->GetValue().find("#<procedure ") == 0) {
                                // It's bound to a procedure
                                string funcName = atom->GetValue().substr(12, atom->GetValue().length() - 13);
                                
                                if (primitives.find(funcName) != primitives.end()) {
                                    // Evaluate arguments
                                    NodePtr argsNode = cons->GetRight();
                                    vector<NodePtr> evalArgs = EvaluateArgs(argsNode, originalExpr);
                                    
                                    if (errorHandler.HasError()) {
                                        return nullptr;
                                    }
                                    
                                    // Apply function
                                    return primitives[funcName].func(evalArgs, errorHandler, originalExpr);
                                } else {
                                    return ApplyUserFunction(binding, cons->GetRight(), originalExpr, funcName);
                                }
                            }
                            
                            // Symbol is bound, but not to a function
                            errorHandler.SetError("attempt to apply non-function", "", "", binding);
                            return nullptr;
                        }
                        else {
                            // Symbol is bound to a non-atom (like a list)
                            errorHandler.SetError("attempt to apply non-function", "", "", binding);
                            return nullptr;
                        }
                    }
                }
                else if (firstArg->GetType() == CONS_NODE) {
                    // The first argument of (...) is (...)
                    
                    // Evaluate (...)
                    NodePtr evaluatedFirstArg = Eval(firstArg, originalExpr);
    
                    if (errorHandler.HasError()) {
                        return nullptr;
                    }
                    
                    if (!evaluatedFirstArg) {
                        errorHandler.SetError("no return value", "", "", firstArg);
                        return nullptr;
                    }
                    
                    // 檢查是否為函數
                    if (evaluatedFirstArg->GetType() == ATOM_NODE) {
                        AtomNode* atom = dynamic_cast<AtomNode*>(evaluatedFirstArg.get());
                        string value = atom->GetValue();
                        
                        if (atom->GetAtomType() == SYMBOL && value.find("#<procedure ") == 0) {
                            // 提取函數名（如果有的話）
                            string funcName = "";
                            if (value == "#<procedure lambda>" || value.find("#<procedure lambda:") == 0) {
                                // 這是一個 lambda，應用它
                                return ApplyUserFunction(evaluatedFirstArg, cons->GetRight(), originalExpr);
                            }
                            if (value != "#<procedure lambda>") {
                                // 從 "#<procedure funcname>" 中提取 funcname
                                funcName = value.substr(12, value.length() - 13);
                            }
                            
                            // 檢查是否為內建函數
                            if (!funcName.empty() && primitives.find(funcName) != primitives.end()) {
                                // 處理內建函數
                                NodePtr argsNode = cons->GetRight();
                                vector<NodePtr> argList = GetArgList(argsNode);
                                
                                int minArgs = primitives[funcName].minArgs;
                                int maxArgs = primitives[funcName].maxArgs;
                                
                                if ((minArgs >= 0 && argList.size() < minArgs) || 
                                    (maxArgs >= 0 && argList.size() > maxArgs)) {
                                    errorHandler.SetError("incorrect number of arguments", "", funcName, originalExpr);
                                    return nullptr;
                                }
                                
                                vector<NodePtr> evalArgs = EvaluateArgs(argsNode, originalExpr);
                                if (errorHandler.HasError()) {
                                    return nullptr;
                                }
                                
                                return primitives[funcName].func(evalArgs, errorHandler, originalExpr);
                            }
                            else {
                                // 如果已經實現了 ApplyUserFunction，使用它
                                // return ApplyUserFunction(evaluatedFirstArg, cons->GetRight(), originalExpr, funcName);
                                
                                // 如果還沒實現，暫時返回錯誤
                                errorHandler.SetError("attempt to apply non-function", "", "", evaluatedFirstArg);
                                return nullptr;
                            }
                        }
                    }
                    
                    // 不是函數
                    errorHandler.SetError("attempt to apply non-function", "", "", evaluatedFirstArg);
                    return nullptr;
                }
            }
            return nullptr;
        }
        // 錯誤處理
        bool HasError() const {
            return errorHandler.HasError();
        }
        string GetErrorMessage() const {
            return errorHandler.GetErrorMessage();
        }
        void ClearError() {
            errorHandler.ClearError();
        }
        // 清除環境
        void CleanEnvironment() {
            env.Clean();
        }
        NodePtr GetErrorExpr() const {
            return errorHandler.GetErrorExpr();
        }
        NodePtr EvalQuote(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "quote", originalExpr);
                return nullptr;
            }
            
            ConsNode* cons = (ConsNode*)args.get();
            return cons->GetLeft();
        }

        NodePtr EvalDefine(NodePtr args, NodePtr originalExpr = nullptr) {
            // 1. Basic structure validation
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* argsCons = dynamic_cast<ConsNode*>(args.get());
            if (!argsCons) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            // Check if the args list is improperly dotted
            if (argsCons->IsDotted()) {
                errorHandler.SetError("non-list", "", "", originalExpr);
                return nullptr;
            }
        
            // 2. Symbol validation
            NodePtr symbolNode = argsCons->GetLeft();
            if (!symbolNode || symbolNode->GetType() != ATOM_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
        
            AtomNode* symbolAtom = dynamic_cast<AtomNode*>(symbolNode.get());
            if (!symbolAtom || symbolAtom->GetAtomType() != SYMBOL) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
        
            string symbol = symbolAtom->GetValue();
            
            // Check if trying to redefine a primitive or reserved keyword
            if (env.IsPrimitive(symbol) || symbol == "define" || symbol == "quote" || 
                symbol == "if" || symbol == "cond" || symbol == "begin" || 
                symbol == "and" || symbol == "or" || symbol == "clean-environment" || 
                symbol == "exit") {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
        
            // 3. Check that we have exactly one s-expression argument
            NodePtr rest = argsCons->GetRight();
            if (!rest || rest->GetType() != CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* restCons = dynamic_cast<ConsNode*>(rest.get());
            if (!restCons) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            NodePtr valueNode = restCons->GetLeft();
            NodePtr moreArgs = restCons->GetRight();
            
            // Ensure there are no extra arguments
            if (moreArgs && moreArgs->GetType() == CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            // 4. Check for nested define expressions (recursively)
            if (!validateNoNestedDefines(valueNode, errorHandler, originalExpr)) {
                return nullptr; // Error already set in validation function
            }
            
            // 5. Evaluate the expression and bind the result
            NodePtr value = Eval(valueNode, originalExpr);
            
            if (errorHandler.HasError()) {
                return nullptr;
            }
            if (!value) {
                errorHandler.SetError("no return value", "", "", valueNode);
                return nullptr;
            }
            
            return env.Define(symbol, value, errorHandler);
        }
        NodePtr EvalIf(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments)", "", "if", originalExpr);
                return nullptr;
            }

            int argCount = 0;
            NodePtr current = args;
            while (current && current->GetType() == CONS_NODE) {
                ConsNode* currentCons = (ConsNode*)current.get();
                argCount++;
                current = currentCons->GetRight();
            }
            
            if (argCount < 2 || argCount > 3) {
                errorHandler.SetError("incorrect number of arguments)", "", "if", originalExpr);
                return nullptr;
            }
            
            ConsNode* cons = (ConsNode*)args.get();
            NodePtr condNode = cons->GetLeft();
            NodePtr condResult = Eval(condNode, originalExpr);
            
            if (errorHandler.HasError()) {
                return nullptr;
            }

            if (!condResult) {
                errorHandler.SetError("unbound test-condition", "", "", condNode);
                return nullptr;
            }

            bool isTrue = true;
            if (condResult->GetType() == ATOM_NODE) {
                AtomNode* atom = (AtomNode*)condResult.get();
                if (atom && atom->GetAtomType() == NIL) {
                    isTrue = false;
                }
            }

            NodePtr thenNode = nullptr;
            NodePtr elseNode = nullptr;
            
            ConsNode* restCons = (ConsNode*)cons->GetRight().get();
            thenNode = restCons->GetLeft();

            if (argCount == 3) {
                ConsNode* elseRestCons = (ConsNode*)restCons->GetRight().get();
                elseNode = elseRestCons->GetLeft();
            }
            
            
            if (isTrue) {
                return Eval(thenNode, originalExpr);
            }
            
        
            if (argCount == 2) {
                errorHandler.SetError("no return value", "", "", originalExpr);
                return nullptr;
            }
            
            return Eval(elseNode, originalExpr);
        }
        NodePtr EvalCond(NodePtr args, NodePtr originalExpr = nullptr) {
            // 1. 檢查 COND 是否是一個非空列表
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("COND format", "", "", originalExpr);
                return nullptr;
            }
            
            // 2. 檢查每個子句的結構
            NodePtr currentClause = args;
            
            while (currentClause && currentClause->GetType() == CONS_NODE) {
                ConsNode* clauseCons = (ConsNode*)currentClause.get();
                NodePtr clause = clauseCons->GetLeft();
                
                // 確保子句是一個列表
                if (!clause || clause->GetType() != CONS_NODE) {
                    errorHandler.SetError("COND format", "", "", originalExpr);
                    return nullptr;
                }
                
                ConsNode* clauseContentCons = (ConsNode*)clause.get();
                
                // 檢查是否有至少一個結果表達式
                NodePtr resultExprs = clauseContentCons->GetRight();
                if (!resultExprs || resultExprs->GetType() != CONS_NODE) {
                    errorHandler.SetError("COND format", "", "", originalExpr);
                    return nullptr;
                }
                
                // 檢查結果表達式列表中的每個表達式
                NodePtr currentExpr = resultExprs;
                while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                    ConsNode* exprCons = (ConsNode*)currentExpr.get();
                    
                    // 結果表達式列表中不允許出現點對
                    if (exprCons->IsDotted()) {
                        errorHandler.SetError("COND format", "", "", originalExpr);
                        return nullptr;
                    }
                    
                    currentExpr = exprCons->GetRight();
                }
                
                currentClause = clauseCons->GetRight();
            }
            
            // 3. 求值 COND 子句
            currentClause = args;
            
            while (currentClause && currentClause->GetType() == CONS_NODE) {
                ConsNode* clauseCons = (ConsNode*)currentClause.get();
                NodePtr clause = clauseCons->GetLeft();
                ConsNode* clauseContentCons = (ConsNode*)clause.get();
                
                NodePtr test = clauseContentCons->GetLeft();
                NodePtr exprList = clauseContentCons->GetRight();
                
                // 檢查是否為 else 關鍵字
                bool isElseKeyword = false;
                NodePtr testResult = nullptr;
                
                if (test->GetType() == ATOM_NODE) {
                    AtomNode* atom = (AtomNode*)test.get();
                    if (atom->GetAtomType() == SYMBOL && atom->GetValue() == "else") {
                        // 檢查是否為最後一個子句
                        if (clauseCons->GetRight() && clauseCons->GetRight()->GetType() == CONS_NODE) {
                            // else 不在最後一個子句，當作普通符號
                            testResult = env.LookUp("else", errorHandler);
                            if (errorHandler.HasError()) {
                                return nullptr;
                            }
                        } else {
                            // else 在最後一個子句，視為必定為真
                            isElseKeyword = true;
                            testResult = make_shared<AtomNode>(T, "#t");
                        }
                    } else {
                        testResult = Eval(test, originalExpr);
                    }
                } else {
                    testResult = Eval(test, originalExpr);
                }
                
                if (errorHandler.HasError()) {
                    return nullptr;
                }
                
                if (!testResult) {
                    errorHandler.SetError("unbound test-condition", "", "", test);
                    return nullptr;
                }
                
                // 判斷條件結果
                bool testPassed = true;
                if (!isElseKeyword && testResult->GetType() == ATOM_NODE) {
                    AtomNode* atom = (AtomNode*)testResult.get();
                    if (atom->GetAtomType() == NIL) {
                        testPassed = false;
                    }
                }
                
                // 如果條件通過，執行並返回結果
                if (testPassed) {
                    NodePtr currentExpr = exprList;
                    NodePtr lastResult = nullptr;
                    
                    // 執行所有表達式
                    while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                        ConsNode* exprCons = (ConsNode*)currentExpr.get();
                        NodePtr nextExpr = exprCons->GetRight();
                        
                        // 評估當前表達式
                        NodePtr result = Eval(exprCons->GetLeft(), originalExpr);
                        
                        // 檢查是否是最後一個表達式
                        bool isLastExpr = (!nextExpr || nextExpr->GetType() != CONS_NODE);
                        
                        if (errorHandler.HasError()) {
                            // 如果是最後一個表達式，任何錯誤都要返回
                            if (isLastExpr) {
                                return nullptr;
                            }
                            // 如果不是最後一個表達式，只有 "no return value" 錯誤可以忽略
                            else if (errorHandler.GetErrorMessage().find("no return value") == 0) {
                                errorHandler.ClearError();
                            } else {
                                return nullptr;
                            }
                        }
                        
                        // 如果是最後一個表達式，返回其結果（可能是 nullptr）
                        if (isLastExpr) {
                            if (!result) {
                                errorHandler.SetError("no return value", "", "", originalExpr);
                                return nullptr;
                            }
                            return result;
                        }
                        
                        lastResult = result;
                        currentExpr = nextExpr;
                    }
                }
                
                currentClause = clauseCons->GetRight();
            }
            
            // 4. 如果沒有條件通過且沒有 else 子句，報錯
            errorHandler.SetError("no return value", "", "", originalExpr);
            return nullptr;
        }
        
        // 遞迴檢查 cond 結構是否合法
        bool ValidateCondStructure(NodePtr node, ErrorHandler& errorHandler, NodePtr originalExpr) {
            // 處理原子節點：原子節點總是合法的
            if (node->GetType() == ATOM_NODE) {
                return true;
            }
            
            // 處理列表節點
            if (node->GetType() == CONS_NODE) {
                ConsNode* cons = (ConsNode*)node.get();
                
                // 檢查是否為 dotted pair
                if (cons->IsDotted()) {
                    // cond 結構中不允許出現 dotted pair
                    errorHandler.SetError("non-list", "", "", node);
                    return false;
                }
                
                // 遞迴檢查左子節點
                if (cons->GetLeft() && !ValidateCondStructure(cons->GetLeft(), errorHandler, originalExpr)) {
                    return false;
                }
                
                // 遞迴檢查右子節點
                if (cons->GetRight() && !ValidateCondStructure(cons->GetRight(), errorHandler, originalExpr)) {
                    return false;
                }
            }
            
            return true;
        }
        NodePtr EvalBegin(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "begin", originalExpr);
                return nullptr;
            }
            
            // 檢查序列中是否包含特殊形式
            NodePtr currentExpr = args;
            NodePtr lastResult = nullptr;
            
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = (ConsNode*)currentExpr.get();
                NodePtr expr = exprCons->GetLeft();
                NodePtr nextExpr = exprCons->GetRight();
                
                // 檢查是否是特殊形式
                if (expr && expr->GetType() == CONS_NODE) {
                    ConsNode* innerCons = (ConsNode*)expr.get();
                    NodePtr op = innerCons->GetLeft();
                    
                    if (op && op->GetType() == ATOM_NODE) {
                        AtomNode* opAtom = (AtomNode*)op.get();
                        if (opAtom && opAtom->GetAtomType() == SYMBOL) {
                            string opName = opAtom->GetValue();
                            
                            if (opName == "define") {
                                errorHandler.SetError("level of DEFINE", "", "", originalExpr);
                                return nullptr;
                            }
                            else if (opName == "clean-environment") {
                                errorHandler.SetError("level of CLEAN-ENVIRONMENT", "", "", originalExpr);
                                return nullptr;
                            }
                            else if (opName == "exit") {
                                errorHandler.SetError("level of EXIT", "", "", originalExpr);
                                return nullptr;
                            }
                        }
                    }
                }
                
                // 評估當前表達式
                lastResult = Eval(expr, originalExpr);
                
                // 檢查是否是最後一個表達式
                bool isLastExpr = (!nextExpr || nextExpr->GetType() != CONS_NODE);
                
                if (errorHandler.HasError()) {
                    // 如果是最後一個表達式，任何錯誤都要返回
                    if (isLastExpr) {
                        return nullptr;
                    }
                    // 如果不是最後一個表達式，只有 "no return value" 錯誤可以忽略
                    else if (errorHandler.GetErrorMessage().find("no return value") == 0) {
                        errorHandler.ClearError();
                    } else {
                        return nullptr;
                    }
                }
                
                // 如果是最後一個表達式且沒有返回值
                if (isLastExpr && !lastResult) {
                    errorHandler.SetError("no return value", "", "", originalExpr);
                    return nullptr;
                }
                
                currentExpr = nextExpr;
            }
            
            return lastResult;
        }
        NodePtr EvalAnd(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "and", originalExpr);
                return nullptr;
            }
            
            NodePtr currentExpr = args;
            NodePtr lastResult = make_shared<AtomNode>(T, "#t");
            
            // 對每個Expr評估，如果有任一為false則立即返回false
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = (ConsNode*)currentExpr.get();
                lastResult = Eval(exprCons->GetLeft(), originalExpr);
                
                if (errorHandler.HasError()) {
                    return nullptr;  // 評估過程中發生錯誤
                }
                if (!lastResult) {
                    errorHandler.SetError("unbound condition", "", "", exprCons->GetLeft());
                    return nullptr;
                }
                
                // 檢查結果是否為false
                if (lastResult->GetType() == ATOM_NODE) {
                    AtomNode* atom = (AtomNode*)lastResult.get();
                    if (atom && atom->GetAtomType() == NIL) {
                        return lastResult;  
                    }
                }
                
                currentExpr = exprCons->GetRight();
            }
            
            // 所有Expr都為true，返回最後一個結果
            return lastResult;
        }
        NodePtr EvalOr(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "or", originalExpr);
                return nullptr;
            }
            NodePtr currentExpr = args;
            // 對每個Expr評估，如果有任一為true則立即返回該值
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = (ConsNode*)currentExpr.get();
                NodePtr result = Eval(exprCons->GetLeft(), originalExpr);
                
                if (errorHandler.HasError()) {
                    return nullptr;  // Expr評估過程中發生錯誤
                }
                if (!result) {
                    errorHandler.SetError("unbound condition", "", "", exprCons->GetLeft());
                    return nullptr;
                }
                
                // 檢查結果是否為true
                bool isTrue = true;
                if (result->GetType() == ATOM_NODE) {
                    AtomNode* atom = (AtomNode*)result.get();
                    if (atom && atom->GetAtomType() == NIL) {
                        isTrue = false;
                    }
                }
                
                if (isTrue) {
                    return result; 
                }
                
                currentExpr = exprCons->GetRight();
            }
            // 全部為false，返回nil
            return make_shared<AtomNode>(NIL, "nil");
        }
        NodePtr EvalLet(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("let format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* cons = dynamic_cast<ConsNode*>(args.get());
            NodePtr bindings = cons->GetLeft();
            NodePtr body = cons->GetRight();
            
            // 檢查body是否存在
            if (!body || body->GetType() != CONS_NODE) {
                errorHandler.SetError("let format", "", "", originalExpr);
                return nullptr;
            }
            
            // **修改：使用正確的環境管理方法**
            map<string, NodePtr> savedEnv = env.SaveState();
            
            // 收集所有綁定並在原環境中評估
            vector<pair<string, NodePtr>> evaluatedBindings;
            
            if (bindings && bindings->GetType() == CONS_NODE) {
                NodePtr current = bindings;
                while (current && current->GetType() == CONS_NODE) {
                    ConsNode* currentCons = dynamic_cast<ConsNode*>(current.get());
                    NodePtr binding = currentCons->GetLeft();
                    
                    // 驗證綁定格式
                    if (!binding || binding->GetType() != CONS_NODE) {
                        errorHandler.SetError("let format", "", "", originalExpr);
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    ConsNode* bindingCons = dynamic_cast<ConsNode*>(binding.get());
                    NodePtr symbol = bindingCons->GetLeft();
                    
                    // 驗證符號
                    if (!symbol || symbol->GetType() != ATOM_NODE) {
                        errorHandler.SetError("let format", "", "", originalExpr);
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    AtomNode* symbolAtom = dynamic_cast<AtomNode*>(symbol.get());
                    if (symbolAtom->GetAtomType() != SYMBOL) {
                        errorHandler.SetError("let format", "", "", originalExpr);
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    // 檢查綁定是否只有兩個元素
                    NodePtr valueExpr = bindingCons->GetRight();
                    if (!valueExpr || valueExpr->GetType() != CONS_NODE) {
                        errorHandler.SetError("let format", "", "", originalExpr);
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    ConsNode* valueExprCons = dynamic_cast<ConsNode*>(valueExpr.get());
                    if (valueExprCons->GetRight() && valueExprCons->GetRight()->GetType() == CONS_NODE) {
                        errorHandler.SetError("let format", "", "", originalExpr);
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    // **關鍵：在原環境中評估**
                    NodePtr value = Eval(valueExprCons->GetLeft(), originalExpr);
                    if (errorHandler.HasError()) {
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    if (!value) {
                        errorHandler.SetError("no return value", "", "", valueExprCons->GetLeft());
                        env.RestoreState(savedEnv);
                        return nullptr;
                    }
                    
                    // 收集綁定，但不立即加入環境
                    evaluatedBindings.push_back({symbolAtom->GetValue(), value});
                    
                    current = currentCons->GetRight();
                }
            } else if (bindings && bindings->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(bindings.get());
                if (atom->GetAtomType() != NIL) {
                    errorHandler.SetError("let format", "", "", originalExpr);
                    env.RestoreState(savedEnv);
                    return nullptr;
                }
            }
            
            // 一次性加入所有新綁定
            for (const auto& binding : evaluatedBindings) {
                env.Define(binding.first, binding.second, errorHandler);
            }
            
            // 在新環境中執行body
            NodePtr result = nullptr;
            NodePtr currentExpr = body;
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = dynamic_cast<ConsNode*>(currentExpr.get());
                result = Eval(exprCons->GetLeft(), originalExpr);
                
                if (errorHandler.HasError()) {
                    env.RestoreState(savedEnv);
                    return nullptr;
                }
                
                currentExpr = exprCons->GetRight();
            }
            
            // 恢復環境
            env.RestoreState(savedEnv);
            return result;
        }
        NodePtr EvalLambda(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* cons = dynamic_cast<ConsNode*>(args.get());
            NodePtr params = cons->GetLeft();
            NodePtr body = cons->GetRight();
            
            // 檢查 body 是否存在（至少要有一個表達式）
            if (!body || body->GetType() != CONS_NODE) {
                errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                return nullptr;
            }
            
            // 驗證參數列表格式
            vector<string> paramNames;
            if (params && params->GetType() == CONS_NODE) {
                // 檢查參數列表是否為合法列表（非 dotted pair）
                if (!NodeNormalizer::IsListNode(params)) {
                    errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                    return nullptr;
                }
                
                NodePtr currentParam = params;
                while (currentParam && currentParam->GetType() == CONS_NODE) {
                    ConsNode* paramCons = dynamic_cast<ConsNode*>(currentParam.get());
                    NodePtr param = paramCons->GetLeft();
                    
                    if (!param || param->GetType() != ATOM_NODE) {
                        errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                        return nullptr;
                    }
                    
                    AtomNode* paramAtom = dynamic_cast<AtomNode*>(param.get());
                    if (paramAtom->GetAtomType() != SYMBOL) {
                        errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                        return nullptr;
                    }
                    
                    paramNames.push_back(paramAtom->GetValue());
                    currentParam = paramCons->GetRight();
                }
            } else if (params && params->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(params.get());
                if (atom->GetAtomType() != NIL) {
                    errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                    return nullptr;
                }
            } else {
                // params 不是列表也不是 nil
                errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                return nullptr;
            }
            
            // 檢查 body 是否為合法列表
            if (!NodeNormalizer::IsListNode(body)) {
                errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                return nullptr;
            }
            
            // 收集函數體表達式
            vector<NodePtr> bodyExprs;
            NodePtr currentExpr = body;
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = dynamic_cast<ConsNode*>(currentExpr.get());
                bodyExprs.push_back(exprCons->GetLeft());
                currentExpr = exprCons->GetRight();
            }
            
            // 確保至少有一個 body 表達式
            if (bodyExprs.empty()) {
                errorHandler.SetError("LAMBDA format", "", "", originalExpr);
                return nullptr;
            }
            
            // 創建 lambda 信息
            auto lambdaInfo = make_shared<LambdaInfo>();
            lambdaInfo->parameters = paramNames;
            lambdaInfo->bodyExpressions = bodyExprs;
            
            // 移除變數捕獲邏輯
            // 不需要分析自由變數或捕獲綁定
            
            string lambdaId = "lambda_" + to_string(lambdaCounter++);
            lambdaInfo->lambdaId = lambdaId;
            lambdaFunctions[lambdaId] = lambdaInfo;
            
            return make_shared<AtomNode>(SYMBOL, "#<procedure lambda:" + lambdaId + ">");
        }
        NodePtr EvalFunctionDefine(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* cons = dynamic_cast<ConsNode*>(args.get());
            NodePtr funcDef = cons->GetLeft();
            NodePtr body = cons->GetRight();
            
            if (!funcDef || funcDef->GetType() != CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            ConsNode* funcDefCons = dynamic_cast<ConsNode*>(funcDef.get());
            NodePtr funcName = funcDefCons->GetLeft();
            NodePtr params = funcDefCons->GetRight();
            
            if (!funcName || funcName->GetType() != ATOM_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            AtomNode* funcNameAtom = dynamic_cast<AtomNode*>(funcName.get());
            if (funcNameAtom->GetAtomType() != SYMBOL) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            // 檢查參數列表格式
            if (params) {
                if (params->GetType() == CONS_NODE) {
                    // 檢查是否為合法列表（非 dotted pair）
                    if (!NodeNormalizer::IsListNode(params)) {
                        errorHandler.SetError("DEFINE format", "", "", originalExpr);
                        return nullptr;
                    }
                    
                    // 檢查參數是否都是符號，並收集參數名檢查重複
                    set<string> paramSet;
                    NodePtr currentParam = params;
                    while (currentParam && currentParam->GetType() == CONS_NODE) {
                        ConsNode* paramCons = dynamic_cast<ConsNode*>(currentParam.get());
                        NodePtr param = paramCons->GetLeft();
                        
                        if (!param || param->GetType() != ATOM_NODE) {
                            errorHandler.SetError("DEFINE format", "", "", originalExpr);
                            return nullptr;
                        }
                        
                        AtomNode* paramAtom = dynamic_cast<AtomNode*>(param.get());
                        if (paramAtom->GetAtomType() != SYMBOL) {
                            errorHandler.SetError("DEFINE format", "", "", originalExpr);
                            return nullptr;
                        }
                        
                        // 檢查參數名重複
                        string paramName = paramAtom->GetValue();
                        if (paramSet.find(paramName) != paramSet.end()) {
                            errorHandler.SetError("DEFINE format", "", "", originalExpr);
                            return nullptr;
                        }
                        paramSet.insert(paramName);
                        
                        currentParam = paramCons->GetRight();
                    }
                } else if (params->GetType() == ATOM_NODE) {
                    AtomNode* atom = dynamic_cast<AtomNode*>(params.get());
                    if (atom->GetAtomType() != NIL) {
                        errorHandler.SetError("DEFINE format", "", "", originalExpr);
                        return nullptr;
                    }
                } else {
                    errorHandler.SetError("DEFINE format", "", "", originalExpr);
                    return nullptr;
                }
            }
            
            // 檢查body不為空且至少有一個表達式
            if (!body || body->GetType() != CONS_NODE) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            // 檢查body是否為合法列表
            if (!NodeNormalizer::IsListNode(body)) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            // 確保body至少有一個表達式
            bool hasBodyExpr = false;
            NodePtr currentExpr = body;
            while (currentExpr && currentExpr->GetType() == CONS_NODE) {
                ConsNode* exprCons = dynamic_cast<ConsNode*>(currentExpr.get());
                if (exprCons->GetLeft()) {
                    hasBodyExpr = true;
                    break;
                }
                currentExpr = exprCons->GetRight();
            }
            
            if (!hasBodyExpr) {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }
            
            string symbolName = funcNameAtom->GetValue();
            
            // 檢查是否試圖重定義保留字
            if (env.IsPrimitive(symbolName) || symbolName == "define" || symbolName == "quote" || 
                symbolName == "if" || symbolName == "cond" || symbolName == "begin" || 
                symbolName == "and" || symbolName == "or" || symbolName == "clean-environment" || 
                symbolName == "exit" || symbolName == "lambda" || symbolName == "let") {
                errorHandler.SetError("DEFINE format", "", "", originalExpr);
                return nullptr;
            }

            // 清理舊的lambda函數
            for (auto it = lambdaFunctions.begin(); it != lambdaFunctions.end();) {
                if (it->first.find(symbolName + "_") == 0) {
                    it = lambdaFunctions.erase(it);
                } else {
                    ++it;
                }
            }
            
            // 構建lambda表達式
            NodePtr lambdaSymbol = make_shared<AtomNode>(SYMBOL, "lambda");
            NodePtr lambdaExpr = make_shared<ConsNode>(lambdaSymbol, 
                                                    make_shared<ConsNode>(params, body, false),
                                                    false);
            
            // 評估lambda表達式
            NodePtr lambdaValue = Eval(lambdaExpr, originalExpr);
            if (errorHandler.HasError()) {
                return nullptr;
            }
            
            // 創建自定義的函數表示
            NodePtr funcValue = make_shared<AtomNode>(SYMBOL, "#<procedure " + symbolName + ">");
            
            // 更新lambda函數的key
            if (!lambdaFunctions.empty()) {
                string oldKey = "lambda_" + to_string(lambdaCounter - 1);
                if (lambdaFunctions.find(oldKey) != lambdaFunctions.end()) {
                    auto lambdaInfo = lambdaFunctions[oldKey];
                    lambdaFunctions.erase(oldKey);
                    string newKey = symbolName + "_" + to_string(lambdaCounter - 1);
                    lambdaFunctions[newKey] = lambdaInfo;
                }
            }
            
            return env.Define(symbolName, funcValue, errorHandler);
        }
        NodePtr EvalVerbose(NodePtr args, NodePtr originalExpr = nullptr) {
            if (!args || args->GetType() != CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "verbose");
                return nullptr;
            }
            
            ConsNode* cons = dynamic_cast<ConsNode*>(args.get());
            NodePtr arg = cons->GetLeft();
            
            // 檢查是否只有一個參數
            if (cons->GetRight() && cons->GetRight()->GetType() == CONS_NODE) {
                errorHandler.SetError("incorrect number of arguments", "", "verbose");
                return nullptr;
            }
            
            NodePtr evaluatedArg = Eval(arg, originalExpr);
            if (errorHandler.HasError()) {
                return nullptr;
            }
            
            // 根據參數設置verbose模式
            if (evaluatedArg && evaluatedArg->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(evaluatedArg.get());
                verboseMode = (atom->GetAtomType() != NIL);
            }
            
            return verboseMode ? make_shared<AtomNode>(T, "#t") : make_shared<AtomNode>(NIL, "nil");
        }
        //處理verbose?函數
        NodePtr EvalVerboseQuery(NodePtr args, NodePtr originalExpr = nullptr) {
            // verbose?不應該有參數
            if (args && args->GetType() == CONS_NODE) {
                ConsNode* cons = dynamic_cast<ConsNode*>(args.get());
                if (cons->GetLeft()) {
                    errorHandler.SetError("incorrect number of arguments", "", "verbose?");
                    return nullptr;
                }
            }
            if (args && args->GetType() == ATOM_NODE) {
                AtomNode* atom = dynamic_cast<AtomNode*>(args.get());
                verboseMode = (atom->GetAtomType() != NIL);
            }
            
            return verboseMode ? make_shared<AtomNode>(T, "#t") : make_shared<AtomNode>(NIL, "nil");
        }
        //獲取verbose模式狀態
        bool IsVerboseMode() const {
            return verboseMode;
        }

};
class Printer {
public:
    // 列印S-Expr
    void PrintSExp(NodePtr expr) {
        if (!expr) {
            return;
        }

        expr->Print(0, 0);
        cout << endl;
    }
};

int main() {
    int testNum;
    cin >> testNum;

    char c;
    cin.get(c);

    cout << "Welcome to OurScheme!" << endl << endl;

    Parser parser;
    Printer printer;
    Evaluator evaluator;

    parser.Initialize();

    while (!parser.exit && !parser.IsEOF()) {
        cout << "> ";
        cout.flush();
        
        // 清除之前的錯誤
        evaluator.ClearError();
        
        parser.Initialize();
        NodePtr inSExp = parser.ReadSExp();
        
        if (inSExp == nullptr) {
            // 處理語法錯誤或EOF
            if (parser.IsEOF()) {
                break;
            }
            continue;
        }

        if (parser.exit) {
            break;  // 如果是(exit)命令，直接退出程式
        }
        NodePtr result = evaluator.Eval(inSExp);
        if (evaluator.HasError()) {
            string errorMsg = evaluator.GetErrorMessage();
            NodePtr errorExpr = evaluator.GetErrorExpr();
            
            // 輸出錯誤訊息
            cout << "ERROR (" << errorMsg;
            
            // 根據需要輸出錯誤表達式
            bool showExpr = false;
            
            // 檢查是否需要顯示表達式
            if (errorExpr && (errorMsg.find("attempt to apply non-function") == 0)) {
                // 對於 attempt to apply non-function 錯誤，顯示完整表達式
                errorExpr->Print(0, 0);
            } else if (errorExpr && 
                      (errorMsg.find("DEFINE format") >= 0 ||
                       errorMsg.find("non-list") >= 0 ||
                       errorMsg.find("with incorrect argument type") >= 0) &&
                      errorMsg.find("level of") != 0 &&
                      errorMsg.find("incorrect number of arguments") != 0) {
                cout << " : ";
                errorExpr->Print(0, 0);
            }
            
            if (errorExpr && showExpr) {
                cout << " : ";
                errorExpr->Print(0, 0);
            }
            
            cout << endl << endl;
        } else {
            if (!result) {
                cout << "ERROR (no return value) : ";
                inSExp->Print(0, 0);
                cout << endl << endl;
                continue;
            }
            bool isSpecialOutput = false;
            if (inSExp->GetType() == CONS_NODE) {
                ConsNode* cons = dynamic_cast<ConsNode*>(inSExp.get());
                NodePtr first = cons->GetLeft();
                if (first && first->GetType() == ATOM_NODE) {
                    AtomNode* atom = dynamic_cast<AtomNode*>(first.get());
                    if (atom->GetAtomType() == SYMBOL) {
                        string funcName = atom->GetValue();
                        if (funcName == "define" && evaluator.IsVerboseMode()) {
                            // result應該是AtomNode類型
                            if (result && result->GetType() == ATOM_NODE) {
                                AtomNode* atomResult = dynamic_pointer_cast<AtomNode>(result).get();
                                if (atomResult) {
                                    cout << atomResult->GetValue() << endl << endl;
                                    isSpecialOutput = true;
                                }
                            }
                        } else if (funcName == "clean-environment" && evaluator.IsVerboseMode()) {
                            // 輸出clean-environment成功消息
                            cout << "environment cleaned" << endl << endl;
                            isSpecialOutput = true;
                        }
                    }
                }
            }
            
            // 如果不是特殊輸出，正常輸出結果
            if (!isSpecialOutput) {
                printer.PrintSExp(result);
                cout << endl;
            }
        }

        parser.Initialize();
        
        // 檢查是否需要退出
        if (parser.exit) {
            break;
        }
    }

    if (parser.IsEOF() && !parser.exit) {
        cout << "ERROR (no more input) : END-OF-FILE encountered" << endl;
    }

    cout << "Thanks for using OurScheme!";

    return 0;
}