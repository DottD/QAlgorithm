// QAlgorithm: a class for Qt/C++ implementing generic algorithm logic.
// Copyright (C) 2018  Filippo Santarelli
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Contact me at: filippo2.santarelli@gmail.com
//

/** \file qa_macros.h
 *  Convenience macros that improves usability.
 */

#ifndef _QA_MACRO
#define _QA_MACRO

#ifndef QA_IN
/** \brief Prefix for input properties. */
#define QA_IN "algin_"
#endif

#ifndef QA_OUT
/** \brief Prefix for output properties. */
#define QA_OUT "algout_"
#endif

#ifndef QA_PAR
/** \brief Prefix for parameters. */
#define QA_PAR "par_"
#endif

#ifndef QA_INPUT
/**
 * \brief Defines an input property for the algorithm.
 *
 * The purpose of this macro is to be used in subclasses of the QAlgorithm
 * class and allows to easily define an input to the algorithm.
 * This macro registers a property of type \e Type called \link QA_IN\endlink\<\e Name\> in the Qt's
 * MetaObject System (calling Q_PROPERTY). It automatically defines a member attribute
 * for the subclass called m_\link QA_IN\endlink\<\e Name\> using the keyword
 * MEMBER of Q_PROPERTY.\n
 * QA_INPUT generates setter and getter methods for the given property; the
 * name convention used is:
 *  - setIn\<\e Name\> for the setter
 *  - getIn\<\e Name\> for the const getter
 *  - getInRef\<\e Name\> for the getter that returns a reference to the property
 *  - getInMove\<\e Name\> for the move getter, that returns an rvalue to the property
 *
 * \param[in] Type Type of the property; must be registered in the Qt's MetaObject System.
 * \param[in] Name Name of the property.
 * 
 * \sa QA_INPUT_LIST, QA_INPUT_VEC, QA_OUTPUT, QA_PARAMETER
 */
#define QA_INPUT(Type, Name) 															\
Q_PROPERTY(Type algin_##Name MEMBER m_algin_##Name READ getIn##Name WRITE setIn##Name)	\
private:																				\
	Type m_algin_##Name;																\
public:																					\
	void setIn##Name (Type value){														\
		this->m_algin_##Name = value;													\
	}																					\
	Type getIn##Name () const{															\
		return this->m_algin_##Name;													\
	}																					\
	Type& getInRef##Name (){															\
		return std::ref(this->m_algin_##Name);											\
	}																					\
	Type&& getInMove##Name (){															\
		return std::move(this->m_algin_##Name);											\
	}
#endif

#ifndef QA_INPUT_LIST
/**
 * \brief Defines a list of input properties for the algorithm.
 *
 * The purpose of this macro is to be used in subclasses of the QAlgorithm
 * class and allows to easily define an input to the algorithm.
 * This macro registers a property of type \e Type called \link QA_IN\endlink\<\e Name\> in the Qt's
 * MetaObject System (calling Q_PROPERTY). It automatically defines two
 * member attributes using the keyword MEMBER of Q_PROPERTY:
 * - one called m_\link QA_IN\endlink\<\e Name\> and type \e Type
 * - one called m_listin_\<\e Name\> and type QList\<\e Type\>
 *
 * QA_INPUT_LIST generates setter and getter methods for the given property; the
 * name convention used is:
 *  - setIn\<\e Name\> for the setter; it takes a value of type \e Type as input
 *		and appends it to the list of inputs.
 *  - getIn\<\e Name\> for the const getter, that returns the list of inputs.
 *  - getInRef\<\e Name\> for the getter that returns a reference to the list of inputs.
 *  - getInMove\<\e Name\> for the move getter, that returns an rvalue to the list of inputs.
 *
 * \param[in] Type Type of a single property of the list; must be registered in the Qt's MetaObject System.
 * \param[in] Name Name of the property list.
 * 
 * \sa QA_INPUT, QA_INPUT_VEC, QA_OUTPUT, QA_PARAMETER
 */
#define QA_INPUT_LIST(Type, Name)											\
Q_PROPERTY(Type algin_##Name MEMBER m_algin_##Name WRITE setIn##Name)		\
private:																	\
	Type m_algin_##Name;													\
	QList<Type> m_listin_##Name;											\
public:																		\
	void setIn##Name (Type value){											\
		this->m_algin_##Name = value;										\
		this->m_listin_##Name << value;										\
	}																		\
	QList<Type> getIn##Name () const{										\
		return this->m_listin_##Name;										\
	}																		\
	QList<Type>& getInRef##Name (){											\
		return std::ref(this->m_listin_##Name);								\
	}																		\
	QList<Type>&& getInMove##Name (){										\
		return std::move(this->m_listin_##Name);							\
	}
#endif

#ifndef QA_INPUT_VEC
/**
 * \brief Defines a vector of input properties for the algorithm.
 *
 * The purpose of this macro is the same of QA_INPUT_LIST, but instead
 * of appending to a QList, uses a QVector. Its intent is to be used whenever
 * memory contiguity is of concern.
 *
 * \param[in] Type Type of a single property of the vector; must be registered in the Qt's MetaObject System.
 * \param[in] Name Name of the property list.
 * 
 * \sa QA_INPUT, QA_INPUT_LIST, QA_OUTPUT, QA_PARAMETER
 */
