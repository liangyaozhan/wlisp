/// A microlisp named Wisp, by Adam McDaniel

////////////////////////////////////////////////////////////////////////////////
/// LANGUAGE OPTIONS ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Comment this define out to drop support for libm functions
#define HAS_LIBM
#ifdef HAS_LIBM
#include <cmath>
#else
#define NO_LIBM_SUPPORT "no libm support"
#endif

#define USE_STD
// Comment this define out to drop support for standard library functions.
// This allows the program to run without a runtime.

#ifdef USE_STD
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>


#else
#define NO_STD "no standard library support"
#endif


////////////////////////////////////////////////////////////////////////////////
/// REQUIRED INCLUDES //////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <exception>

#include "wlisp.hpp"

////////////////////////////////////////////////////////////////////////////////
/// ERROR MESSAGES /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define TOO_FEW_ARGS "too few arguments to function"
#define TOO_MANY_ARGS "too many arguments to function"
#define INVALID_ARGUMENT "invalid argument"
#define MISMATCHED_TYPES "mismatched types"
#define CALL_NON_FUNCTION "called non-function"
#define UNKNOWN_ERROR "unknown exception"
#define INVALID_LAMBDA "invalid lambda"
#define INVALID_BIN_OP "invalid binary operation"
#define INVALID_ORDER "cannot order expression"
#define BAD_CAST "cannot cast"
#define ATOM_NOT_DEFINED "atom not defined"
#define EVAL_EMPTY_LIST "evaluated empty list"
#define INTERNAL_ERROR "interal virtual machine error"
#define INDEX_OUT_OF_RANGE "index out of range"
#define MALFORMED_PROGRAM "malformed program"

////////////////////////////////////////////////////////////////////////////////
/// TYPE NAMES /////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define STRING_TYPE "string"
#define INT_TYPE "int"
#define FLOAT_TYPE "float"
#define UNIT_TYPE "unit"
#define FUNCTION_TYPE "function"
#define ATOM_TYPE "atom"
#define QUOTE_TYPE "quote"
#define LIST_TYPE "list"
#define USERDATA_TYPE "user-data"

////////////////////////////////////////////////////////////////////////////////
/// HELPER FUNCTIONS ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void load_default_lib();


