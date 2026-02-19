// Z# (Зи Шарп) v5.0 - ИСПРАВЛЕННАЯ версия для Cxxdroid
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <ctime>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>
#include <fstream>
#include <functional>
#include <numeric>
#include <climits>
#include <unistd.h>

using namespace std;
using namespace chrono;

struct Color {
    static const string RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN;
    static const string WHITE, BOLD, DIM, UNDERLINE, RESET;
    static const string BG_RED, BG_GREEN, BG_BLUE, BG_YELLOW;
};
const string Color::RED       = "\033[31m";
const string Color::GREEN     = "\033[32m";
const string Color::YELLOW    = "\033[33m";
const string Color::BLUE      = "\033[34m";
const string Color::MAGENTA   = "\033[35m";
const string Color::CYAN      = "\033[36m";
const string Color::WHITE     = "\033[37m";
const string Color::BOLD      = "\033[1m";
const string Color::DIM       = "\033[2m";
const string Color::UNDERLINE = "\033[4m";
const string Color::RESET     = "\033[0m";
const string Color::BG_RED    = "\033[41m";
const string Color::BG_GREEN  = "\033[42m";
const string Color::BG_BLUE   = "\033[44m";
const string Color::BG_YELLOW = "\033[43m";

enum class VarType { NUMBER, STRING_T, BOOL_T };

struct Variable {
    VarType type;
    double  number;
    string  str;
    bool    boolean;
    Variable() : type(VarType::NUMBER), number(0), boolean(false) {}
    Variable(double n) : type(VarType::NUMBER), number(n), boolean(false) {}
    Variable(string s) : type(VarType::STRING_T), number(0), str(s), boolean(false) {}
    Variable(bool b) : type(VarType::BOOL_T), number(0), boolean(b) {}
};

struct UserFunction {
    vector<string> params;
    vector<string> body;
};

class ZSharp {
private:
    map<string, Variable>     vars;
    map<string, UserFunction> functions;
    map<string, vector<double>> arrays;

    int    loop_counter;
    bool   debug_mode;
    bool   strict_mode;
    bool   running;
    double exec_time;
    int    line_count;
    int    error_count;
    string last_error;

    vector<string> call_stack;
    mt19937 rng;

    // ── утилиты ──────────────────────────────