#define QA_INPUT_VEC(Type, Name)												\
Q_PROPERTY(Type algin_##Name MEMBER m_algin_##Name WRITE setIn##Name)			\
private:																		\
	Type m_algin_##Name;														\
	QVector<Type> m_vecin_##Name;												\
public:																			\
	void setIn##Name (Type value){												\
		this->m_algin_##Name = value;											\
		this->m_vecin_##Name << value;											\
	}																			\
	QVector<Type> getIn##Name () const{											\
		return this->m_vecin_##Name;											\
	}																			\
	QVector<Type>& getInRef##Name (){											\
		return std::ref(this->m_vecin_##Name);									\
	}																			\
	QVector<Type>&& getInMove##Name (){											\
		return std::move(this->m_vecin_##Name);									\
	}
#endif

/**
 * \brief Defines an output property for the algorithm.
 *
 * The purpose of this macro is to be used in subclasses of the QAlgorithm
 * class and allows to easily define an output to the algorithm.
 * This macro registers a property of type \e Type called \link QA_OUT\endlink\<\e Name\> in the Qt's
 * MetaObject System (calling Q_PROPERTY). It automatically defines a member attribute
 * for the subclass called m_\link QA_OUT\endlink\<\e Name\> using the keyword
 * MEMBER of Q_PROPERTY.\n
 * QA_OUTPUT generates setter and getter methods for the given property; the
 * name convention used is the same as in QA_INPUT.
 * 
 * \param[in] Type Type of the property; must be registered in the Qt's MetaObject System.
 * \param[in] Name Name of the property.
 * 
 * \sa QA_INPUT, QA_INPUT_LIST, QA_INPUT_VEC, QA_PARAMETER
 */
#ifndef QA_OUTPUT
#define QA_OUTPUT(Type, Name)																	\
Q_PROPERTY(Type algout_##Name MEMBER m_algout_##Name READ getOut##Name WRITE setOut##Name)		\
private:																						\
	Type m_algout_##Name;																		\
protected:																						\
	void setOut##Name (Type value){																\
		this->m_algout_##Name = value;															\
	}																							\
public:																							\
	Type getOut##Name () const{																	\
		return this->m_algout_##Name;															\
	}																							\
	Type& getOutRef##Name (){																	\
		return std::ref(this->m_algout_##Name);													\
	}																							\
	Type&& getOutMove##Name (){																	\
		return std::move(this->m_algout_##Name);												\
	}
#endif

#ifndef QA_PARAMETER
/**
 * \brief Defines a parameter of the algorithm.
 *
 * The purpose of this macro is to be used in subclasses of the QAlgorithm
 * class and allows to easily define a parameter to the algorithm.
 * This macro registers a property of type \e Type called \link QA_PAR\endlink\<\e Name\> in the Qt's
 * MetaObject System (calling Q_PROPERTY). It automatically defines a member attribute
 * for the subclass called \link QA_PAR\endlink\<\e Name\>.\n
 * QA_PARAMETER generates setter and getter methods for the given property; the
 * name convention used is:
 *  - setIn\<\e Name\> for the setter
 *  - getIn\<\e Name\> for the getter
 *
 * \param[in] Type Type of the property; must be registered in the Qt's MetaObject System.
 * \param[in] Name Name of the property.
 * \param[in] Default Value of type \e Type to be used as default for the parameter.
 * 
 * \sa QA_INPUT, QA_INPUT_LIST, QA_INPUT_VEC, QA_OUTPUT
 */
#define QA_PARAMETER(Type, Name, Default)													\
Q_PROPERTY(Type par_##Name READ get##Name WRITE set##Name)									\
private:																					\
	Type par_##Name = Default;																\
public:																						\
	void set##Name (Type value){															\
	this->par_##Name = value;																\
	}																						\
Type get##Name (){																			\
	return this->par_##Name;																\
}
#endif

/** 
 * \brief Make a subclass inherit QAlgorithm's default constructor.
 * 
 * This macro is to be used in each direct subclass of QAlgoritm to make it inherit
 * the default constructor and the QAlgorithm::setup method.\n
 * This is useful if no further customization is needed, and mandatory 
 * is no other constructor is defined.
 *
 * \sa QAlgorithm, setup
 */
#ifndef QA_CTOR_INHERIT
#define QA_CTOR_INHERIT 																	\
public:																						\
	using QAlgorithm::QAlgorithm;															\
protected:																					\
	using QAlgorithm::setup;
#endif


/** 
 * \brief Define the create static method in a subclass.
 * 
 * This macro is meant to be used in a subclass of QAlgorithm, but it's not mandatory.
 * It defines a predefined \e create static method, that allocates a new instance of that
 * subclass and returns a QSharedPointer to it. It also allows to directly pass the parameters
 * and the inputs on creation, saving several lines of code.
 * 
 * The create method is a wrapper around a set of operations:
 * - algorithm allocation on the heap
 * - call to QAlgorithm::setup() (subclasses can reimplement it)
 * - if a set of parameter name-value pairs are provided, they are passed to
 *		the algorithm using QAlgorithm::setParameters()
 * - call to QAlgorithm::init() (subclasses can reimplement it)
 *
 * \b Note: this macro cannot be used in abstract subclass.
 *
 * @param[in] ClassName Name of the subclass which inherit from QAlgorithm.
 */
#ifndef QA_IMPL_CREATE
#define QA_IMPL_CREATE(ClassName)																	\
public:																								\
	static inline QSharedPointer<ClassName> create(QAPropertyMap parameters = QAPropertyMap(),		\
													QObject* parent = Q_NULLPTR){					\
		auto ptr = QSharedPointer<ClassName>(new ClassName(parent), &QObject::deleteLater);			\
		ptr->setup();																				\
		if(!parameters.isEmpty()){																	\
			ptr->setParameters(parameters);															\
		}																							\
		ptr->init();																				\
		return ptr;																					\
	}
#endif

#endif