namespace lisp{

static std::map<std::string, Value> g_global_defs;
static std::map<std::string, const char *> g_global_defs_doc;
static volatile bool g_running = true;

std::string read_file_contents( std::string filename) {
    std::ifstream f;
    f.open(filename.c_str());
    if (!f)
        throw std::runtime_error("could not open file");

    f.seekg(0, std::ios::end);
    std::string contents;
    contents.reserve(f.tellg());
    f.seekg(0, std::ios::beg);
    contents.assign(std::istreambuf_iterator<char>(f),
                std::istreambuf_iterator<char>());
    f.close();

    return contents;
}

std::shared_ptr<user_data> user_data::shared_from_this()
{
    return std::enable_shared_from_this<user_data>::shared_from_this();
}

Value user_data::apply(const std::vector<Value> &args, Environment &env)
{
	throw Error(Value("user-data",this->shared_from_this()), env, "user-data not support function");
}

// Convert an object to a string using a stringstream conveniently
template<class T>
std::string to_string(T x) {
    std::ostringstream ss;
    ss << std::dec << x;
    return ss.str();
}

// Replace a substring with a replacement string in a source string
static
void replace_substring(std::string &src, const std::string &substr, const std::string &replacement) {
    size_t i=0;
    for (i=src.find(substr, i); i!=std::string::npos; i=src.find(substr, i)) {
        src.replace(i, substr.size(), replacement);
        i += replacement.size();
    }
}

// Is this character a valid lisp symbol character
static
bool is_symbol(char ch) {
    return (isalpha(ch) || ispunct(ch)) && ch != '(' && ch != ')' && ch != '"' && ch != '\'';
}

////////////////////////////////////////////////////////////////////////////////
/// LISP CONSTRUCTS ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Construct a quoted value
Value Value::quote(const Value &quoted) {
	Value result;
	result.type = QUOTE;

	// The first position in the list is
	// used to store the quoted expression.
	result.list.push_back(quoted);
	return result;
}

// Construct an atom
Value Value::atom(const std::string &s) {
	Value result;
	result.type = ATOM;

	// We use the `str` member to store the atom.
	result.str = s;
	return result;
}

// Construct a string
Value Value::string(const std::string &s) {
	Value result;
	result.type = STRING;

	// We use the `str` member to store the string.
	result.str = s;
	return result;
}

Value Value::nil() {
	Value result;
	result.type = NIL;
	return result;
}

// Construct a lambda function
Value::Value(const std::vector<Value> &params, const Value &ret, Environment const &env) : type(LAMBDA),userdata(0) {
	// We store the params and the result in the list member
	// instead of having dedicated members. This is to save memory.
	list.push_back(Value(params));
	list.push_back(ret);

	// Lambdas capture only variables that they know they will use.
	std::vector<std::string> used_atoms = ret.get_used_atoms();
	for (size_t i=0; i<used_atoms.size(); i++) {
		// If the environment has a symbol that this lambda uses, capture it.
		if (env.has(used_atoms[i]))
			lambda_scope.set(used_atoms[i], env.get(used_atoms[i]));
	}
}

// Construct a builtin function
//Value::Value(std::string name, Builtin b) : type(BUILTIN) {
	// Store the name of the builtin function in the str member
	// to save memory, and use the builtin function slot in the union
	// to store the function pointer.
	//str = name;
	//stack_data.b = b;
//}

// Construct a builtin function
Value::Value(std::string name, std::function<Value(std::vector<Value>, Environment &)>bf) : type(BUILTIN) {
	// Store the name of the builtin function in the str member
	// to save memory, and use the builtin function slot in the union
	// to store the function pointer.
	str = name;
	//stack_data.b = 0;
	this->b = bf;
}


Value::Value(std::string name, std::shared_ptr<user_data> ud) : str(name), type(USERDATA),userdata(ud) {
}


////////////////////////////////////////////////////////////////////////////////
/// C++ INTEROP METHODS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Get all of the atoms used in a given Value
std::vector<std::string> Value::get_used_atoms() const{
	std::vector<std::string> result, tmp;
	switch (type) {
	case QUOTE:
		// The data for a quote is stored in the
		// first slot of the list member.
		return list[0].get_used_atoms();
	case ATOM:
		// If this is an atom, add it to the list
		// of used atoms in this expression.
		result.push_back(as_atom());
		return result;
	case LAMBDA:
		// If this is a lambda, get the list of used atoms in the body
		// of the expression.
		return list[1].get_used_atoms();
	case LIST:
		// If this is a list, add each of the atoms used in all
		// of the elements in the list.
		for (size_t i=0; i<list.size(); i++) {
			// Get the atoms used in the element
			tmp = list[i].get_used_atoms();
			// Add the used atoms to the current list of used atoms
			result.insert(result.end(), tmp.begin(), tmp.end());
		}
		return result;
	default:
		return result;
	}
}

bool Value::is_number() const {
	return type == INT || type == FLOAT;
}

// Get the "truthy" boolean value of this value.
bool Value::as_bool() const {
	return *this != Value(0);
}

// Get this item's integer value
int Value::as_int() const {
	return cast_to_int().stack_data.i;
}

// Get this item's floating point value
double Value::as_float() const {
	return cast_to_int().stack_data.f;
}

// Get this item's string value
std::string Value::as_string() const {
	// If this item is not a string, throw a cast error.
	if (type != STRING)
		throw Error(*this, Environment(), BAD_CAST);
	return str;
}

// Get this item's user data value
std::shared_ptr<user_data> Value::as_user_data() const {
	// If this item is not a string, throw a cast error.
	if (type != USERDATA)
		throw Error(*this, Environment(), BAD_CAST);
	return this->userdata;
}

// Get this item's atom value
std::string Value::as_atom() const {
	// If this item is not an atom, throw a cast error.
	if (type != ATOM)
		throw Error(*this, Environment(), BAD_CAST);
	return str;
}

// Get this item's list value
std::vector<Value> Value::as_list() const {
	// If this item is not a list, throw a cast error.
	if (type != LIST)
		throw Error(*this, Environment(), BAD_CAST);
	return list;
}

// Push an item to the end of this list
void Value::push(const Value &val) {
	// If this item is not a list, you cannot push to it.
	// Throw an error.
	if (type != LIST)
		throw Error(*this, Environment(), MISMATCHED_TYPES);

	list.push_back(val);
}

// Push an item from the end of this list
Value Value::pop() {
	// If this item is not a list, you cannot pop from it.
	// Throw an error.
	if (type != LIST)
		throw Error(*this, Environment(), MISMATCHED_TYPES);

	// Remember the last item in the list
	Value result = list[list.size()-1];
	// Remove it from this instance
	list.pop_back();
	// Return the remembered value
	return result;
}

////////////////////////////////////////////////////////////////////////////////
/// TYPECASTING METHODS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Cast this to an integer value
Value Value::cast_to_int() const {
	switch (type) {
	case INT: return *this;
    case STRING: {
        return Value(int(std::atol( this->str.c_str() )));
    }
	case FLOAT: return Value(int(stack_data.f));
	// Only ints and floats can be cast to an int
	default:
		throw Error(*this, Environment(), BAD_CAST);
	}
}

// Cast this to a floating point value
Value Value::cast_to_float() const {
	switch (type) {
	case FLOAT: return *this;
    case STRING: {
        return Value(double(std::atof( this->str.c_str() )));
    }
	case INT: return Value(float(stack_data.i));
	// Only ints and floats can be cast to a float
	default:
		throw Error(*this, Environment(), BAD_CAST);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// COMPARISON OPERATIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Value::operator==(const Value &other) const {
	// If either of these values are floats, promote the
	// other to a float, and then compare for equality.
	if (type == FLOAT && other.type == INT) return *this == other.cast_to_float();
	else if (type == INT && other.type == FLOAT) return this->cast_to_float() == other;
	// If the values types aren't equal, then they cannot be equal.
	else if (type != other.type) return false;

	switch (type) {
	case FLOAT:
		return stack_data.f == other.stack_data.f;
	case INT:
		return stack_data.i == other.stack_data.i;
	case BUILTIN:
        throw Error(*this, Environment(), BAD_CAST);
		//return this->b == other.b;
	case STRING:
	case ATOM:
		// Both atoms and strings store their
		// data in the str member.
		return str == other.str;
	case LAMBDA:
	case LIST:
		// Both lambdas and lists store their
		// data in the list member.
		return list == other.list;
	case QUOTE:
		// The values for quotes are stored in the
		// first slot of the list member.
		return list[0] == other.list[0];
	default:
		return true;
	}
}

bool Value::operator!=(const Value &other) const {
	return !(*this == other);
}

// bool operator<(Value other) const {
//     if (other.type != FLOAT && other.type != INT)
//         throw Error(*this, Environment(), INVALID_BIN_OP);

//     switch (type) {
//     case FLOAT:
//         return stack_data.f < other.cast_to_float().stack_data.f;
//     case INT:
//         if (other.type == FLOAT)
//             return cast_to_float().stack_data.f < other.stack_data.f;
//         else return stack_data.i < other.stack_data.i;
//     default:
//         throw Error(*this, Environment(), INVALID_ORDER);
//     }
// }

////////////////////////////////////////////////////////////////////////////////
/// ORDERING OPERATIONS ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Value::operator>=(const Value &other) const {
	return !(*this < other);
}

bool Value::operator<=(const Value &other) const {
	return (*this == other) || (*this < other);
}

bool Value::operator>(const Value &other) const {
	return !(*this <= other);
}

bool Value::operator<(const Value &other) const {
	// Other type must be a float or an int
	if (other.type != FLOAT && other.type != INT)
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	case FLOAT:
		// If this is a float, promote the other value to a float and compare.
		return stack_data.f < other.cast_to_float().stack_data.f;
	case INT:
		// If the other value is a float, promote this value to a float and compare.
		if (other.type == FLOAT)
			return cast_to_float().stack_data.f < other.stack_data.f;
		// Otherwise compare the integer values
		else return stack_data.i < other.stack_data.i;
	default:
		// Only allow comparisons between integers and floats
		throw Error(*this, Environment(), INVALID_ORDER);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// ARITHMETIC OPERATIONS //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// This function adds two lisp values, and returns the lisp value result.
Value Value::operator+(const Value &other) const {
	// If the other value's type is the unit type,
	// don't even bother continuing.
	// Unit types consume all arithmetic operations.
	if (other.type == UNIT) return other;

	// Other type must be a float or an int
	if ((is_number() || other.is_number()) &&
		!(is_number() && other.is_number()))
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	case FLOAT:
		// If one is a float, promote the other by default and do
		// float addition.
		return Value(stack_data.f + other.cast_to_float().stack_data.f);
	case INT:
		// If the other type is a float, go ahead and promote this expression
		// before continuing with the addition.
		if (other.type == FLOAT)
			return Value(cast_to_float() + other.stack_data.f);
		// Otherwise, do integer addition.
		else return Value(stack_data.i + other.stack_data.i);
	case STRING:
		// If the other value is also a string, do the concat
		if (other.type == STRING)
			return Value::string(str + other.str);
		// We throw an error if we try to concat anything of non-string type
		else throw Error(*this, Environment(), INVALID_BIN_OP);
	case LIST:
		// If the other value is also a list, do the concat
		if (other.type == LIST) {
			// Maintain the value that will be returned
			Value result = *this;
			// Add each item in the other list to the end of this list
			for (size_t i=0; i<other.list.size(); i++)
				result.push(other.list[i]);
			return result;

		} else throw Error(*this, Environment(), INVALID_BIN_OP);
	case UNIT:
		return *this;
	default:
		throw Error(*this, Environment(), INVALID_BIN_OP);
	}
}

// This function subtracts two lisp values, and returns the lisp value result.
Value Value::operator-(const Value &other) const {
	// If the other value's type is the unit type,
	// don't even bother continuing.
	// Unit types consume all arithmetic operations.
	if (other.type == UNIT) return other;

	// Other type must be a float or an int
	if (other.type != FLOAT && other.type != INT)
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	case FLOAT:
		// If one is a float, promote the other by default and do
		// float subtraction.
		return Value(stack_data.f - other.cast_to_float().stack_data.f);
	case INT:
		// If the other type is a float, go ahead and promote this expression
		// before continuing with the subtraction
		if (other.type == FLOAT)
			return Value(cast_to_float().stack_data.f - other.stack_data.f);
		// Otherwise, do integer subtraction.
		else return Value(stack_data.i - other.stack_data.i);
	case UNIT:
		// Unit types consume all arithmetic operations.
		return *this;
	default:
		// This operation was done on an unsupported type
		throw Error(*this, Environment(), INVALID_BIN_OP);
	}
}