    string trim(const string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    string toLower(string s) {
        transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    string toUpper(string s) {
        transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    vector<string> split(const string& s, char delim) {
        vector<string> result;
        stringstream ss(s);
        string token;
        while (getline(ss, token, delim))
            result.push_back(trim(token));
        return result;
    }

    string replaceAll(string str, const string& from, const string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != string::npos) {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
        return str;
    }

    void errMsg(const string& msg) {
        error_count++;
        last_error = msg;
        cout << Color::RED << Color::BOLD
             << "ERROR: " << msg
             << Color::RESET << "\n";
    }

    void warn(const string& msg) {
        cout << Color::YELLOW << "WARN: " << msg << Color::RESET << "\n";
    }

    void dbg(const string& msg) {
        if (debug_mode)
            cout << Color::DIM << "  [DBG] " << msg << Color::RESET << "\n";
    }

    string formatNum(double n, int dec = -1) {
        if (dec == -1) {
            if (n == (long long)n) return to_string((long long)n);
            ostringstream oss; oss << n; return oss.str();
        }
        ostringstream oss;
        oss << fixed << setprecision(dec) << n;
        return oss.str();
    }

    // ── вычисление выражений ─────────────────

    double tokenValue(const string& tok) {
        string t = trim(tok);
        if (t == "true")  return 1;
        if (t == "false") return 0;
        if (t == "null")  return 0;
        if (vars.count(t)) {
            if (vars[t].type == VarType::NUMBER) return vars[t].number;
            if (vars[t].type == VarType::BOOL_T) return vars[t].boolean ? 1.0 : 0.0;
        }
        try { return stod(t); } catch (...) {}
        return 0;
    }

    double evalExpr(const string& raw) {
        string expr = trim(raw);
        if (expr.empty()) return 0;
        if (vars.count(expr) && vars[expr].type == VarType::STRING_T) return 0;

        // Одноаргументные функции
        struct Fn1 { string name; function<double(double)> fn; };
        vector<Fn1> fn1list = {
            {"sqrt",  [](double x){ return sqrt(x); }},
            {"abs",   [](double x){ return fabs(x); }},
            {"floor", [](double x){ return floor(x); }},
            {"ceil",  [](double x){ return ceil(x); }},
            {"round", [](double x){ return round(x); }},
            {"sin",   [](double x){ return sin(x); }},
            {"cos",   [](double x){ return cos(x); }},
            {"tan",   [](double x){ return tan(x); }},
            {"asin",  [](double x){ return asin(x); }},
            {"acos",  [](double x){ return acos(x); }},
            {"atan",  [](double x){ return atan(x); }},
            {"log",   [](double x){ return log(x); }},
            {"log2",  [](double x){ return log2(x); }},
            {"log10", [](double x){ return log10(x); }},
            {"exp",   [](double x){ return exp(x); }},
            {"neg",   [](double x){ return -x; }},
            {"sign",  [](double x){ return (double)((x>0)-(x<0)); }},
            {"frac",  [](double x){ return x - floor(x); }},
            {"deg",   [](double x){ return x * 180.0 / M_PI; }},
            {"rad",   [](double x){ return x * M_PI / 180.0; }},
            {"fact",  [](double x){
                double r=1; for(int i=2;i<=(int)x;i++) r*=i; return r;
            }},
        };
        for (auto& f : fn1list) {
            string pref = f.name + "(";
            if (expr.substr(0, pref.size()) == pref && expr.back() == ')') {
                string arg = expr.substr(pref.size(), expr.size()-pref.size()-1);
                return f.fn(evalExpr(arg));
            }
        }

        // Двухаргументные функции
        struct Fn2 { string name; function<double(double,double)> fn; };
        vector<Fn2> fn2list = {
            {"pow",   [](double a,double b){ return pow(a,b); }},
            {"max",   [](double a,double b){ return a>b?a:b; }},
            {"min",   [](double a,double b){ return a<b?a:b; }},
            {"mod",   [](double a,double b){ return fmod(a,b); }},
            {"hypot", [](double a,double b){ return hypot(a,b); }},
            {"atan2", [](double a,double b){ return atan2(a,b); }},
        };
        for (auto& f : fn2list) {
            string pref = f.name + "(";
            if (expr.substr(0, pref.size()) == pref && expr.back() == ')') {
                string inside = expr.substr(pref.size(), expr.size()-pref.size()-1);
                auto parts = split(inside, ',');
                if (parts.size() == 2)
                    return f.fn(evalExpr(parts[0]), evalExpr(parts[1]));
            }
        }

        // rand(a,b)
        if (expr.size() > 5 && expr.substr(0,5) == "rand(" && expr.back()==')') {
            string inside = expr.substr(5, expr.size()-6);
            auto parts = split(inside, ',');
            if (parts.size() == 2) {
                int a = (int)evalExpr(parts[0]);
                int b = (int)evalExpr(parts[1]);
                uniform_int_distribution<int> dist(a,b);
                return dist(rng);
            }
        }

        // randf(a,b)
        if (expr.size() > 6 && expr.substr(0,6) == "randf(" && expr.back()==')') {
            string inside = expr.substr(6, expr.size()-7);
            auto parts = split(inside, ',');
            if (parts.size() == 2) {
                double a = evalExpr(parts[0]);
                double b = evalExpr(parts[1]);
                uniform_real_distribution<double> dist(a,b);
                return dist(rng);
            }
        }

        // len(x)
        if (expr.size() > 4 && expr.substr(0,4) == "len(" && expr.back()==')') {
            string name = trim(expr.substr(4, expr.size()-5));
            if (arrays.count(name)) return (double)arrays[name].size();
            if (vars.count(name) && vars[name].type == VarType::STRING_T)
                return (double)vars[name].str.size();
        }

        // sum(arr)
        if (expr.size() > 4 && expr.substr(0,4) == "sum(" && expr.back()==')') {
            string name = trim(expr.substr(4, expr.size()-5));
            if (arrays.count(name)) {
                double s = 0;
                for (double v : arrays[name]) s += v;
                return s;
            }
        }

        // avg(arr)
        if (expr.size() > 4 && expr.substr(0,4) == "avg(" && expr.back()==')') {
            string name = trim(expr.substr(4, expr.size()-5));
            if (arrays.count(name) && !arrays[name].empty()) {
                double s = 0;
                for (double v : arrays[name]) s += v;
                return s / arrays[name].size();
            }
        }

        // arr[i]
        {
            size_t lb = expr.find('[');
            size_t rb = expr.find(']');
            if (lb != string::npos && rb != string::npos && rb > lb) {
                string aname = trim(expr.substr(0, lb));
                int idx = (int)evalExpr(expr.substr(lb+1, rb-lb-1));
                if (arrays.count(aname) && idx >= 0 && idx < (int)arrays[aname].size())
                    return arrays[aname][idx];
                return 0;
            }
        }

        // Скобки
        if (expr.size() >= 2 && expr.front()=='(' && expr.back()==')') {
            return evalExpr(expr.substr(1, expr.size()-2));
        }

        // Унарный минус
        if (expr.front()=='-' && expr.size() > 1) {
            return -evalExpr(expr.substr(1));
        }

        // Бинарные операции (правоассоциативно, + и -)
        int depth = 0;
        for (int i = (int)expr.size()-1; i >= 1; i--) {
            char c = expr[i];
            if (c == ')') depth++;
            else if (c == '(') depth--;
            if (depth == 0 && (c == '+' || c == '-')) {
                return (c=='+')
                    ? evalExpr(expr.substr(0,i)) + evalExpr(expr.substr(i+1))
                    : evalExpr(expr.substr(0,i)) - evalExpr(expr.substr(i+1));
            }
        }

        // * / %
        depth = 0;
        for (int i = (int)expr.size()-1; i >= 1; i--) {
            char c = expr[i];
            if (c == ')') depth++;
            else if (c == '(') depth--;
            if (depth == 0 && (c=='*' || c=='/' || c=='%')) {
                double L = evalExpr(expr.substr(0,i));
                double R = evalExpr(expr.substr(i+1));
                if (c=='*') return L*R;
                if (c=='/') return R!=0 ? L/R : 0;
                return fmod(L,R);
            }
        }

        // ^
        depth = 0;
        for (int i = (int)expr.size()-1; i >= 1; i--) {
            char c = expr[i];
            if (c == ')') depth++;
            else if (c == '(') depth--;
            if (depth == 0 && c == '^')
                return pow(evalExpr(expr.substr(0,i)), evalExpr(expr.substr(i+1)));
        }

        return tokenValue(expr);
    }

    // ── условия ──────────────────────────────

    bool evalCond(const string& cond) {
        string c = trim(cond);
        size_t pos;

        if ((pos = c.find(" and ")) != string::npos)
            return evalCond(c.substr(0,pos)) && evalCond(c.substr(pos+5));
        if ((pos = c.find(" or ")) != string::npos)
            return evalCond(c.substr(0,pos)) || evalCond(c.substr(pos+4));
        if (c.size()>4 && c.substr(0,4) == "not ")
            return !evalCond(c.substr(4));

        // Операторы сравнения
        vector<pair<string,function<bool(double,double)>>> ops = {
            {">=", [](double a,double b){ return a>=b; }},
            {"<=", [](double a,double b){ return a<=b; }},
            {"!=", [](double a,double b){ return a!=b; }},
            {"==", [](double a,double b){ return a==b; }},
            {">",  [](double a,double b){ return a>b;  }},
            {"<",  [](double a,double b){ return a<b;  }},
        };
        for (auto& op : ops) {
            pos = c.find(op.first);
            if (pos != string::npos) {
                double L = evalExpr(trim(c.substr(0, pos)));
                double R = evalExpr(trim(c.substr(pos + op.first.size())));
                return op.second(L, R);
            }
        }
        return evalExpr(c) != 0;
    }

    // ── строки ───────────────────────────────

    string evalString(const string& raw) {
        string s = trim(raw);
        if (s.size() >= 2 && s.front()=='"' && s.back()=='"')
            return s.substr(1, s.size()-2);
        if (vars.count(s) && vars[s].type == VarType::STRING_T) return vars[s].str;
        if (vars.count(s) && vars[s].type == VarType::NUMBER)
            return formatNum(vars[s].number);

        if (s.size()>6 && s.substr(0,6)=="upper(")
            return toUpper(evalString(s.substr(6, s.size()-7)));
        if (s.size()>6 && s.substr(0,6)=="lower(")
            return toLower(evalString(s.substr(6, s.size()-7)));
        if (s.size()>8 && s.substr(0,8)=="reverse(") {
            string r = evalString(s.substr(8, s.size()-9));
            std::reverse(r.begin(), r.end());
            return r;
        }
        if (s.size()>5 && s.substr(0,5)=="trim(")
            return trim(evalString(s.substr(5, s.size()-6)));

        return s;
    }

    string processColors(string text) {
        map<string,string> colors = {
            {"{red}",     Color::RED},
            {"{green}",   Color::GREEN},
            {"{yellow}",  Color::YELLOW},
            {"{blue}",    Color::BLUE},
            {"{magenta}", Color::MAGENTA},
            {"{cyan}",    Color::CYAN},
            {"{white}",   Color::WHITE},
            {"{bold}",    Color::BOLD},
            {"{dim}",     Color::DIM},
            {"{under}",   Color::UNDERLINE},
            {"{reset}",   Color::RESET},
            {"{bg_red}",  Color::BG_RED},
            {"{bg_green}",Color::BG_GREEN},
            {"{bg_blue}", Color::BG_BLUE},
        };
        for (auto& kv : colors)
            text = replaceAll(text, kv.first, kv.second);
        return text;
    }

    string interpolate(string text) {
        size_t pos = 0;
        while ((pos = text.find('{', pos)) != string::npos) {
            size_t end = text.find('}', pos);
            if (end == string::npos) break;
            string key = text.substr(pos+1, end-pos-1);
            // Пропустить цветовые теги
            bool colorTag = (key=="red"||key=="green"||key=="yellow"||
                             key=="blue"||key=="magenta"||key=="cyan"||
                             key=="white"||key=="bold"||key=="dim"||
                             key=="under"||key=="reset"||
                             key.substr(0,3)=="bg_");
            if (colorTag) { pos = end+1; continue; }

            if (vars.count(key)) {
                string val;
                if (vars[key].type == VarType::NUMBER)
                    val = formatNum(vars[key].number);
                else if (vars[key].type == VarType::STRING_T)
                    val = vars[key].str;
                else
                    val = vars[key].boolean ? "true" : "false";
                text.replace(pos, end-pos+1, val);
                pos += val.size();
            } else {
                pos = end+1;
            }
        }
        return text;
    }

    string getArgs(const string& line) {
        size_t lp = line.find('(');
        size_t rp = line.rfind(')');
        if (lp==string::npos || rp==string::npos) return "";
        return trim(line.substr(lp+1, rp-lp-1));
    }

    // ── константы ────────────────────────────

    void initConstants() {
        vars["pi"]      = Variable(3.141592653589793);
        vars["e"]       = Variable(2.718281828459045);
        vars["phi"]     = Variable(1.618033988749895);
        vars["million"] = Variable(1000000.0);
        vars["billion"] = Variable(1000000000.0);
        vars["inf"]     = Variable(1e300);
        vars["true"]    = Variable(true);
        vars["false"]   = Variable(false);
    }

    // ── выполнение одной строки ───────────────

    string executeLine(const string& raw, int depth = 0) {
        string l = trim(raw);
        if (l.empty() || l[0] == '#') return "";

        // Убираем инлайн-комментарий
        {
            size_t cc = l.find("  #");
            if (cc != string::npos) l = trim(l.substr(0, cc));
        }

        dbg("exec: " + l);
        line_count++;

        // ════════ ВЫВОД ════════════════════════

        // say
        if (l.size() > 3 && l.substr(0,3) == "say") {
            string rest = trim(l.substr(3));
            // убрать обёртку ()
            if (!rest.empty() && rest.front()=='(') {
                rest = rest.substr(1);
                if (!rest.empty() && rest.back()==')') rest.pop_back();
            }
            // убрать кавычки
            if (rest.size()>=2 && rest.front()=='"' && rest.back()=='"')
                rest = rest.substr(1, rest.size()-2);

            rest = interpolate(processColors(rest));

            // $( expr ) — вычислить выражение внутри строки
            size_t pos = 0;
            while ((pos = rest.find("$(")) != string::npos) {
                size_t end = rest.find(')', pos);
                if (end == string::npos) break;
                string eStr = rest.substr(pos+2, end-pos-2);
                string val  = formatNum(evalExpr(eStr));
                rest.replace(pos, end-pos+1, val);
            }

            cout << rest << Color::RESET << "\n";
            return "";
        }

        // s(text) — совместимость
        if (l.size()>2 && l.substr(0,2)=="s(" && l.back()==')') {
            string text = l.substr(2, l.size()-3);
            if (text.size()>=2 && text.front()=='"' && text.back()=='"')
                text = text.substr(1, text.size()-2);
            text = interpolate(processColors(text));
            cout << text << Color::RESET << "\n";
            return "";
        }

        // print
        if (l.size()>5 && l.substr(0,5)=="print") {
            string rest = trim(l.substr(5));
            if (arrays.count(rest)) {
                cout << rest << " = [";
                for (size_t i=0;i<arrays[rest].size();i++) {
                    if (i) cout << ", ";
                    cout << formatNum(arrays[rest][i]);
                }
                cout << "]\n";
                return "";
            }
            if (vars.count(rest) && vars[rest].type==VarType::STRING_T) {
                cout << vars[rest].str << "\n"; return "";
            }
            if (vars.count(rest) && vars[rest].type==VarType::BOOL_T) {
                cout << (vars[rest].boolean?"true":"false") << "\n"; return "";
            }
            cout << formatNum(evalExpr(rest)) << "\n";
            return "";
        }

        // printfmt x dec
        if (l.size()>8 && l.substr(0,8)=="printfmt") {
            auto parts = split(l.substr(8), ' ');
            if (parts.size()==2)
                cout << formatNum(evalExpr(parts[0]), (int)evalExpr(parts[1])) << "\n";
            return "";
        }

        // printtable arr
        if (l.size()>10 && l.substr(0,10)=="printtable") {
            string name = trim(l.substr(10));
            if (arrays.count(name)) {
                cout << "+-----+------------+\n";
                cout << "|  i  |   value    |\n";
                cout << "+-----+------------+\n";
                for (size_t i=0;i<arrays[name].size();i++) {
                    cout << "|" << setw(5) << i
                         << "|" << setw(12) << formatNum(arrays[name][i]) << "|\n";
                }
                cout << "+-----+------------+\n";
            }
            return "";
        }

        // putchar code
        if (l.size()>7 && l.substr(0,7)=="putchar") {
            cout << (char)(int)evalExpr(trim(l.substr(7)));
            return "";
        }

        // newline / tab / separator
        if (l=="newline")   { cout << "\n"; return ""; }
        if (l=="tab")       { cout << "\t"; return ""; }
        if (l=="separator") { cout << string(40, '-') << "\n"; return ""; }

        // banner
        if (l.size()>6 && l.substr(0,6)=="banner") {
            string text = trim(l.substr(6));
            if (!text.empty() && text.front()=='"') text=text.substr(1,text.size()-2);
            int len = (int)text.size() + 4;
            string border(len, '=');
            cout << Color::BOLD << Color::CYAN
                 << border << "\n= " << text << " =\n"
                 << border << Color::RESET << "\n";
            return "";
        }

        // header
        if (l.size()>6 && l.substr(0,6)=="header") {
            string text = trim(l.substr(6));
            if (!text.empty() && text.front()=='"') text=text.substr(1,text.size()-2);
            cout << Color::BOLD << Color::YELLOW
                 << "\n== " << text << " ==\n"
                 << Color::RESET;
            return "";
        }

        // progress cur max
        if (l.size()>8 && l.substr(0,8)=="progress") {
            auto parts = split(l.substr(8), ' ');
            if (parts.size()==2) {
                int cur = (int)evalExpr(parts[0]);
                int mx  = (int)evalExpr(parts[1]);
                int pct = (mx>0) ? cur*100/mx : 0;
                int bars = pct/5;
                cout << "[";
                for (int i=0;i<20;i++) cout << (i<bars ? "#" : ".");
                cout << "] " << pct << "%\n";
            }
            return "";
        }

        // warn / error messages
        if (l.size()>4 && l.substr(0,4)=="warn") {
            string text = trim(l.substr(4));
            if (!text.empty() && text.front()=='"') text=text.substr(1,text.size()-2);
            warn(text); return "";
        }
        if (l.size()>5 && l.substr(0,5)=="error" &&
            l.size()>6 && l[5]==' ') {
            string text = trim(l.substr(5));
            if (!text.empty() && text.front()=='"') text=text.substr(1,text.size()-2);
            errMsg(text); return "";
        }

        // ════════ ВВОД ═════════════════════════

        // input x
        if (l.size()>5 && l.substr(0,5)=="input") {
            string var = trim(l.substr(5));
            cout << "> "; string ui; getline(cin, ui);
            try { vars[var] = Variable(stod(ui)); }
            catch (...) { vars[var] = Variable(ui); }
            return "";
        }

        // ask "вопрос?" -> x
        if (l.size()>3 && l.substr(0,3)=="ask") {
            size_t arrow = l.find("->");
            if (arrow != string::npos) {
                string prompt = trim(l.substr(3, arrow-3));
                string var    = trim(l.substr(arrow+2));
                if (!prompt.empty() && prompt.front()=='"')
                    prompt = prompt.substr(1, prompt.size()-2);
                cout << prompt << " ";
                string ui; getline(cin, ui);
                try { vars[var] = Variable(stod(ui)); }
                catch (...) { vars[var] = Variable(ui); }
            }
            return "";
        }

        // ════════ ПЕРЕМЕННЫЕ ═══════════════════

        // let / set / var x = expr
        bool isLet = (l.size()>3 && (l.substr(0,3)=="let"||l.substr(0,3)=="set"||l.substr(0,3)=="var"));
        if (isLet) {
            size_t eq = l.find('=');
            if (eq != string::npos) {
                string name = trim(l.substr(3, eq-3));
                string expr = trim(l.substr(eq+1));
                if (!expr.empty() && expr.front()=='"')
                    vars[name] = Variable(evalString(expr));
                else if (expr=="true"||expr=="false")
                    vars[name] = Variable(expr=="true");
                else
                    vars[name] = Variable(evalExpr(expr));
                dbg(name + " = " + formatNum(vars[name].number));
            }
            return "";
        }

        // inc x / inc x by N
        if (l.size()>3 && l.substr(0,3)=="inc") {
            string rest = trim(l.substr(3));
            size_t by = rest.find(" by ");
            if (by != string::npos) {
                string name = trim(rest.substr(0,by));
                vars[name].number += evalExpr(rest.substr(by+4));
            } else {
                vars[rest].number += 1;
            }
            return "";
        }

        // dec x / dec x by N
        if (l.size()>3 && l.substr(0,3)=="dec") {
            string rest = trim(l.substr(3));
            size_t by = rest.find(" by ");
            if (by != string::npos) {
                string name = trim(rest.substr(0,by));
                vars[name].number -= evalExpr(rest.substr(by+4));
            } else {
                vars[rest].number -= 1;
            }
            return "";
        }

        // mul x by N
        if (l.size()>3 && l.substr(0,3)=="mul") {
            string rest = trim(l.substr(3));
            size_t by = rest.find(" by ");
            if (by != string::npos) {
                string name = trim(rest.substr(0,by));
                vars[name].number *= evalExpr(rest.substr(by+4));
            }
            return "";
        }

        // div x by N
        if (l.size()>3 && l.substr(0,3)=="div") {
            string rest = trim(l.substr(3));
            size_t by = rest.find(" by ");
            if (by != string::npos) {
                string name = trim(rest.substr(0,by));
                double d = evalExpr(rest.substr(by+4));
                if (d!=0) vars[name].number /= d;
                else errMsg("Division by zero");
            }
            return "";
        }

        // swap x y
        if (l.size()>4 && l.substr(0,4)=="swap") {
            auto parts = split(l.substr(4), ' ');
            if (parts.size()==2) {
                Variable tmp = vars[parts[0]];
                vars[parts[0]] = vars[parts[1]];
                vars[parts[1]] = tmp;
            }
            return "";
        }

        // delete x
        if (l.size()>6 && l.substr(0,6)=="delete") {
            string name = trim(l.substr(6));
            vars.erase(name); arrays.erase(name);
            return "";
        }

        // typeof x
        if (l.size()>6 && l.substr(0,6)=="typeof") {
            string name = trim(l.substr(6));
            string t = "undefined";
            if (vars.count(name)) {
                if (vars[name].type==VarType::NUMBER)   t="number";
                if (vars[name].type==VarType::STRING_T) t="string";
                if (vars[name].type==VarType::BOOL_T)   t="bool";
            } else if (arrays.count(name)) t="array";
            cout << name << " : " << t << "\n";
            vars["__type"] = Variable(t);
            return "";
        }

        // cast x to num/str/bool
        if (l.size()>4 && l.substr(0,4)=="cast") {
            auto parts = split(l.substr(4), ' ');
            if (parts.size()==3 && parts[1]=="to" && vars.count(parts[0])) {
                string name=parts[0], type=parts[2];
                if (type=="num") {
                    double v=0;
                    if (vars[name].type==VarType::STRING_T) {
                        try { v=stod(vars[name].str); } catch(...) {}
                    } else if (vars[name].type==VarType::BOOL_T) {
                        v = vars[name].boolean ? 1:0;
                    }
                    vars[name]=Variable(v);
                } else if (type=="str") {
                    string s = (vars[name].type==VarType::NUMBER)
                               ? formatNum(vars[name].number)
                               : (vars[name].boolean?"true":"false");
                    vars[name]=Variable(s);
                } else if (type=="bool") {
                    bool b = (vars[name].type==VarType::NUMBER)
                             ? (vars[name].number!=0)
                             : (!vars[name].str.empty());
                    vars[name]=Variable(b);
                }
            }
            return "";
        }

        // ════════ МАССИВЫ ══════════════════════

        // array name = ...
        if (l.size()>5 && l.substr(0,5)=="array") {
            size_t eq = l.find('=');
            if (eq != string::npos) {
                string name = trim(l.substr(5, eq-5));
                string rest = trim(l.substr(eq+1));

                if (rest.size()>5 && rest.substr(0,5)=="range") {
                    string inside = rest.substr(6, rest.size()-7);
                    auto parts = split(inside, ',');
                    if (parts.size()>=2) {
                        int from=(int)evalExpr(parts[0]);
                        int to  =(int)evalExpr(parts[1]);
                        int step=(parts.size()==3)?(int)evalExpr(parts[2]):1;
                        vector<double> arr;
                        for (int i=from;i<=to;i+=step) arr.push_back(i);
                        arrays[name]=arr;
                    }
                }
                else if (rest.size()>5 && rest.substr(0,5)=="zeros") {
                    int n=(int)evalExpr(rest.substr(6,rest.size()-7));
                    arrays[name]=vector<double>(n,0);
                }
                else if (rest.size()>4 && rest.substr(0,4)=="ones") {
                    int n=(int)evalExpr(rest.substr(5,rest.size()-6));
                    arrays[name]=vector<double>(n,1);
                }
                else if (rest.size()>7 && rest.substr(0,7)=="randArr") {
                    string inside=rest.substr(8,rest.size()-9);
                    auto parts=split(inside,',');
                    if (parts.size()==3) {
                        int n=(int)evalExpr(parts[0]);
                        int lo=(int)evalExpr(parts[1]);
                        int hi=(int)evalExpr(parts[2]);
                        uniform_int_distribution<int> dist(lo,hi);
                        vector<double> arr;
                        for (int i=0;i<n;i++) arr.push_back(dist(rng));
                        arrays[name]=arr;
                    }
                }
                else if (!rest.empty() && rest.front()=='[') {
                    string inside=rest.substr(1,rest.size()-2);
                    auto parts=split(inside,',');
                    vector<double> arr;
                    for (auto& p:parts) arr.push_back(evalExpr(p));
                    arrays[name]=arr;
                }
            }
            return "";
        }

        // push arr val
        if (l.size()>4 && l.substr(0,4)=="push") {
            auto parts = split(l.substr(4), ' ');
            if (parts.size()==2)
                arrays[parts[0]].push_back(evalExpr(parts[1]));
            return "";
        }

        // pop arr
        if (l.size()>3 && l.substr(0,3)=="pop") {
            string name=trim(l.substr(3));
            if (arrays.count(name) && !arrays[name].empty()) {
                vars["popped"]=Variable(arrays[name].back());
                arrays[name].pop_back();
            }
            return "";
        }

        // arr[i] = val
        {
            size_t lb=l.find('['), rb=l.find(']'), eq=l.find('=');
            if (lb!=string::npos && rb!=string::npos &&
                eq!=string::npos && eq>rb) {
                string aname=trim(l.substr(0,lb));
                int idx=(int)evalExpr(l.substr(lb+1,rb-lb-1));
                double val=evalExpr(l.substr(eq+1));
                if (arrays.count(aname)&&idx>=0&&idx<(int)arrays[aname].size())
                    arrays[aname][idx]=val;
                return "";
            }
        }

        // sort / sortdesc / shuffle / reverse arr
        if (l.size()>4 && l.substr(0,4)=="sort") {
            string name=trim(l.substr(4));
            if (arrays.count(name))
                std::sort(arrays[name].begin(),arrays[name].end());
            return "";
        }
        if (l.size()>8 && l.substr(0,8)=="sortdesc") {
            string name=trim(l.substr(8));
            if (arrays.count(name))
                std::sort(arrays[name].begin(),arrays[name].end(),
                          [](double a,double b){ return a>b; });
            return "";
        }
        if (l.size()>7 && l.substr(0,7)=="shuffle") {
            string name=trim(l.substr(7));
            if (arrays.count(name))
                std::shuffle(arrays[name].begin(),arrays[name].end(),rng);
            return "";
        }
        if (l.size()>7 && l.substr(0,7)=="reverse") {
            string name=trim(l.substr(7));
            if (arrays.count(name))
                std::reverse(arrays[name].begin(),arrays[name].end());
            return "";
        }

        // fill arr val
        if (l.size()>4 && l.substr(0,4)=="fill") {
            auto parts=split(l.substr(4),' ');
            if (parts.size()==2 && arrays.count(parts[0]))
                fill(arrays[parts[0]].begin(),arrays[parts[0]].end(),
                     evalExpr(parts[1]));
            return "";
        }

        // findmax arr -> var
        if (l.size()>7 && l.substr(0,7)=="findmax") {
            size_t arrow=l.find("->");
            if (arrow!=string::npos) {
                string name=trim(l.substr(7,arrow-7));
                string rv  =trim(l.substr(arrow+2));
                if (arrays.count(name) && !arrays[name].empty())
                    vars[rv]=Variable(*max_element(
                        arrays[name].begin(),arrays[name].end()));
            }
            return "";
        }

        // findmin arr -> var
        if (l.size()>7 && l.substr(0,7)=="findmin") {
            size_t arrow=l.find("->");
            if (arrow!=string::npos) {
                string name=trim(l.substr(7,arrow-7));
                string rv  =trim(l.substr(arrow+2));
                if (arrays.count(name) && !arrays[name].empty())
                    vars[rv]=Variable(*min_element(
                        arrays[name].begin(),arrays[name].end()));
            }
            return "";
        }

        // ════════ МАТЕМАТИКА ═══════════════════

        // calc expr -> var  или  calc expr
        if (l.size()>4 && l.substr(0,4)=="calc") {
            size_t arrow=l.find("->");
            if (arrow!=string::npos) {
                string expr=trim(l.substr(4,arrow-4));
                string var =trim(l.substr(arrow+2));
                vars[var]=Variable(evalExpr(expr));
            } else {
                cout << "= " << formatNum(evalExpr(trim(l.substr(4)))) << "\n";
            }
            return "";
        }

        // solve a b  (ax+b=0)
        if (l.size()>5 && l.substr(0,5)=="solve") {
            auto parts=split(l.substr(5),' ');
            if (parts.size()==2) {
                double a=evalExpr(parts[0]), b=evalExpr(parts[1]);
                if (a!=0) {
                    double x=-b/a;
                    cout << "x = " << formatNum(x) << "\n";
                    vars["x"]=Variable(x);
                } else errMsg("a=0, no solution");
            }
            return "";
        }

        // gcd a b -> var
        if (l.size()>3 && l.substr(0,3)=="gcd") {
            size_t arrow=l.find("->");
            string rest=(arrow!=string::npos)?l.substr(3,arrow-3):l.substr(3);
            auto parts=split(rest,' ');
            if (parts.size()>=2) {
                long long a=(long long)evalExpr(parts[0]);
                long long b=(long long)evalExpr(parts[1]);
                while (b) { a%=b; swap(a,b); }
                if (arrow!=string::npos)
                    vars[trim(l.substr(arrow+2))]=Variable((double)a);
                else cout << "GCD = " << a << "\n";
            }
            return "";
        }

        // lcm a b -> var
        if (l.size()>3 && l.substr(0,3)=="lcm") {
            size_t arrow=l.find("->");
            string rest=(arrow!=string::npos)?l.substr(3,arrow-3):l.substr(3);
            auto parts=split(rest,' ');
            if (parts.size()>=2) {
                long long a=(long long)evalExpr(parts[0]);
                long long b=(long long)evalExpr(parts[1]);
                long long g=a, bb=b;
                while (bb) { g%=bb; swap(g,bb); }
                long long lcmv = (g!=0) ? a/g*b : 0;
                if (arrow!=string::npos)
                    vars[trim(l.substr(arrow+2))]=Variable((double)lcmv);
                else cout << "LCM = " << lcmv << "\n";
            }
            return "";
        }

        // prime n
        if (l.size()>5 && l.substr(0,5)=="prime") {
            long long n=(long long)evalExpr(trim(l.substr(5)));
            bool isp=(n>1);
            for (long long i=2;i*i<=n&&isp;i++) if(n%i==0) isp=false;
            vars["is_prime"]=Variable(isp);
            cout << n << (isp?" is prime":" is not prime") << "\n";
            return "";
        }

        // fib n
        if (l.size()>3 && l.substr(0,3)=="fib") {
            int n=(int)evalExpr(trim(l.substr(3)));
            long long a=0,b=1;
            for (int i=0;i<n-1;i++){long long c=a+b;a=b;b=c;}
            vars["fib_result"]=Variable((double)(n<=0?0:b));
            cout << "F(" << n << ") = " << (n<=0?0:b) << "\n";
            return "";
        }

        // ════════ СТРОКИ ═══════════════════════

        // strset name = "val"
        if (l.size()>6 && l.substr(0,6)=="strset") {
            size_t eq=l.find('=');
            if (eq!=string::npos) {
                string name=trim(l.substr(6,eq-6));
                string val =evalString(trim(l.substr(eq+1)));
                vars[name]=Variable(val);
            }
            return "";
        }

        // strcat dest src
        if (l.size()>6 && l.substr(0,6)=="strcat") {
            auto parts=split(l.substr(6),' ');
            if (parts.size()==2) {
                string add = vars.count(parts[1]) ? vars[parts[1]].str : parts[1];
                vars[parts[0]].str += add;
                vars[parts[0]].type = VarType::STRING_T;
            }
            return "";
        }

        // strprint name
        if (l.size()>8 && l.substr(0,8)=="strprint") {
            string name=trim(l.substr(8));
            if (vars.count(name)&&vars[name].type==VarType::STRING_T)
                cout << vars[name].str << "\n";
            return "";
        }

        // ════════ ФАЙЛЫ ════════════════════════

        // writefile "name" "content"
        if (l.size()>9 && l.substr(0,9)=="writefile") {
            size_t s1=l.find('"'), e1=l.find('"',s1+1);
            size_t s2=l.find('"',e1+1), e2=l.find('"',s2+1);
            if (s1!=string::npos && s2!=string::npos) {
                string fname  =l.substr(s1+1,e1-s1-1);
                string content=l.substr(s2+1,e2-s2-1);
                ofstream f(fname);
                if (f) { f<<content; cout<<"OK: wrote "<<fname<<"\n"; }
                else errMsg("Cannot open: "+fname);
            }
            return "";
        }

        // readfile "name" -> var
        if (l.size()>8 && l.substr(0,8)=="readfile") {
            size_t s1=l.find('"'), e1=l.find('"',s1+1);
            size_t arrow=l.find("->");
            if (s1!=string::npos && arrow!=string::npos) {
                string fname=l.substr(s1+1,e1-s1-1);
                string var  =trim(l.substr(arrow+2));
                ifstream f(fname);
                if (f) {
                    string content((istreambuf_iterator<char>(f)),
                                    istreambuf_iterator<char>());
                    vars[var]=Variable(content);
                    cout<<"OK: read "<<fname<<"\n";
                } else errMsg("Cannot open: "+fname);
            }
            return "";
        }

        // appendfile "name" "content"
        if (l.size()>10 && l.substr(0,10)=="appendfile") {
            size_t s1=l.find('"'), e1=l.find('"',s1+1);
            size_t s2=l.find('"',e1+1), e2=l.find('"',s2+1);
            if (s1!=string::npos && s2!=string::npos) {
                ofstream f(l.substr(s1+1,e1-s1-1), ios::app);
                if (f) f << l.substr(s2+1,e2-s2-1);
            }
            return "";
        }

        // ════════ ВРЕМЯ ════════════════════════

        // time
        if (l=="time") {
            time_t t=time(nullptr);
            cout << "Time: " << ctime(&t);
            return "";
        }

        // sleep N (мс)
        if (l.size()>5 && l.substr(0,5)=="sleep") {
            int ms=(int)evalExpr(trim(l.substr(5)));
            usleep(ms * 1000); // microseconds
            return "";
        }

        // timeit N
        if (l.size()>6 && l.substr(0,6)=="timeit") {
            long long n=(long long)evalExpr(trim(l.substr(6)));
            auto t0=high_resolution_clock::now();
            volatile long long d=0;
            for (long long i=0;i<n;i++) d++;
            auto ms=duration_cast<milliseconds>(
                high_resolution_clock::now()-t0).count();
            cout << n << " iters: " << ms << " ms  ("
                 << (ms>0?n*1000/ms:0) << " ops/sec)\n";
            vars["timeit_ms"]=Variable((double)ms);
            return "";
        }

        // timestamp -> var
        if (l.size()>9 && l.substr(0,9)=="timestamp") {
            size_t arrow=l.find("->");
            long long ts=duration_cast<seconds>(
                system_clock::now().time_since_epoch()).count();
            if (arrow!=string::npos)
                vars[trim(l.substr(arrow+2))]=Variable((double)ts);
            else cout<<"Timestamp: "<<ts<<"\n";
            return "";
        }

        // ════════ РИСОВАНИЕ ════════════════════

        // box W H [char]
        if (l.size()>3 && l.substr(0,3)=="box") {
            auto parts=split(l.substr(3),' ');
            if (parts.size()>=2) {
                int w=(int)evalExpr(parts[0]);
                int h=(int)evalExpr(parts[1]);
                char c=(parts.size()>=3) ? parts[2][0] : '*';
                for (int r=0;r<h;r++) {
                    for (int cc=0;cc<w;cc++) cout<<c;
                    cout<<"\n";
                }
            }
            return "";
        }

        // line W [char]
        if (l.size()>4 && l.substr(0,4)=="line") {
            auto parts=split(l.substr(4),' ');
            if (!parts.empty()) {
                int w=(int)evalExpr(parts[0]);
                char c=(parts.size()>=2)?parts[1][0]:'-';
                for (int i=0;i<w;i++) cout<<c;
                cout<<"\n";
            }
            return "";
        }

        // triangle N
        if (l.size()>8 && l.substr(0,8)=="triangle") {
            int n=(int)evalExpr(trim(l.substr(8)));
            for (int i=1;i<=n;i++) {
                for (int j=0;j<i;j++) cout<<"* ";
                cout<<"\n";
            }
            return "";
        }

        // diamond N
        if (l.size()>7 && l.substr(0,7)=="diamond") {
            int n=(int)evalExpr(trim(l.substr(7)));
            for (int i=1;i<=n;i++) {
                for (int j=n-i;j>0;j--) cout<<" ";
                for (int j=0;j<2*i-1;j++) cout<<"*";
                cout<<"\n";
            }
            for (int i=n-1;i>=1;i--) {
                for (int j=n-i;j>0;j--) cout<<" ";
                for (int j=0;j<2*i-1;j++) cout<<"*";
                cout<<"\n";
            }
            return "";
        }

        // ════════ СИСТЕМНОЕ ════════════════════

        // assert cond "msg"
        if (l.size()>6 && l.substr(0,6)=="assert") {
            size_t q=l.find('"');
            string condStr=(q!=string::npos)?trim(l.substr(6,q-6)):trim(l.substr(6));
            string msg=(q!=string::npos)?l.substr(q+1,l.rfind('"')-q-1):"Assert failed";
            if (!evalCond(condStr)) errMsg("ASSERT: "+msg);
            else cout<<Color::GREEN<<"assert ok"<<Color::RESET<<"\n";
            return "";
        }

        // throw "msg"
        if (l.size()>5 && l.substr(0,5)=="throw") {
            string msg=trim(l.substr(5));
            if (!msg.empty()&&msg.front()=='"') msg=msg.substr(1,msg.size()-2);
            errMsg("throw: "+msg);
            return "exit";
        }

        // vars
        if (l=="vars") {
            cout<<Color::CYAN<<"\n-- Variables --"<<Color::RESET<<"\n";
            for (auto& kv:vars) {
                if (kv.second.type==VarType::NUMBER)
                    cout<<"  "<<kv.first<<" = "<<formatNum(kv.second.number)<<"\n";
                else if (kv.second.type==VarType::STRING_T)
                    cout<<"  "<<kv.first<<" = \""<<kv.second.str<<"\"\n";
                else
                    cout<<"  "<<kv.first<<" = "<<(kv.second.boolean?"true":"false")<<"\n";
            }
            for (auto& kv:arrays) {
                cout<<"  "<<kv.first<<"["<<kv.second.size()<<"] = [";
                for (size_t i=0;i<min(kv.second.size(),(size_t)5);i++) {
                    if (i) cout<<", ";
                    cout<<formatNum(kv.second[i]);
                }
                if (kv.second.size()>5) cout<<"...";
                cout<<"]\n";
            }
            return "";
        }

        // stats
        if (l=="stats") {
            cout<<Color::CYAN<<"\n-- Stats --"<<Color::RESET<<"\n";
            cout<<"  Lines:     "<<line_count<<"\n";
            cout<<"  Errors:    "<<error_count<<"\n";
            cout<<"  Variables: "<<vars.size()<<"\n";
            cout<<"  Arrays:    "<<arrays.size()<<"\n";
            cout<<"  Functions: "<<functions.size()<<"\n";
            cout<<"  Time ms:   "<<formatNum(exec_time,2)<<"\n";
            return "";
        }

        if (l=="debug on")  { debug_mode=true;  cout<<"Debug ON\n";  return ""; }
        if (l=="debug off") { debug_mode=false; cout<<"Debug OFF\n"; return ""; }
        if (l=="strict on") { strict_mode=true;  return ""; }
        if (l=="strict off"){ strict_mode=false; return ""; }

        if (l=="clear") {
            vars.clear(); arrays.clear(); initConstants();
            cout<<"Variables cleared\n";
            return "";
        }
        if (l=="clearscreen") { cout<<"\033[2J\033[H"; return ""; }
        if (l=="help")  { printHelp(); return ""; }
        if (l=="exit"||l=="quit") { running=false; return "exit"; }

        // ════════ COUNT / BENCH / SPEED ════════

        if (l.size()>=5 && l.substr(0,5)=="count") { executeCount(l); return ""; }
        if (l.size()>=5 && l.substr(0,5)=="bench") { executeBench(l); return ""; }
        if (l=="speed") { executeSpeed(); return ""; }

        // ════════ BREAK / CONTINUE ═════════════
        if (l=="break")    return "break";
        if (l=="continue") return "continue";

        // Неизвестная команда
        if (strict_mode) errMsg("Unknown: " + l);
        else dbg("skip: " + l);
        return "";
    }

    // ── запуск блока ─────────────────────────

    string runBlock(vector<string>& lines, int& i, int depth=0) {
        while (i < (int)lines.size()) {
            string l = trim(lines[i]);

            // Конец блока
            if (l=="end"||l=="else"||l=="endwhile"||
                l=="endfor"||l=="endfunc") {
                return "block_end:" + l;
            }
            if (l.size()>=4 && l.substr(0,4)=="elif") {
                return "block_end:elif";
            }

            // ── IF ──────────────────────────────
            if (l.size()>=2 && l.substr(0,2)=="if") {
                string cond = trim(l.substr(2));
                bool result = evalCond(cond);

                vector<string> thenB, elseB;
                i++;
                // Собираем then-ветку
                int depth2=0;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if ((cl=="end"||cl=="else"||
                         (cl.size()>=4&&cl.substr(0,4)=="elif"))&&depth2==0) break;
                    if (cl.size()>=2&&cl.substr(0,2)=="if") depth2++;
                    if (cl=="end"&&depth2>0) depth2--;
                    thenB.push_back(lines[i]);
                    i++;
                }
                // else / elif
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end") { i++; break; }
                    if (cl=="else") {
                        i++;
                        while (i<(int)lines.size()) {
                            string ecl=trim(lines[i]);
                            if (ecl=="end") { i++; break; }
                            elseB.push_back(lines[i]);
                            i++;
                        }
                        break;
                    }
                    if (cl.size()>=4&&cl.substr(0,4)=="elif") {
                        // превращаем в if
                        string newIf = "if " + trim(cl.substr(4));
                        elseB.push_back(newIf);
                        i++;
                        int d3=0;
                        while (i<(int)lines.size()) {
                            string ecl=trim(lines[i]);
                            if (ecl.size()>=2&&ecl.substr(0,2)=="if") d3++;
                            if (ecl=="end"&&d3==0) { elseB.push_back("end"); i++; break; }
                            if (ecl=="end"&&d3>0) d3--;
                            elseB.push_back(lines[i]);
                            i++;
                        }
                        break;
                    }
                    i++;
                }

                auto& blk = result ? thenB : elseB;
                int bi=0;
                string r = runBlock(blk, bi, depth+1);
                if (r=="break"||r=="continue"||r.substr(0,6)=="return") return r;
                continue;
            }

            // ── WHILE ───────────────────────────
            if (l.size()>=5 && l.substr(0,5)=="while") {
                string cond=trim(l.substr(5));
                vector<string> body;
                i++;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end"||cl=="endwhile") { i++; break; }
                    body.push_back(lines[i]);
                    i++;
                }
                while (evalCond(cond)) {
                    int bi=0;
                    string r=runBlock(body,bi,depth+1);
                    if (r=="break") break;
                    if (r.size()>=6&&r.substr(0,6)=="return") return r;
                }
                continue;
            }

            // ── FOR i = a to b [step s] ──────────
            if (l.size()>=3 && l.substr(0,3)=="for") {
                string rest=trim(l.substr(3));
                size_t eq_pos=rest.find('=');
                size_t to_pos=rest.find(" to ");
                if (eq_pos!=string::npos && to_pos!=string::npos) {
                    string varName=trim(rest.substr(0,eq_pos));
                    double fromVal=evalExpr(rest.substr(eq_pos+1,to_pos-eq_pos-1));
                    string after  =rest.substr(to_pos+4);
                    double stepVal=1, toVal;
                    size_t sp=after.find(" step ");
                    if (sp!=string::npos) {
                        toVal  =evalExpr(after.substr(0,sp));
                        stepVal=evalExpr(after.substr(sp+6));
                    } else toVal=evalExpr(after);

                    vector<string> body;
                    i++;
                    while (i<(int)lines.size()) {
                        string cl=trim(lines[i]);
                        if (cl=="end"||cl=="endfor") { i++; break; }
                        body.push_back(lines[i]);
                        i++;
                    }
                    vars[varName]=Variable(fromVal);
                    while ((stepVal>0 ? vars[varName].number<=toVal
                                      : vars[varName].number>=toVal)) {
                        int bi=0;
                        string r=runBlock(body,bi,depth+1);
                        if (r=="break") break;
                        if (r.size()>=6&&r.substr(0,6)=="return") return r;
                        vars[varName].number+=stepVal;
                    }
                }
                continue;
            }

            // ── FOREACH item in arr ──────────────
            if (l.size()>=7 && l.substr(0,7)=="foreach") {
                string rest=trim(l.substr(7));
                size_t in_pos=rest.find(" in ");
                if (in_pos!=string::npos) {
                    string itemName=trim(rest.substr(0,in_pos));
                    string arrName =trim(rest.substr(in_pos+4));
                    vector<string> body;
                    i++;
                    while (i<(int)lines.size()) {
                        string cl=trim(lines[i]);
                        if (cl=="end") { i++; break; }
                        body.push_back(lines[i]);
                        i++;
                    }
                    if (arrays.count(arrName)) {
                        for (double val:arrays[arrName]) {
                            vars[itemName]=Variable(val);
                            int bi=0;
                            string r=runBlock(body,bi,depth+1);
                            if (r=="break") break;
                            if (r.size()>=6&&r.substr(0,6)=="return") return r;
                        }
                    }
                }
                continue;
            }

            // ── REPEAT(N) ────────────────────────
            if (l.size()>=6 && l.substr(0,6)=="repeat") {
                string inside=getArgs(l);
                int n=(int)evalExpr(inside);
                vector<string> body;
                i++;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end") { i++; break; }
                    body.push_back(lines[i]);
                    i++;
                }
                for (int r=0;r<n;r++) {
                    vars["__i"]=Variable((double)r);
                    loop_counter=r;
                    int bi=0;
                    string res=runBlock(body,bi,depth+1);
                    if (res=="break") break;
                    if (res.size()>=6&&res.substr(0,6)=="return") return res;
                }
                continue;
            }

            // ── LOOP (бесконечный) ───────────────
            if (l=="loop") {
                vector<string> body;
                i++;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end") { i++; break; }
                    body.push_back(lines[i]);
                    i++;
                }
                while (true) {
                    int bi=0;
                    string r=runBlock(body,bi,depth+1);
                    if (r=="break") break;
                    if (r.size()>=6&&r.substr(0,6)=="return") return r;
                }
                continue;
            }

