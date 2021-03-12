
#include "Poco/File.h"
#include "Poco/DateTime.h"

#include "Poco/Exception.h"

#include "wlisp.hpp"


void w_exception_init()
{
	lisp::extened_buildin = [](lisp::Value* p_value, lisp::Environment &env,  std::function<lisp::Value()> default_func)->lisp::Value{
		try {
			return default_func();
		}catch (Poco::NullValueException &e)
		{
			lisp::exception le(env);
			le.type = "nullvalue";
			le.value = lisp::Value::string(e.displayText());
			throw le;
		}
		catch (Poco::NotFoundException &e){
			lisp::exception le(env);
			le.type = "notfound";
			le.value = lisp::Value::string(e.displayText());
			throw le;
		}
		catch (Poco::TimeoutException &e){
			lisp::exception le(env);
			le.type = "timeout";
			le.value = lisp::Value::string(e.displayText());
			throw le;
		}
		catch (Poco::Exception &e){
			lisp::exception le(env);
			le.value = lisp::Value::string(e.displayText());
			throw le;
		} catch (std::runtime_error &e){
			lisp::exception le(env);
			le.value = lisp::Value::string(e.what());
			throw le;
		}
		//  catch (lisp::ctrl_c_event &e){
		// 	throw e;
		// } catch (lisp::func_return &e){
		// 	throw e;
		// } catch (lisp::loop_continue &e){
		// 	throw e;
		// } catch (lisp::loop_break &e){
		// 	throw e;
		// } catch (lisp::exception &e){
		// 	throw e;
		// }
		 catch (lisp::Error &e){
			lisp::exception le(env);
			// le.set_cause(*p_value);
			le.value = lisp::Value::string(e.description());
			throw le;
		}
		return lisp::Value::nil();
	};
}