// This function multiplies two lisp values, and returns the lisp value result.
Value Value::operator*(const Value &other) const {
	// If the other value's type is the unit type,
	// don't even bother continuing.
	// Unit types consume all arithmetic operations.
	if (other.type == UNIT) return other;

	// Other type must be a float or an int
	if (other.type != FLOAT && other.type != INT)
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	case FLOAT:
		return Value(stack_data.f * other.cast_to_float().stack_data.f);
	case INT:
		// If the other type is a float, go ahead and promote this expression
		// before continuing with the product
		if (other.type == FLOAT)
			return Value(cast_to_float().stack_data.f * other.stack_data.f);
		// Otherwise, do integer multiplication.
		else return Value(stack_data.i * other.stack_data.i);
	case UNIT:
		// Unit types consume all arithmetic operations.
		return *this;
	default:
		// This operation was done on an unsupported type
		throw Error(*this, Environment(), INVALID_BIN_OP);
	}
}

// This function divides two lisp values, and returns the lisp value result.
Value Value::operator/(const Value &other) const {
	// If the other value's type is the unit type,
	// don't even bother continuing.
	// Unit types consume all arithmetic operations.
	if (other.type == UNIT) return other;

	// Other type must be a float or an int
	if (other.type != FLOAT && other.type != INT)
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	case FLOAT:
		return Value(stack_data.f / other.cast_to_float().stack_data.f);
	case INT:
		// If the other type is a float, go ahead and promote this expression
		// before continuing with the product
		if (other.type == FLOAT)
			return Value(cast_to_float().stack_data.f / other.stack_data.f);
		// Otherwise, do integer multiplication.
		else return Value(stack_data.i / other.stack_data.i);
	case UNIT:
		// Unit types consume all arithmetic operations.
		return *this;
	default:
		// This operation was done on an unsupported type
		throw Error(*this, Environment(), INVALID_BIN_OP);
	}
}

// This function finds the remainder of two lisp values, and returns the lisp value result.
Value Value::operator%(const Value &other) const {
	// If the other value's type is the unit type,
	// don't even bother continuing.
	// Unit types consume all arithmetic operations.
	if (other.type == UNIT) return other;

	// Other type must be a float or an int
	if (other.type != FLOAT && other.type != INT)
		throw Error(*this, Environment(), INVALID_BIN_OP);

	switch (type) {
	// If we support libm, we can find the remainder of floating point values.
	#ifdef HAS_LIBM
	case FLOAT:
		return Value(fmod(stack_data.f, other.cast_to_float().stack_data.f));
	case INT:
		if (other.type == FLOAT)
			return Value(fmod(cast_to_float().stack_data.f, other.stack_data.f));
		else return Value(stack_data.i % other.stack_data.i);

	#else
	case INT:
		// If we do not support libm, we have to throw errors for floating point values.
		if (other.type != INT)
			throw Error(other, Environment(), NO_LIBM_SUPPORT);
		return Value(stack_data.i % other.stack_data.i);
	#endif

	case UNIT:
		// Unit types consume all arithmetic operations.
		return *this;
	default:
		// This operation was done on an unsupported type
		throw Error(*this, Environment(), INVALID_BIN_OP);
	}
}

// Get the name of the type of this value
std::string Value::get_type_name() const {
	switch (type) {
	case QUOTE: return QUOTE_TYPE;
	case ATOM: return ATOM_TYPE;
	case INT: return INT_TYPE;
	case FLOAT: return FLOAT_TYPE;
	case LIST: return LIST_TYPE;
	case STRING: return STRING_TYPE;
	case BUILTIN:
	case LAMBDA:
		// Instead of differentiating between
		// lambda and builtin types, we group them together.
		// This is because they are both callable.
		return FUNCTION_TYPE;
	case UNIT:
		return UNIT_TYPE;
	case USERDATA:
		return std::string(USERDATA_TYPE) + "-" + this->as_user_data()->subtype();
	default:
		// We don't know the name of this type.
		// This isn't the users fault, this is just unhandled.
		// This should never be reached.
		throw Error(*this, Environment(), INTERNAL_ERROR);
	}
}

std::string Value::display() const {
	std::string result;
	switch (type) {
	case QUOTE:
		return "'" + list[0].debug();
	case ATOM:
		return str;
	case INT:
		return to_string(stack_data.i);
	case FLOAT:
		return to_string(stack_data.f);
	case STRING:
		return str;
	case LAMBDA:
		for (size_t i=0; i<list.size(); i++) {
			result += list[i].debug();
			if (i < list.size()-1) result += " ";
		}
		return "(lambda " + result + ")";
	case LIST:
		for (size_t i=0; i<list.size(); i++) {
			result += list[i].debug();
			if (i < list.size()-1) result += " ";
		}
		return "(" + result + ")";
	case BUILTIN:
		return "<" + str + " at " + this->b.target_type().name() + ">";
	case UNIT:
		return "@";
	case NIL:
		return "<nil>";
	case USERDATA:
		if (this->userdata){
			return this->userdata->display();
		} else {
			return "<UD-" + str + " at " + to_string(long(this->userdata.get())) + ">";
		}
	default:
		// We don't know how to display whatever type this is.
		// This isn't the users fault, this is just unhandled.
		// This should never be reached.
		throw Error(*this, Environment(), INTERNAL_ERROR);
	}
}

std::string Value::debug() const {
	std::string result;
	switch (type) {
	case QUOTE:
		return "'" + list[0].debug();
	case ATOM:
		return str;
	case INT:
		return to_string(stack_data.i);
	case FLOAT:
		return to_string(stack_data.f);
	case STRING:
		for (size_t i=0; i<str.length(); i++) {
			if (str[i] == '"') result += "\\\"";
			else result.push_back(str[i]);
		}
		return "\"" + result + "\"";
	case LAMBDA:
		for (size_t i=0; i<list.size(); i++) {
			result += list[i].debug();
			if (i < list.size()-1) result += " ";
		}
		return "(lambda " + result + ")";
	case LIST:
		for (size_t i=0; i<list.size(); i++) {
			result += list[i].debug();
			if (i < list.size()-1) result += " ";
		}
		return "(" + result + ")";
	case BUILTIN:
        return "<" + str + " at " + this->b.target_type().name() + ">";
	case UNIT:
		return "@";
	case NIL:
		return "<nil>";
    case USERDATA:
        if (this->userdata) {
            return this->userdata->display();
        }
        else {
            return "#userdata#";
        }
	default:
		// We don't know how to debug whatever type this is.
		// This isn't the users fault, this is just unhandled.
		// This should never be reached.
		throw Error(*this, Environment(), INTERNAL_ERROR);
	}
}

std::ostream &operator<<(std::ostream &os, Value const &v) {
	return os << v.display();
}



Error::Error(Value v, Environment const &env, const char *msg) : env(env), msg(msg) {
    static char static_buffer[2048];
    this->msg = static_buffer;
    strcpy(static_buffer, msg);
    cause = new Value;
    *cause = v;
}
void Error::set_cause(const Value &v)
{
    if (cause){
        *cause = v;
    }
}

Error::Error( const Error &other) : env(other.env), msg(other.msg) {
    cause = new Value(*other.cause);
}

Error::~Error() {
    delete cause;
}

std::string Error::description() {
    return "error: the expression `" + cause->debug() + "` failed in scope " + to_string(env) + " with message \"" + msg + "\"";
}

void Environment::combine(const Environment &other) {
    // Normally, I would use the `insert` method of the `map` class,
    // but it doesn't overwrite previously declared values for keys.
    std::map<std::string, Value>::const_iterator itr = other.defs.begin();
    for (; itr!=other.defs.end(); itr++) {
        // Iterate through the keys and assign each value.
        defs[itr->first] = itr->second;
    }
}

std::ostream &operator<<(std::ostream &os, const Environment &e) {
    std::map<std::string, Value>::const_iterator itr = e.defs.begin();
    os << "{ ";
    for (; itr != e.defs.end(); itr++) {
        os << '\'' << itr->first << "' : " << itr->second.debug() << ", ";
    }
    return os << "}";
}

void Environment::set(const std::string &name, const Value &value) {
    defs[name] = value;
}