            // ── FUNC ────────────────────────────
            if (l.size()>=4 && l.substr(0,4)=="func") {
                string rest=trim(l.substr(4));
                size_t lp=rest.find('('), rp=rest.rfind(')');
                string fname=trim(rest.substr(0,lp));
                string pstr=(lp!=string::npos&&rp!=string::npos)?
                             rest.substr(lp+1,rp-lp-1):"";
                UserFunction fn;
                fn.params = pstr.empty() ? vector<string>{} : split(pstr,',');
                i++;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end"||cl=="endfunc") { i++; break; }
                    fn.body.push_back(lines[i]);
                    i++;
                }
                functions[fname]=fn;
                cout<<Color::DIM<<"func "<<fname<<" defined"<<Color::RESET<<"\n";
                continue;
            }

            // ── CALL name(args) [-> var] ─────────
            if (l.size()>=4 && l.substr(0,4)=="call") {
                string rest=trim(l.substr(4));
                size_t arrow=rest.find("->");
                string callPart=(arrow!=string::npos)?trim(rest.substr(0,arrow)):rest;
                string retVar  =(arrow!=string::npos)?trim(rest.substr(arrow+2)):"";
                size_t lp=callPart.find('('), rp=callPart.rfind(')');
                string fname=trim(callPart.substr(0,lp));
                string astr=(lp!=string::npos&&rp!=string::npos)?
                             callPart.substr(lp+1,rp-lp-1):"";
                if (functions.count(fname)) {
                    auto& fn=functions[fname];
                    auto args=astr.empty()?vector<string>{}:split(astr,',');
                    map<string,Variable> saved=vars;
                    for (size_t pi=0;pi<fn.params.size();pi++) {
                        if (pi<args.size()) {
                            if (!args[pi].empty()&&args[pi].front()=='"')
                                vars[fn.params[pi]]=Variable(evalString(args[pi]));
                            else
                                vars[fn.params[pi]]=Variable(evalExpr(args[pi]));
                        }
                    }
                    call_stack.push_back(fname);
                    int bi=0;
                    string r=runBlock(fn.body,bi,depth+1);
                    call_stack.pop_back();
                    double retVal=0;
                    if (r.size()>=7&&r.substr(0,7)=="return:")
                        try { retVal=stod(r.substr(7)); } catch(...) {}
                    vars=saved;
                    if (!retVar.empty()) vars[retVar]=Variable(retVal);
                } else errMsg("Function not found: "+fname);
                i++;
                continue;
            }

            // ── RETURN ──────────────────────────
            if (l.size()>=6 && l.substr(0,6)=="return") {
                string val=trim(l.substr(6));
                return "return:" + formatNum(evalExpr(val));
            }

            // ── SWITCH ──────────────────────────
            if (l.size()>=6 && l.substr(0,6)=="switch") {
                double val=evalExpr(trim(l.substr(6)));
                i++;
                bool matched=false;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end") { i++; break; }
                    if (cl.size()>=4&&cl.substr(0,4)=="case") {
                        size_t colon=cl.find(':');
                        if (colon!=string::npos) {
                            double cv=evalExpr(cl.substr(4,colon-4));
                            if (!matched&&cv==val) {
                                matched=true;
                                string body=trim(cl.substr(colon+1));
                                if (!body.empty()) executeLine(body,depth);
                            }
                        }
                    } else if (cl.size()>=7&&cl.substr(0,7)=="default") {
                        if (!matched) {
                            size_t colon=cl.find(':');
                            if (colon!=string::npos) {
                                string body=trim(cl.substr(colon+1));
                                if (!body.empty()) executeLine(body,depth);
                            }
                        }
                    }
                    i++;
                }
                continue;
            }

            // ── TRY / CATCH ─────────────────────
            if (l=="try") {
                vector<string> tryB, catchB;
                i++;
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="catch"||cl=="end") { i++; break; }
                    tryB.push_back(lines[i]);
                    i++;
                }
                while (i<(int)lines.size()) {
                    string cl=trim(lines[i]);
                    if (cl=="end") { i++; break; }
                    catchB.push_back(lines[i]);
                    i++;
                }
                int savedErr=error_count;
                int bi=0;
                runBlock(tryB,bi,depth+1);
                if (error_count>savedErr) {
                    bi=0;
                    runBlock(catchB,bi,depth+1);
                }
                continue;
            }

            // Обычная строка
            string r=executeLine(lines[i],depth);
            if (r=="break"||r=="continue"||r=="exit"||
                (r.size()>=6&&r.substr(0,6)=="return")) return r;
            i++;
        }
        return "";
    }

    // ── count ─────────────────────────────────

    void executeCount(const string& line) {
        string l = trim(line.substr(5));
        long long sv=0, ev=0, step=1;

        if (l.size()>=2 && l.substr(0,2)=="to")
            ev=(long long)evalExpr(trim(l.substr(2)));
        else if (l.size()>=4 && l.substr(0,4)=="from") {
            size_t tp=l.find(" to ");
            if (tp!=string::npos) {
                sv=(long long)evalExpr(l.substr(4,tp-4));
                ev=(long long)evalExpr(l.substr(tp+4));
            }
        } else if (l.find("step")!=string::npos) {
            size_t sp=l.find("step"), tp=l.find(" to ");
            if (sp!=string::npos&&tp!=string::npos) {
                step=(long long)evalExpr(l.substr(sp+4,tp-sp-4));
                ev  =(long long)evalExpr(l.substr(tp+4));
            }
        } else ev=(long long)evalExpr(l);
        if (ev==0) ev=(long long)evalExpr(l);

        cout<<Color::CYAN<<"\nCounting to "<<ev<<Color::RESET<<"\n";
        auto t0=high_resolution_clock::now();
        volatile long long cnt=sv;
        long long last_pct=-1;
        for (long long i=sv;i<ev;i+=step) {
            cnt=i;
            if (ev>1000000) {
                long long pct=(i-sv)*100/(ev-sv);
                if (pct>=last_pct+10) {
                    last_pct=pct;
                    auto ms=duration_cast<milliseconds>(
                        high_resolution_clock::now()-t0).count();
                    cout<<"  "<<pct<<"% - "<<i<<" ("<<ms/1000.0<<"s)\n";
                }
            }
        }
        auto ms=duration_cast<milliseconds>(high_resolution_clock::now()-t0).count();
        double speed=(double)(ev-sv)/(ms>0?ms/1000.0:0.001);
        cout<<Color::GREEN<<"Done! "<<Color::RESET
            <<"Time: "<<ms/1000.0<<"s  Speed: "
            <<(long long)speed<<" ops/sec\n";
        vars["last_count"] =Variable((double)cnt);
        vars["count_time"] =Variable(ms/1000.0);
        vars["count_speed"]=Variable(speed);
    }

    // ── bench ─────────────────────────────────

    void executeBench(const string& line) {
        string l=trim(line.substr(5));
        long long n=l.empty()?1000000LL:(long long)evalExpr(l);
        cout<<Color::CYAN<<"\nBenchmark: "<<n<<" ops"<<Color::RESET<<"\n";
        cout<<"+---------------------+----------+-----------------+\n";
        cout<<"| Test                | ms       | ops/sec         |\n";
        cout<<"+---------------------+----------+-----------------+\n";

        auto run=[&](const string& name, function<void(long long)> fn){
            auto t0=high_resolution_clock::now();
            fn(n);
            auto ms=duration_cast<milliseconds>(high_resolution_clock::now()-t0).count();
            long long ops=(ms>0)?n*1000/ms:0;
            cout<<"| "<<setw(19)<<left<<name
                <<" | "<<setw(8)<<right<<ms
                <<" | "<<setw(15)<<ops<<" |\n";
        };

        run("Empty loop",   [](long long n){ volatile long long d=0; for(long long i=0;i<n;i++) d++; });
        run("Addition",     [](long long n){ long long s=0; for(long long i=0;i<n;i++) s+=i; });
        run("Multiply",     [](long long n){ long long m=1; for(long long i=1;i<n;i++) m*=(i%100+1); });
        run("Division",     [](long long n){ double d=1; for(long long i=1;i<n;i++) d/=(i%10+1.0); });
        run("sqrt",         [](long long n){ double d=0; for(long long i=0;i<n;i++) d+=sqrt((double)i); });

        cout<<"+---------------------+----------+-----------------+\n";
    }

    // ── speed ─────────────────────────────────

    void executeSpeed() {
        cout<<Color::CYAN<<"\nSpeed test"<<Color::RESET<<"\n";
        long long sizes[]={1000000LL,10000000LL,100000000LL,1000000000LL};
        const char* names[]={"1M","10M","100M","1B"};
        for (int k=0;k<4;k++) {
            long long n=sizes[k];
            auto t0=high_resolution_clock::now();
            volatile long long d=0;
            for (long long i=0;i<n;i++) d++;
            auto ms=duration_cast<milliseconds>(high_resolution_clock::now()-t0).count();
            cout<<"  "<<setw(4)<<names[k]<<" iters: "
                <<setw(6)<<ms<<" ms  ("
                <<n*1000/max(ms,1LL)<<" ops/sec)\n";
        }
    }

    // ── help ──────────────────────────────────

    void printHelp() {
        cout<<Color::BOLD<<Color::YELLOW<<"\n=== Z# v5.0 HELP ===\n"<<Color::RESET;
        auto S=[](const string& s){
            cout<<Color::CYAN<<"\n["<<s<<"]\n"<<Color::RESET;
        };
        auto C=[](const string& c, const string& d){
            cout<<"  "<<setw(28)<<left<<c<<"  "<<d<<"\n";
        };

        S("OUTPUT");
        C("say \"text {var}\"",      "print with interpolation");
        C("print expr",              "print expression");
        C("printfmt x 2",            "print with decimals");
        C("printtable arr",          "table of array");
        C("banner \"text\"",         "big header");
        C("header \"text\"",         "section header");
        C("progress cur max",        "progress bar");
        C("separator",               "---- line");
        C("newline / tab",           "newline / tab");
        C("putchar 65",              "print ASCII char");

        S("INPUT");
        C("input x",                 "read value");
        C("ask \"Q?\" -> x",         "read with prompt");

        S("VARIABLES");
        C("let/set/var x = expr",    "assign variable");
        C("inc x / inc x by N",      "increment");
        C("dec x / dec x by N",      "decrement");
        C("mul x by N",              "multiply");
        C("div x by N",              "divide");
        C("swap x y",                "swap two vars");
        C("delete x",                "delete var");
        C("typeof x",                "get type");
        C("cast x to num/str/bool",  "type cast");

        S("ARRAYS");
        C("array a = [1,2,3]",       "literal array");
        C("array a = range(1,10)",   "range");
        C("array a = zeros(5)",      "zeros");
        C("array a = ones(5)",       "ones");
        C("array a = randArr(5,0,9)","random");
        C("push arr val",            "append");
        C("pop arr",                 "remove last -> popped");
        C("arr[i] = val",            "set element");
        C("sort/sortdesc arr",       "sort");
        C("shuffle/reverse arr",     "shuffle/reverse");
        C("fill arr val",            "fill all");
        C("findmax/findmin arr->v",  "max/min");

        S("CONDITIONS");
        C("if cond",                 "if");
        C("elif cond",               "else if");
        C("else / end",              "else / end");

        S("LOOPS");
        C("for i = 0 to 10",         "for loop (+step N)");
        C("while cond",              "while loop");
        C("repeat(N)",               "repeat N times");
        C("foreach item in arr",     "iterate array");
        C("loop",                    "infinite loop");
        C("break / continue",        "flow control");

        S("FUNCTIONS");
        C("func name(a,b)",          "define function");
        C("  return expr",           "return value");
        C("end",                     "end function");
        C("call name(1,2) -> res",   "call function");

        S("MATH");
        C("calc expr -> var",        "evaluate expr");
        C("solve a b",               "solve ax+b=0");
        C("gcd/lcm a b -> var",      "GCD / LCM");
        C("prime n",                 "primality test");
        C("fib n",                   "Fibonacci");
        C("sqrt/abs/sin/cos/tan(x)", "math functions");
        C("pow(x,y)/max/min(x,y)",   "power/max/min");
        C("rand(a,b)/randf(a,b)",    "random int/float");
        C("log/log2/log10/exp(x)",   "log/exp");
        C("floor/ceil/round/fact(x)","rounding/factorial");
        C("sum(arr)/avg(arr)/len(x)","array math");

        S("STRINGS");
        C("strset name = \"val\"",   "set string");
        C("strcat dest src",         "concatenate");
        C("upper/lower(str)",        "case");
        C("reverse(str)",            "reverse");
        C("trim(str)",               "trim");

        S("FILES");
        C("writefile \"f\" \"txt\"", "write file");
        C("readfile \"f\" -> var",   "read file");
        C("appendfile \"f\" \"t\"",  "append file");

        S("TIME");
        C("time",                    "current time");
        C("sleep N",                 "sleep N ms");
        C("timeit N",                "time N iterations");
        C("timestamp -> var",        "unix timestamp");

        S("DRAW");
        C("box W H [char]",          "rectangle");
        C("line W [char]",           "horizontal line");
        C("triangle N",              "triangle");
        C("diamond N",               "diamond");

        S("FLOW CONTROL");
        C("switch x / case V:",      "switch/case");
        C("default: / end",          "default/end");
        C("try / catch / end",       "try-catch");
        C("assert cond \"msg\"",     "assertion");
        C("throw \"msg\"",           "throw error");

        S("SYSTEM");
        C("debug on/off",            "debug mode");
        C("strict on/off",           "strict mode");
        C("vars / stats",            "show vars/stats");
        C("clear / clearscreen",     "clear");
        C("help / exit",             "help / quit");

        S("BENCHMARK");
        C("count N",                 "count to N");
        C("count from A to B",       "count range");
        C("bench N",                 "benchmark N ops");
        C("speed",                   "speed test");

        S("COLORS in say");
        C("{red} {green} {yellow}",  "text colors");
        C("{blue} {cyan} {magenta}", "more colors");
        C("{bold} {dim} {reset}",    "styles");

        S("CONSTANTS");
        C("pi / e / phi",            "math constants");
        C("million / billion",       "big numbers");
        C("true / false / inf",      "special values");

        cout<<Color::BOLD<<"\n====================\n"<<Color::RESET;
    }

