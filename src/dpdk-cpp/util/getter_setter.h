/*
 * getter_setter.h
 *
 *  Created on: Dec 18, 2014
 *      Author: yonch
 */

#ifndef UTIL_GETTER_SETTER_H_
#define UTIL_GETTER_SETTER_H_

/**
 * Adds a getter and setter.
 * @param class_name: the name of the class including the getter and setter
 * @param fn_name: name of the getter/setter function
 * @param type: the type of the struct to set
 * @param struct_name: the name of the struct
 */

#define GETTER_SETTER(class_name, fn_name, type, attr) 		\
	inline type fn_name()					\
	{																\
		return attr;												\
	}																\
	inline class_name & fn_name (type _val) 	\
	{																\
		attr = _val;													\
		return *this;												\
	}																\


#endif /* UTIL_GETTER_SETTER_H_ */