void Environment::setp(const std::string &name, const Value &value)
{
    Environment *ptr = this->parent_scope;
    while (ptr != NULL){
        std::map<std::string, Value>::iterator itr = ptr->defs.find(name);
        if (itr != ptr->defs.end()){
            itr->second = value;
            return ;
        }
        ptr = ptr->parent_scope;
    }
    throw Error(Value::string("setp"), *this, "var not found in parent");
}


Value Value::apply(const std::vector<Value> &args, Environment &env) {
    Environment e;
    std::vector<Value> params;
    bool is_varlist = false;
    switch (type) {
    case LAMBDA:
        // Get the list of parameter atoms
        params = list[0].list;
        for (int i=0; i<params.size(); i++){
            auto &li = params[i];
            if ( li.as_atom() == "..."){
                is_varlist = true;
                params.resize(i);
            }
        }
        if (is_varlist){
            if (args.size() < params.size() ){
                throw Error(Value(args), env, args.size() > params.size()?
                    TOO_MANY_ARGS : TOO_FEW_ARGS
                );
            }
        } else if (params.size() != args.size())
            throw Error(Value(args), env, args.size() > params.size()?
                TOO_MANY_ARGS : TOO_FEW_ARGS
            );

        // Get the captured scope from the lambda
        e = lambda_scope;
        // And make this scope the parent scope
        e.set_parent_scope(&env);

        // Iterate through the list of parameters and
        // insert the arguments into the scope.
        for (size_t i=0; i<params.size(); i++) {
            if (params[i].type != ATOM) 
                throw Error(*this, env, INVALID_LAMBDA);
            // Set the parameter name into the scope.
            e.set(params[i].str, args[i]);
        }

        if (is_varlist){
            for (int i=params.size(); i<args.size(); i++){
                e.set(std::string("{") + std::to_string(i-params.size())+ "}", args[i] );
            }
            e.set(std::string("{size}"), lisp::Value(int(args.size()-params.size())));
        }
        try {
            return list[1].eval(e);
        }catch (func_return &r){
            return r.value;
        }
        
#if 0
        // Evaluate the function body with the function scope
        if (list[1].is_list()){
            auto l = list[1].as_list();
            int n = l.size();
            if (n == 0){
                throw Error(*this, env, INVALID_LAMBDA " empty function body");
            }
            for (int i=0; i<n-1; i++){
                list[i].eval(e);
            }
            return list[n-1].eval(e);
        } else {
            return list[1].eval(e);
        }
#endif
        
    case BUILTIN:
        // Here, we call the builtin function with the current scope.
        // This allows us to write special forms without syntactic sugar.
        // For functions that are not special forms, we just evaluate
        // the arguments before we run the function.
        if (extened_buildin){
            return extened_buildin(this, env, [this, &args, &env]()->Value{
                return this->b(args, env);
            });
        } else {
            return this->b(args, env);
        }
        
    case USERDATA:
    	// call user data to construct object
        return this->userdata->apply(args, env);
    case NIL:
    case UNIT:
        return *this;
        break;
    default:
        // We can only call lambdas and builtins
        throw Error(*this, env, CALL_NON_FUNCTION);
    }
}


Value Value::eval(Environment &env) const {
    std::vector<Value> args;
    Value function;
    Environment e;

    if (!g_running){
        ctrl_c_event e(env);
        e.value = *this;
        throw e;
    }
    
    switch (type) {
    case QUOTE:
        return list[0];
    case ATOM:
        return env.get(str);
    case LIST:
        if (list.size() < 1)
            throw Error(*this, env, EVAL_EMPTY_LIST);

        args = std::vector<Value>(list.begin() + 1, list.end());
        
        // Only evaluate our arguments if it's not builtin!
        // Builtin functions can be special forms, so we
        // leave them to evaluate their arguments.
        function = list[0].eval(env);

        if (!function.is_builtin())
        {
            for (size_t i=0; i<args.size(); i++)
            {
                args[i] = args[i].eval(env);
            }
        }
        return function.apply(args,env);
    default:
        return *this;
    }
}

static
void skip_whitespace(std::string &s, int &ptr) {
    int c = 0;
    for (int c = s[ptr]; isspace(c) || c == ';'; ) {
        if (++ptr > s.length()){
            break;
        }
        if (c == ';'){
            while ( (c=s[ptr]) != '\n' && ptr< s.length()){
                ptr++;
            }
        }
        c = s[ptr];
    }
}

// Parse a single value and increment the pointer
// to the beginning of the next value to parse.
static
Value parse(std::string &s, int &ptr) {
    skip_whitespace(s, ptr);
#if 0
    while (s[ptr] == ';') {
        // If this is a comment
        int save_ptr = ptr;
        while (s[save_ptr] != '\n' && save_ptr < int(s.length())) { save_ptr++; }
        s.erase(ptr, save_ptr - ptr);
        skip_whitespace(s, ptr);

        if (s.substr(ptr, s.length()-ptr-1) == "")
            return Value();
    }
#else
    if (s.substr(ptr, s.length()-ptr-1) == "")
        return Value();
#endif


    if (s == "") {
        return Value();
    } else if (s[ptr] == '\'') {
        // If this is a quote
        ptr++;
        return Value::quote(parse(s, ptr));

    } else if (s[ptr] == '(') {
        int ptr_old = ptr;
        // If this is a list
        skip_whitespace(s, ++ptr);

        Value result = Value(std::vector<Value>());

        while (s[ptr] != ')'){
            if ( ptr >= s.length()){
                int n = 100;
                std::cerr << "Error: mismatch `(` at `" << s.substr(ptr_old, 100) << "`" << std::endl;
                throw std::runtime_error(MALFORMED_PROGRAM);
            }
            result.push(parse(s, ptr));
        }
        skip_whitespace(s, ++ptr);
        return result;
        
    } else if (isdigit(s[ptr]) || (s[ptr] == '-' && isdigit(s[ptr + 1]))) {
        // If this is a number
        bool negate = s[ptr] == '-';
        if (negate) ptr++;
        
        int save_ptr = ptr;
        while (isdigit(s[ptr]) || s[ptr] == '.') ptr++;
        std::string n = s.substr(save_ptr, ptr);
        skip_whitespace(s, ptr);
        
        if (n.find('.') != std::string::npos)
            return Value((negate? -1 : 1) * atof(n.c_str()));
        else return Value((negate? -1 : 1) * atoi(n.c_str()));

    } else if (s[ptr] == '`' && s[ptr+1] == '`' && s[ptr+2] == '`') {
        // If this is a multi-line string
        int n = 3;
        while (!(s[ptr + n] == '`' && s[ptr + n +1] == '`'  && s[ptr + n + 2] == '`') ){
            if (ptr + n >= int(s.length()))
            {
                std::cerr << "Error: mismatch ``` with position " << ptr << std::endl;
                throw std::runtime_error(MALFORMED_PROGRAM);
            }
                
                
            if (s[ptr + n] == '\\') n++;
            n++;
        }

        std::string x = s.substr(ptr+3, n-3);
        ptr += n+3;
        skip_whitespace(s, ptr);

        // Iterate over the characters in the string, and
        // replace escaped characters with their intended values.
        for (size_t i=0; i<x.size(); i++) {
            if (x[i] == '\\' && x[i+1] == '\\')
                x.replace(i, 2, "\\");
            else if (x[i] == '\\' && x[i+1] == '"')
                x.replace(i, 2, "\"");
            else if (x[i] == '\\' && x[i+1] == 'n')
                x.replace(i, 2, "\n");
            else if (x[i] == '\\' && x[i+1] == 't')
                x.replace(i, 2, "\t");
            else if (x[i] == '\\' && x[i+1] == '`')
                x.replace(i, 2, "`");
        }

        return Value::string(x);
    }else if (s[ptr] == '\"') {
        // If this is a string
        int n = 1;
        while (s[ptr + n] != '\"') {
            if (ptr + n >= int(s.length()))
            {
                std::cerr << "Error: mismatch \"" << " at position " << ptr << std::endl;
                throw std::runtime_error(MALFORMED_PROGRAM);
            }
                
            if (s[ptr + n] == '\\') n++;
            n++;
        }

        std::string x = s.substr(ptr+1, n-1);
        ptr += n+1;
        skip_whitespace(s, ptr);

        // Iterate over the characters in the string, and
        // replace escaped characters with their intended values.
        for (size_t i=0; i<x.size(); i++) {
            if (x[i] == '\\' && x[i+1] == '\\')
                x.replace(i, 2, "\\");
            else if (x[i] == '\\' && x[i+1] == '"')
                x.replace(i, 2, "\"");
            else if (x[i] == '\\' && x[i+1] == 'n')
                x.replace(i, 2, "\n");
            else if (x[i] == '\\' && x[i+1] == 't')
                x.replace(i, 2, "\t");
        }

        return Value::string(x);
    } else if (s[ptr] == '@') {
        ptr++;
        skip_whitespace(s, ptr);
        return Value();

    } else if (is_symbol(s[ptr])) {
        // If this is a string
        int n = 0;
        while (is_symbol(s[ptr + n]) || isdigit(s[ptr + n]) ) {
            n++;
        }

        std::string x = s.substr(ptr, n);
        ptr += n;
        skip_whitespace(s, ptr);
        return Value::atom(x);
    } else {
        int pos = ptr - 20;
        if (pos < 0) {
            pos = 0;
        }
        int n = ptr + 20;
        if (n > s.npos){
            n -= s.npos;
        }
        std::cerr << "Error:`" << s.substr(ptr, n) << "...`" << std::endl;
        throw std::runtime_error(MALFORMED_PROGRAM);
    }
}