public:
    ZSharp() : rng(random_device{}()) {
        loop_counter=0; debug_mode=false; strict_mode=false;
        running=true; exec_time=0; line_count=0; error_count=0;
        initConstants();
        printLogo();
    }

    void printLogo() {
        cout<<Color::CYAN<<Color::BOLD;
        cout<<"\n  ZZZZZZ  #  #\n";
        cout<<"     Z   ## ##\n";
        cout<<"    Z    # # #\n";
        cout<<"   Z     #   #\n";
        cout<<"  ZZZZZZ #   #\n";
        cout<<Color::RESET;
        cout<<Color::BOLD<<"  Z# (Zi Sharp) v5.0\n"<<Color::RESET;
        cout<<"  ================================\n";
        cout<<"  if/elif/else  for  while  foreach\n";
        cout<<"  func/call  array  switch  try/catch\n";
        cout<<"  math  strings  files  bench  count\n";
        cout<<"  ================================\n";
        cout<<"  Type 'help' for commands\n\n";
    }

    void runProgram(const string& text) {
        vector<string> lines;
        stringstream ss(text);
        string line;
        while (getline(ss,line)) {
            if (!trim(line).empty()) lines.push_back(line);
        }
        auto t0=high_resolution_clock::now();
        line_count=0; error_count=0;
        int i=0;
        runBlock(lines,i);
        exec_time=duration_cast<microseconds>(
            high_resolution_clock::now()-t0).count()/1000.0;
        cout<<Color::GREEN<<"\nDone in "<<formatNum(exec_time,2)
            <<" ms"<<Color::RESET<<"\n";
    }

    void interactive() {
        string input, prog;
        cout<<Color::CYAN<<"Interactive mode"<<Color::RESET<<"\n";
        cout<<"Empty line = run | 'exit' = quit | 'show' = show buffer | 'clear' = clear\n";
        cout<<string(40,'-')<<"\n";

        while (running) {
            cout<<Color::GREEN<<">>> "<<Color::RESET;
            if (!getline(cin,input)) break;

            if (input=="exit"||input=="quit") break;
            if (input.empty()) {
                if (!prog.empty()) {
                    runProgram(prog);
                    prog.clear();
                    cout<<string(40,'-')<<"\n";
                }
            } else if (input=="clear") {
                prog.clear(); cout<<"Buffer cleared\n";
            } else if (input=="show") {
                cout<<Color::DIM<<prog<<Color::RESET;
            } else {
                prog += input + "\n";
            }
        }
        cout<<Color::CYAN<<"Goodbye!"<<Color::RESET<<"\n";
    }
};

