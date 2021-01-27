/// A microlisp named Wisp, by Adam McDaniel
/// modified version by lyzh, 2021-01-07

#ifndef __WISP_HPP
#define __WISP_HPP

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <memory>
#include <functional>

namespace lisp{
std::string read_file_contents( std::string filename);

// Forward declaration for Environment class definition
class Value;

// An instance of a function's scope.
class Environment {
public:
    // Default constructor
    Environment() : parent_scope(NULL) {}

    // Does this environment, or its parent environment,
    // have this atom in scope?
    // This is only used to determine which atoms to capture when
    // creating a lambda function.
    bool has(const std::string &name) const;
    // Get the value associated with this name in this scope
    Value get(const std::string &name) const;
    // Set the value associated with this name in this scope
    void set(const std::string &name, const Value &value);

    void combine(const Environment &other);

    void set_parent_scope(Environment *parent) {
        parent_scope = parent;
    }
    
    // Output this scope in readable form to a stream.
    friend std::ostream &operator<<(std::ostream &os, const Environment &v);
private:

    // The definitions in the scope.
    std::map<std::string, Value> defs;
    Environment *parent_scope;
};


// An exception thrown by the lisp
class Error {
public:
    // Create an error with the value that caused the error,
    // the scope where the error was found, and the message.
    Error(Value v, Environment const &env, const char *msg);
    // Copy constructor is needed to prevent double frees
    Error(const Error &other);
    ~Error();

    // Get the printable error description.
    std::string description();
private:
    Value *cause;
    Environment env;
    const char *msg;
};

// The type for a builtin function, which takes a list of values,
// and the environment to run the function in.
typedef Value (*Builtin)(std::vector<Value>, Environment &);

class user_data:public std::enable_shared_from_this<user_data>
{
public:
	virtual ~user_data(){}
	virtual std::string display() const{ return "<user_data.display not supported>";}
	Value apply(const std::vector<Value> &args, Environment &env);
};

class Value {
public:
    ////////////////////////////////////////////////////////////////////////////////
    /// CONSTRUCTORS ///////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    // Constructs a unit value
    Value() : type(UNIT) {}

    // Constructs an integer
    Value(int i) : type(INT) { stack_data.i = i; }
    // Constructs a floating point value
    Value(double f) : type(FLOAT) { stack_data.f = f; }
    // Constructs a list
    Value(const std::vector<Value> &list) : type(LIST), list(list) {}
    Value(const Value &o) = default;

    // Construct a quoted value
    static Value quote(const Value &other);

    // Construct an atom
    static Value atom(const std::string &s);

    // Construct a string
    static Value string(const std::string &s);

    // Construct a lambda function
    Value(const std::vector<Value> &params, const Value &ret, Environment const &env);

    // Construct a builtin function
    //Value(std::string name, Builtin b);
    Value(std::string name, std::function<Value(std::vector<Value>, Environment &)>bf);
    Value(std::string name, std::shared_ptr<user_data> ud);

    ////////////////////////////////////////////////////////////////////////////////
    /// C++ INTEROP METHODS ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    // Get all of the atoms used in a given Value
    std::vector<std::string> get_used_atoms()const;

    // Is this a builtin function?
    bool is_builtin() {return type == BUILTIN;}

    // Apply this as a function to a list of arguments in a given environment.
    Value apply(const std::vector<Value> &args, Environment &env);
    // Evaluate this value as lisp code.
    Value eval(Environment &env) const;

    bool is_number() const ;

    // Get the "truthy" boolean value of this value.
    bool as_bool() const ;

    // Get this item's integer value
    int as_int() const;
    // Get this item's floating point value
    double as_float() const ;

    // Get this item's string value
    std::string as_string() const ;

    // Get this item's atom value
    std::string as_atom() const;

    // Get this item's list value
    std::vector<Value> as_list() const;

    std::shared_ptr<user_data> as_user_data()const;

    // Push an item to the end of this list
    void push(const Value &val) ;

    // Push an item from the end of this list
    Value pop() ;

    ////////////////////////////////////////////////////////////////////////////////
    /// TYPECASTING METHODS ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    // Cast this to an integer value
    Value cast_to_int() const;

    // Cast this to a floating point value
    Value cast_to_float() const;

    ////////////////////////////////////////////////////////////////////////////////
    /// COMPARISON OPERATIONS //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    bool operator==(const Value &other) const ;
    
    bool operator!=(const Value &other) const;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// ORDERING OPERATIONS ////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    bool operator>=(const Value &other) const;
    
    bool operator<=(const Value &other) const;
    
    bool operator>(const Value &other) const;

    bool operator<(const Value &other) const;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// ARITHMETIC OPERATIONS //////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////

    // This function adds two lisp values, and returns the lisp value result.
    Value operator+(const Value &other) const;

    // This function subtracts two lisp values, and returns the lisp value result.
    Value operator-(const Value &other) const;

    // This function multiplies two lisp values, and returns the lisp value result.
    Value operator*(const Value &other) const;

    // This function divides two lisp values, and returns the lisp value result.
    Value operator/(const Value &other) const;

    // This function finds the remainder of two lisp values, and returns the lisp value result.
    Value operator%(const Value &other) const;

    // Get the name of the type of this value
    std::string get_type_name()const;

    std::string display() const;
    std::string debug() const;

    friend std::ostream &operator<<(std::ostream &os, Value const &v);

private:
    enum {
        QUOTE,
        ATOM,
        INT,
        FLOAT,
        LIST,
        STRING,
        LAMBDA,
        BUILTIN,
        UNIT,
		USERDATA
    } type;

    union {
        int i;
        double f;
    } stack_data;

    std::string str;
    std::vector<Value> list;
    Environment lambda_scope;
    std::shared_ptr<user_data> userdata;
    std::function<Value(std::vector<Value>, Environment &)> b;
};
void eval_args(std::vector<Value> &args, Environment &env);

void global_set(std::string name, Value v);
static inline void global_set(std::string name, Builtin f){
	global_set(name, Value(name,f));
}
std::ostream &operator<<(std::ostream &os, Environment const &e);

// Execute code in an environment
Value run(const std::string &code, Environment &env);
void repl(Environment &env);

}

#endif