// Parse an entire program and get its list of expressions.
static
std::vector<Value> parse(std::string s) {
    int i=0, last_i=-1;
    std::vector<Value> result;

    // While the parser is making progress (while the pointer is moving right)
    // and the pointer hasn't reached the end of the string,
    while (last_i != i && i <= int(s.length()-1)) {
        // Parse another expression and add it to the list.
        last_i = i;
        result.push_back(parse(s, i));
    }
#if 0
    // If the whole string wasn't parsed, the program must be bad.
    if (i < int(s.length()))
    {
        std::cerr << "whole string wasn't parsed, postion:" << i << std::endl;
        throw std::runtime_error(MALFORMED_PROGRAM);
    }
#endif   

    // Return the list of values parsed.
    return result;
}

// Execute code in an environment
Value run(const std::string &code, Environment &env) {
    // Parse the code
    std::vector<Value> parsed = parse(code);
    // Iterate over the expressions and evaluate them
    // in this environment.
    if (parsed.size() == 0) {
        throw Error(Value::string("run"), env, "run empty code");
    }
    for (int i=0; i<int(parsed.size())-1; i++)
        parsed[i].eval(env);

    // Return the result of the last expression.
    return parsed[parsed.size()-1].eval(env);
}

void eval_args(std::vector<Value> &args, Environment &env) {
	for (size_t i=0; i<args.size(); i++)
		args[i] = args[i].eval(env);
}
// This namespace contains all the definitions of builtin functions
namespace builtin {
    // This function is NOT a builtin function, but it is used
    // by almost all of them.
    //
    // Special forms are just builtin functions that don't evaluate
    // their arguments. To make a regular builtin that evaluates its
    // arguments, we just call this function in our builtin definition.

    // Create a lambda function (SPECIAL FORM)
    Value lambda(std::vector<Value> args, Environment &env) {
        if (args.size() < 2)
            throw Error(Value("lambda", lambda), env, TOO_FEW_ARGS);

        if (args[0].get_type_name() != LIST_TYPE)
            throw Error(Value("lambda", lambda), env, INVALID_LAMBDA);

        return Value(args[0].as_list(), args[1], env);
    }

    // if-else (SPECIAL FORM)
    Value if_then_else(std::vector<Value> args, Environment &env) {
        if (args.size() != 3)
            throw Error(Value("if", if_then_else), env, args.size() > 3? TOO_MANY_ARGS : TOO_FEW_ARGS);
        if (args[0].eval(env).as_bool())
            return args[1].eval(env);
        else return args[2].eval(env);
    }

    // Define a variable with a value (SPECIAL FORM)
    Value define(std::vector<Value> args, Environment &env) {
        int n = args.size();
        if (n < 2 || (n % 2)){
            throw Error(Value("define", define), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        }

        Value result;
        for (int i=0; i<n; i+=2){
            result = args[i+1].eval(env);
            env.set(args[i].as_atom(), result);
        }

        return result;
    }

    Value do_block(std::vector<Value> args, Environment &env);
    // Define a function with parameters and a result expression (SPECIAL FORM)
    Value defun(std::vector<Value> args, Environment &env) {
        if (args.size() != 3)
            throw Error(Value("defun", defun), env, args.size() > 3? TOO_MANY_ARGS : TOO_FEW_ARGS);

        if (args[1].get_type_name() != LIST_TYPE)
            throw Error(Value("defun", defun), env, INVALID_LAMBDA);

        Value f = Value(args[1].as_list(), args[2], env);
        env.set(args[0].display(), f);
        return f;
    }

    // Loop over a list of expressions with a condition (SPECIAL FORM)
    Value while_loop(std::vector<Value> args, Environment &env) {
        Value acc;
        while (args[0].eval(env).as_bool()) {
            for (size_t i=1; i<args.size(); i++)
            {
                try {
                    acc = std::move(args[i].eval(env));
                }catch (loop_break &b){
                    acc = b.value;
                    goto done;
                }catch (loop_continue &c)
                {
                    acc = std::move(c.value);
                    break;
                }
            }
        }
        done:;
        return acc;
    }

    // Iterate through a list of values in a list (SPECIAL FORM)
    Value for_loop(std::vector<Value> args, Environment &env) {
        Value acc;
        std::vector<Value> list = args[1].eval(env).as_list();

        for (size_t i=0; i<list.size(); i++) {
            env.set(args[0].as_atom(), list[i]);
            for (size_t j=2; j<args.size(); j++)
            {
                try {
                    acc = args[j].eval(env);
                }catch (loop_break &b){
                    return b.value;
                }catch (loop_continue &c){
                    acc = std::move(c.value);
                    break;
                }
            }

        }
        return acc;
    }
    // Iterate through a list of values in a list (SPECIAL FORM)
    Value for_range(std::vector<Value> args, Environment &env) {
        Value acc;
        std::vector<Value> list = args[1].eval(env).as_list();
        if (list.size() < 2 || list.size() > 3){
            throw Error(Value("for-range", for_range), env, args.size() > 3? TOO_MANY_ARGS : TOO_FEW_ARGS);
        }
        int beg = list[0].as_int();
        int end = list[1].as_int();
        int step = 1;
        if (list.size() == 3){
            step = list[2].as_int();
        }

        for (int i=beg; i<end; i += step) {
            env.set(args[0].as_atom(), i);
            for (size_t j=2; j<args.size(); j++)
            {
                try {
                    acc = args[j].eval(env);
                }catch (loop_break &b){
                    return b.value;
                }catch (loop_continue &c){
                    acc = std::move(c.value);
                    break;
                }
            }
        }
        return acc;
    }

    // Evaluate a block of expressions in the current environment (SPECIAL FORM)
    Value do_block(std::vector<Value> args, Environment &env) {
        Value acc;
        for (size_t i=0; i<args.size(); i++)
            acc = args[i].eval(env);
        return acc;
    }

    // Evaluate a block of expressions in a new environment (SPECIAL FORM)
    Value scope(std::vector<Value> args, Environment &env) {
        Environment e = env;
        Value acc;
        for (size_t i=0; i<args.size(); i++)
            acc = args[i].eval(e);
        return acc;
    }

    // Quote an expression (SPECIAL FORM)
    Value quote(std::vector<Value> args, Environment &env) {
        std::vector<Value> v;
        for (size_t i=0; i<args.size(); i++)
            v.push_back(args[i]);
        return Value(v);
    }

    #ifdef USE_STD
    // Exit the program with an integer code
    Value exit(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        std::exit(args.size() < 1? 0 : args[0].cast_to_int().as_int());
        return Value();
    }

    // Print several values and return the last one
    Value print(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() < 1)
            throw Error(Value("print", print), env, TOO_FEW_ARGS);

        Value acc;
        for (size_t i=0; i<args.size(); i++) {
            acc = args[i];
            std::cout << acc.display();
            if (i < args.size() - 1)
                std::cout << " ";
        }
        std::cout << std::flush;
        return acc;
    }
    // Print several values and return the last one
    Value println(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() < 1)
            throw Error(Value("print", print), env, TOO_FEW_ARGS);