int main() {
    ZSharp z;

    // Демонстрационная программа
    string demo =
        "banner \"Z# v5.0 Demo\"\n"
        "\n"
        "header \"Variables & Math\"\n"
        "let a = 10\n"
        "let b = 3\n"
        "say \"a = {a}, b = {b}\"\n"
        "say \"a + b = $(a + b)\"\n"
        "say \"pow(a,b) = $(pow(a,b))\"\n"
        "say \"sqrt(a) = $(sqrt(a))\"\n"
        "\n"
        "separator\n"
        "header \"Conditions\"\n"
        "if a > b\n"
        "  say \"{green}a is greater than b{reset}\"\n"
        "elif a == b\n"
        "  say \"equal\"\n"
        "else\n"
        "  say \"b is greater\"\n"
        "end\n"
        "\n"
        "separator\n"
        "header \"Arrays\"\n"
        "array nums = [5, 3, 8, 1, 9, 2, 7]\n"
        "say \"Array:\"\n"
        "print nums\n"
        "sort nums\n"
        "say \"Sorted:\"\n"
        "print nums\n"
        "findmax nums -> mx\n"
        "say \"Max = {mx}\"\n"
        "say \"Sum = $(sum(nums))\"\n"
        "say \"Avg = $(avg(nums))\"\n"
        "\n"
        "separator\n"
        "header \"For loop\"\n"
        "let total = 0\n"
        "for i = 1 to 5\n"
        "  inc total by i\n"
        "  say \"  i={i} total={total}\"\n"
        "end\n"
        "\n"
        "separator\n"
        "header \"Functions\"\n"
        "func factorial(n)\n"
        "  if n <= 1\n"
        "    return 1\n"
        "  end\n"
        "  let tmp = n - 1\n"
        "  call factorial(tmp) -> sub\n"
        "  let res = n * sub\n"
        "  return res\n"
        "end\n"
        "call factorial(6) -> f\n"
        "say \"6! = {f}\"\n"
        "\n"
        "separator\n"
        "header \"Number theory\"\n"
        "prime 17\n"
        "prime 18\n"
        "fib 10\n"
        "\n"
        "separator\n"
        "header \"Drawing\"\n"
        "triangle 4\n"
        "separator\n"
        "diamond 3\n"
        "\n"
        "separator\n"
        "header \"Progress bar\"\n"
        "progress 75 100\n"
        "\n"
        "separator\n"
        "header \"Switch\"\n"
        "let x = 2\n"
        "switch x\n"
        "  case 1: say \"one\"\n"
        "  case 2: say \"two\"\n"
        "  case 3: say \"three\"\n"
        "  default: say \"other\"\n"
        "end\n"
        "\n"
        "separator\n"
        "stats\n"
        "separator\n"
        "say \"{green}Demo complete!{reset}\"\n";

    z.runProgram(demo);
    cout << "\n";
    z.interactive();
    return 0;
}