        Value acc;
        for (size_t i=0; i<args.size(); i++) {
            acc = args[i];
            std::cout << acc.display();
            if (i < args.size() - 1)
                std::cout << " ";
        }
        std::cout << std::endl;
        return acc;
    }

    // Get user input with an optional prompt
    Value input(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() > 1)
            throw Error(Value("input", input), env, TOO_MANY_ARGS);

        if (!args.empty())
            std::cout << args[0];

        std::string s;
        std::getline(std::cin, s);
        return Value::string(s);
    }

    // Get a random number between two numbers inclusively
    Value random(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("random", random), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);

        int low = args[0].as_int(), high = args[1].as_int();
        return Value(rand()%(high-low+1) + low);
    }

    // Get the contents of a file
    Value read_file(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("read-file", read_file), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        // return Value::string(content);
        return Value::string(read_file_contents(args[0].as_string()));
    }

    // Write a string to a file
    Value write_file(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("write-file", write_file), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::ofstream f;
        // The first argument is the file name
        f.open(args[0].as_string().c_str());
        // The second argument is the contents of the file to write
        Value result = Value((f << args[1].as_string())? 1 : 0);
        f.close();
        return result;
    }

    // Read a file and execute its code
    Value include(std::vector<Value> args, Environment &env) {
        // Import is technically not a special form, it's more of a macro.
        // We can evaluate our arguments.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("include", include), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        Value result = run(read_file_contents(args[0].as_string()), env);
        return result;
    }
    #endif

    // Evaluate a value as code
    Value eval(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);
        if (args.size() != 1)
            throw Error(Value("eval", eval), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        else{
            auto values = lisp::parse(args[0].as_string());
            lisp::Value ret;
            for (auto &v:values){
                ret = v.eval(env);
            }
            return ret;
        }
        
    }

    // Create a list of values
    Value list(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);
        
        return Value(args);
    }

    // Sum multiple values
    Value sum(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);
        
        if (args.size() < 2)
            throw Error(Value("+", sum), env, TOO_FEW_ARGS);
        
        Value acc = args[0];
        for (size_t i=1; i<args.size(); i++)
            acc = acc + args[i];
        return acc;
    }

    // Subtract two values
    Value subtract(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("-", subtract), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0] - args[1];
    }

    // Multiply several values
    Value product(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() < 2)
            throw Error(Value("*", product), env, TOO_FEW_ARGS);

        Value acc = args[0];
        for (size_t i=1; i<args.size(); i++)
            acc = acc * args[i];
        return acc;
    }

    // Divide two values
    Value divide(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("/", divide), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0] / args[1];
    }

    // Get the remainder of values
    Value remainder(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("%", remainder), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0] % args[1];
    }

    // Are two values equal?
    Value eq(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("=", eq), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] == args[1]));
    }

    // Are two values not equal?
    Value neq(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("!=", neq), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] != args[1]));
    }

    // Is one number greater than another?
    Value greater(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value(">", greater), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] > args[1]));
    }

    // Is one number less than another?
    Value less(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("<", less), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] < args[1]));
    }

    // Is one number greater than or equal to another?
    Value greater_eq(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value(">=", greater_eq), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] >= args[1]));
    }

    // Is one number less than or equal to another?
    Value less_eq(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("<=", less_eq), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return Value(int(args[0] <= args[1]));
    }

    // Get the type name of a value
    Value get_type_name(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("type", get_type_name), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        return Value::string(args[0].get_type_name());
    }

    // Cast an item to a float
    Value cast_to_float(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value(FLOAT_TYPE, cast_to_float), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0].cast_to_float();
    }

    // Cast an item to an int
    Value cast_to_int(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value(INT_TYPE, cast_to_int), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0].cast_to_int();
    }

    // Index a list
    Value index(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("index", index), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::vector<Value> list = args[0].as_list();
        int i = args[1].as_int();
        if (list.empty() || i >= list.size())
            throw Error(list, env, INDEX_OUT_OF_RANGE);

        return list[i];
    }

    // Insert a value into a list
    Value insert(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 3)
            throw Error(Value("insert", insert), env, args.size() > 3? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::vector<Value> list = args[0].as_list();
        int i = args[1].as_int();
        if (i > list.size())
            throw Error(list, env, INDEX_OUT_OF_RANGE);

        list.insert(list.begin() + args[1].as_int(), args[2]);
        return Value(list);
    }

    // Remove a value at an index from a list
    Value remove(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 2)
            throw Error(Value("remove", remove), env, args.size() > 2? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::vector<Value> list = args[0].as_list();
        int i = args[1].as_int();
        if (list.empty() || i >= list.size())
            throw Error(list, env, INDEX_OUT_OF_RANGE);

        list.erase(list.begin() + i);
        return Value(list);
    }

    // Get the length of a list
    Value len(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("len", len), env, args.size() > 1?
                TOO_MANY_ARGS : TOO_FEW_ARGS
            );
        
        return Value(int(args[0].as_list().size()));
    }

    // Add an item to the end of a list
    Value push(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() == 0)
            throw Error(Value("push", push), env, TOO_FEW_ARGS);
        for (size_t i=1; i<args.size(); i++)
            args[0].push(args[i]);
        return args[0];
    }

    Value pop(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("pop", pop), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        return args[0].pop();
    }

    Value head(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("head", head), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        std::vector<Value> list = args[0].as_list();
        if (list.empty())
            throw Error(Value("head", head), env, INDEX_OUT_OF_RANGE);

        return list[0];
    }

    Value tail(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("tail", tail), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::vector<Value> result, list = args[0].as_list();

        for (size_t i = 1; i<list.size(); i++)
            result.push_back(list[i]);
        
        return Value(result);
    }

    Value parse(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("parse", parse), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
        if (args[0].get_type_name() != STRING_TYPE)
            throw Error(args[0], env, INVALID_ARGUMENT);
        std::vector<Value> parsed = lisp::parse(args[0].as_string());

        // if (parsed.size() == 1)
        //     return parsed[0];
        // else return Value(parsed);
        return Value(parsed);
    }

    Value replace(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 3)
            throw Error(Value("replace", replace), env, args.size() > 3? TOO_MANY_ARGS : TOO_FEW_ARGS);

        std::string src = args[0].as_string();
        replace_substring(src, args[1].as_string(), args[2].as_string());
        return Value::string(src);
    }

    Value display(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("display", display), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        return Value::string(args[0].display());
    }

    Value debug(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        if (args.size() != 1)
            throw Error(Value("debug", debug), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);

        return Value::string(args[0].debug());
    }

    Value map_list(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        std::vector<Value> result, l=args[1].as_list(), tmp;
        for (size_t i=0; i<l.size(); i++) {
            tmp.push_back(l[i]);
            result.push_back(args[0].apply(tmp, env));
            tmp.clear();
        }
        return Value(result);
    }

    Value filter_list(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        std::vector<Value> result, l=args[1].as_list(), tmp;
        for (size_t i=0; i<l.size(); i++) {
            tmp.push_back(l[i]);
            if (args[0].apply(tmp, env).as_bool())
                result.push_back(l[i]);
            tmp.clear();
        }
        return Value(result);
    }

    Value reduce_list(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        std::vector<Value> l=args[2].as_list(), tmp;
        Value acc = args[1];
        for (size_t i=0; i<l.size(); i++) {
            tmp.push_back(acc);
            tmp.push_back(l[i]);
            acc = args[0].apply(tmp, env);
            tmp.clear();
        }
        return acc;
    }

    Value range(std::vector<Value> args, Environment &env) {
        // Is not a special form, so we can evaluate our args.
        eval_args(args, env);

        std::vector<Value> result;
        Value low = args[0], high = args[1];
        if (low.get_type_name() != INT_TYPE && low.get_type_name() != FLOAT_TYPE)
            throw Error(low, env, MISMATCHED_TYPES);
        if (high.get_type_name() != INT_TYPE && high.get_type_name() != FLOAT_TYPE)
            throw Error(high, env, MISMATCHED_TYPES);

        if (low >= high) return Value(result);

        while (low < high) {
            result.push_back(low);
            low = low + Value(1);
        }
        return Value(result);
    }
}

void lisp_try_break()
{
    g_running = false;
}
void repl(Environment &env) {
#ifdef USE_STD
    std::string code;
    std::string input;
    Value tmp;
    std::vector<Value> parsed;
    while (true) {
        g_running = true;
        std::cout << ">>> ";
        std::getline(std::cin, input);
        if (input == "!quit" || input == "!q")
            break;
        else if (input == "!env" || input == "!e")
            std::cout << env << std::endl;
        else if (input == "!export" || input == "!x") {
            std::cout << "File to export to: ";
            std::getline(std::cin, input);

            std::ofstream f;
            f.open(input.c_str(), std::ofstream::out);
            f << code;
            f.close();
        } else if (input != "") {
            try {
                tmp = run(input, env);
                std::cout << " => " << tmp.debug() << std::endl;
                code += input + "\n";
            } catch (Error &e) {
                std::cerr << e.description() << std::endl;
            }catch (lisp::ctrl_c_event &e){
		        std::cerr << "ctrl+c break" << e.value.display() << std::endl;
	        } catch (std::runtime_error &e) {
                std::cerr << e.what() << std::endl;
            } catch (lisp::func_return &e){
                std::cout << "return should be used in function:" << e.value.display() << std::endl;
            } catch (lisp::loop_continue &e){
                std::cout << "continue should be used in loop:" << e.value.display() << std::endl;
            } catch (lisp::loop_break &e){
                std::cout << "break should be used in loop:" << e.value.display() << std::endl;
            } catch (lisp::exception &e){
                std::cout << "unhandled lisp-exception:" << e.value.display() << std::endl;
            }
        }
    }
#endif
}

// Does this environment, or its parent environment, have a variable?
bool Environment::has(const std::string &name) const {
    // Find the value in the map
    std::map<std::string, Value>::const_iterator itr = defs.find(name);
    if (itr != defs.end())
        // If it was found
        return true;
    else if (parent_scope != NULL)
        // If it was not found in the current environment,
        // try to find it in the parent environment
        return parent_scope->has(name);
    else return false;
}


void global_set(std::string name, Value v, const char *doc)
{
	g_global_defs[name] = v;
    g_global_defs_doc[name] = doc;
}

static bool __do_and(const Value &v)
{
    if (v.is_list()){
        auto list = v.as_list();
        for (auto &li:list){
            if (!__do_and(li)){
                return false;
            }
        }
        
    } else if (!v.as_bool()){
        return false;
    }
    return true;
}
static bool __do_or(const Value &v)
{
    if (v.is_list()){
        auto list = v.as_list();
        for (auto &li:list){
            if (__do_or(li)){
                return true;
            }
        }
        
    } else if (v.as_bool()){
        return true;
    }
    return false;
}


class global_init
{
public:
	global_init(){
#define __DEF_ONE(xx, f) g_global_defs[#xx] = Value(#xx,  f)
#define __DEF_ONE_DOC(xx, doc) g_global_defs_doc[#xx] = doc
		// Meta operations
		__DEF_ONE(eval, builtin::eval);
		__DEF_ONE(type, builtin::get_type_name);
		__DEF_ONE(parse, builtin::parse);
		// Special forms
		__DEF_ONE(do, builtin::do_block);
		__DEF_ONE(if, builtin::if_then_else);
		__DEF_ONE(for, builtin::for_loop);
		__DEF_ONE(for-range, builtin::for_range);
		__DEF_ONE(while, builtin::while_loop);
		__DEF_ONE(scope, builtin::scope);
		__DEF_ONE(quote, builtin::quote);
		__DEF_ONE(defun, builtin::defun);
		__DEF_ONE(define, builtin::define);
		__DEF_ONE(setq, builtin::define);
		__DEF_ONE(lambda, builtin::lambda);
		// Comparison operations
		__DEF_ONE(=, builtin::eq);
		__DEF_ONE(!=, builtin::neq);
		__DEF_ONE(>, builtin::greater);
		__DEF_ONE(<, builtin::less);
		__DEF_ONE(>=, builtin::greater_eq);
		__DEF_ONE(<=, builtin::less_eq);
		// Arithmetic operations
		__DEF_ONE(+, builtin::sum);
		__DEF_ONE(-, builtin::subtract);
		__DEF_ONE(*, builtin::product);
		__DEF_ONE(/, builtin::divide);
		__DEF_ONE(%, builtin::remainder);
	    // List operations
		__DEF_ONE(list, builtin::list);
		__DEF_ONE(insert, builtin::insert);
		__DEF_ONE(index, builtin::index);
		__DEF_ONE(remove, builtin::remove);
		__DEF_ONE(len, builtin::len);
		__DEF_ONE(length, builtin::len);
		__DEF_ONE(push, builtin::push);
		__DEF_ONE(pop, builtin::pop);
		__DEF_ONE(head, builtin::head);
		__DEF_ONE(tail, builtin::tail);
		__DEF_ONE(first, builtin::head);
		__DEF_ONE(last, builtin::pop);
		__DEF_ONE(range, builtin::range);
	    // Functional operations
		__DEF_ONE(map, builtin::map_list);
		__DEF_ONE(filter, builtin::filter_list);
		__DEF_ONE(reduce, builtin::reduce_list);
	    // IO operations
		__DEF_ONE(exit, builtin::exit);
		__DEF_ONE(quit, builtin::exit);
		__DEF_ONE(print, builtin::print);
		__DEF_ONE(println, builtin::println);
		__DEF_ONE(input, builtin::input);
		__DEF_ONE(random, builtin::random);
		__DEF_ONE(include, builtin::include);
		__DEF_ONE(read-file, builtin::read_file);
		__DEF_ONE(write-file, builtin::write_file);
	    // String operations
		__DEF_ONE(debug, builtin::debug);
		__DEF_ONE(replace, builtin::replace);
		__DEF_ONE(display, builtin::display);
	    // Casting operations
		__DEF_ONE(int, builtin::cast_to_int);
		__DEF_ONE(float, builtin::cast_to_float);
	    // Constants
		g_global_defs["endl"] = Value::string("\n");

        __DEF_ONE_DOC(read-file, "<filename> -> content:string");
        __DEF_ONE_DOC(filter, "<func> <list> -> filtered:list");
        __DEF_ONE_DOC(tail, "<list> -> list:list");
        __DEF_ONE_DOC(reduce, "<lambda(item)> <acc> <list> -> <acc>");
        __DEF_ONE_DOC(map, "<lambda(item)> <list> -> <new-list>");
        __DEF_ONE_DOC(replace, "<src> <substr> <replacement>");
        __DEF_ONE_DOC(tail, "<list> -> <list-first-removed>");
        __DEF_ONE_DOC(remove, "<list> <index> -> <new-list>");
        __DEF_ONE_DOC(write-file, "<filename> <content:string> -> success:bool");

        lisp::global_set("setp", lisp::Value("setp", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value{
            int n = args.size();
            if (args.size() < 2 || (args.size() % 2)){
                throw Error(Value::string("setp"), env, TOO_FEW_ARGS);
            }
            lisp::Value result = lisp::Value::nil();
            for (int i=0; i<n; i+=2){
                result = args[i + 1].eval(env);
                env.setp(args[i].as_atom(), result);
            }
            return result;
        }));

        lisp::global_set( "help", lisp::Value("help", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            lisp::eval_args(args, env);
            int n = 0;
            std::stringstream ss;
            ss << "wlisp golbal definitions:\n";
            for (auto &a:g_global_defs){
                n++;
                ss << "    " << a.first;
                int len = a.first.length();
                for (int i=len; i<24; i++){
                    ss << " ";
                }
                auto it = g_global_defs_doc.find(a.first);
                if (it != g_global_defs_doc.end() && strlen(it->second) > 0){
                    ss << it->second << "\n";
                } else {
                    ss << a.second.display().substr(0, 50) << "\n";
                }
            }
            ss << "\n";
            return lisp::Value::string(ss.str());

        }));
        lisp::global_set( "nil", lisp::Value::nil());
        lisp::global_set( "is-nil", lisp::Value("is-nil", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            lisp::eval_args(args, env);
            if (args.size() != 1){
                throw Error(Value::string("is-nil"), env, args.size() > 1? TOO_MANY_ARGS : TOO_FEW_ARGS);
            }
            return lisp::Value( int(args[args.size()-1].is_nil() ));
        })     
        );
        lisp::global_set( "break", lisp::Value("break", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            lisp::eval_args(args, env);
            loop_break b(env);
            if (args.size()>0){
                b.value = args[args.size()-1];
            }
            throw b;
        }),
            R"((break <list>))"        
        );
        lisp::global_set( "continue", lisp::Value("continue", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            lisp::eval_args(args, env);
            loop_continue b(env);
            if (args.size()>0){
                b.value = args[args.size()-1];
            }

            throw b;
        }), R"((continue <list>))"
        );
        lisp::global_set( "return", lisp::Value("return", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            lisp::eval_args(args, env);
            func_return b(env);
            if (args.size()>0){
                b.value = args[args.size()-1];
            }
            throw b;
        }), "(return <value> ...)");
        lisp::global_set( "and", lisp::Value("and", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            //lisp::eval_args(args, env);
            if (args.size() == 0){
                return lisp::Value::nil();
            } else {
                for (auto &a:args){
                    auto v = a.eval(env);
                    if (!__do_and(v)){
                        return lisp::Value(int(0));
                    }
                }
                return lisp::Value(int(1));
            }
        }), "(and <value> ...)");
        lisp::global_set( "or", lisp::Value("or", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            //lisp::eval_args(args, env);
            if (args.size() == 0){
                return lisp::Value::nil();
            } else {
                for (auto &a:args){
                    auto v = a.eval(env);
                    if (__do_or(v)){
                        return lisp::Value(int(1));
                    }
                }
                return lisp::Value(int(0));
            }
        }), "(or <value> ...)");
        lisp::global_set( "try", lisp::Value("try", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            //lisp::eval_args(args, env);
            if (args.size() < 3){
                throw Error(Value::string("try"), env, TOO_FEW_ARGS);
            }
            int n = args.size();
            std::vector<int> positions;
            positions.reserve(4);
            //int catch_pos = 0;
            for (int i=1; i<n; i++){
                if ( args[i].is_atom() && args[i].display() == "catch" ){
                    positions.push_back(i);
                    i++;
                    if (i >= n){
                        throw Error(Value::string("try"), env, TOO_FEW_ARGS);
                    }
                }
            }
            if (positions.size() == 0){
                throw Error(Value::string("try"), env, "catch not found");
            }
            std::vector<std::string> catch_types;
            std::vector<std::string> catch_vars;
            catch_types.reserve(positions.size());
            for (auto pos:positions){
                if (!args[pos+1].is_list()){
                    throw Error(Value::string("try"), env, "(args-list) expected after `catch`");
                }
                auto var_list = args[pos+1].as_list();
                if (var_list.size() == 1){
                    catch_types.push_back("");
                    catch_vars.push_back(var_list[0].as_atom());
                } else {
                    catch_types.push_back(var_list[0].as_atom());
                    catch_vars.push_back(var_list[1].as_atom());
                }
            }
            
            lisp::Value ret;
            try {
                int first_pos = positions[0];
                for (int i=0; i<first_pos; i++){
                    ret = args[i].eval(env);
                }
            }catch (lisp::exception &e) {
                const std::string &etype = e.type;
                for (int i=0; i<catch_types.size(); i++){
                    auto &t = catch_types[i];
                    if (t.empty() || t == etype){
                        lisp::Environment new_env;
                        new_env.combine(env);
                        new_env.set(catch_vars[i], e.value);
                        ret = e.value;
                        int end = n;
                        if ( i+1 < positions.size() ){
                            end = positions[i+1];
                        }
                        for (int pc=positions[i]+2; pc<end; pc++){
                            ret = args[pc].eval(new_env);
                        }
                        return ret;
                    }
                }
                throw e;
            }
            return ret;
        }), "(try ... catch ([type-name] var) ... [catch ([type-name] var)] ... )");
        lisp::global_set( "throw", lisp::Value("throw", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            int size = args.size();
            if (size != 1 && size != 2){
                throw Error(Value::string("throw"), env, "throw need 1/2 args");
            }
            lisp::exception b(env);
            if (size == 1){
                b.value = args[0].eval(env);
            }else {
                b.type  = args[0].as_atom();
                b.value = args[1].eval(env);
            }
            throw b;
        }), R"((throw [type] <value>))"
        );
        lisp::global_set( "is-list", lisp::Value("is-list", [](std::vector<lisp::Value> args, lisp::Environment& env)->lisp::Value {
            if (args.size() != 1){
                throw Error(Value::string("is-list"), env, "is-list need 1 arg");
            }
            lisp::eval_args(args, env);
            return lisp::Value(args[0].is_list());
        }), "<value>: is-list:bool"
        );


#undef __DEF_ONE
	}
};

std::function<Value(Value*, Environment &e, std::function<Value()>)> extened_buildin = nullptr;

global_init g_lisp_global_initer;

// Get the value associated with this name in this scope
Value Environment::get(const std::string &name) const
{
    const Environment *ptr = this;
    while (ptr != NULL){
        std::map<std::string, Value>::const_iterator itr = ptr->defs.find(name);
        if (itr != ptr->defs.end()){
            return itr->second;
        }
        ptr = ptr->parent_scope;
    }
    std::map<std::string, Value>::const_iterator itr = g_global_defs.find(name);
    if (itr != g_global_defs.end()){
        return itr->second;
    }
    throw Error(Value::atom(name), *this, ATOM_NOT_DEFINED);
}

}

