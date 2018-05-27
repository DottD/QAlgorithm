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

/** \file QAlgorithm.h
 *  Declarations for the QAlgorithm class.
 */

#ifndef QAlgorithm_h
#define QAlgorithm_h

#include <QtCore>
#include <QtConcurrent/qtconcurrentrun.h>
#include "qa_macros.h"

class QAlgorithm;

typedef QSharedPointer<QAlgorithm> QAShrAlgorithm;
typedef QMap<QString, QVariant> QAPropertyMap;
typedef QMultiMap<QString, QString> QAPropagationRules;
typedef QMap<QAShrAlgorithm, bool> QACompletionMap;
typedef QMap<QAShrAlgorithm, QSet<QAShrAlgorithm>> QAFlatRepresentation;

Q_DECLARE_METATYPE(QAPropertyMap)
Q_DECLARE_METATYPE(QAPropagationRules)
Q_DECLARE_METATYPE(QAShrAlgorithm)

/** 
 * \brief Abstract class that implements a generic algorithm.
 * 
 * This is an abstract class that contains the generic logic
 * of an algorithm. You need to subclass it to use it and reimplement
 * the run() function, that will contain the core of your algorithm.
 * 
 * Some macros like QA_INPUT(), QA_OUTPUT(), QA_PARAMETER() come in handy
 * to define inputs, outputs and parameters of your algorithm.
 * 
 * Algorithms can be easily allocated using their \e create() static method,
 * provided that the macro \link QA_IMPL_CREATE\endlink is put in your
 * subclass definition.
 * 
 * If you have more algorithms that need to run in sequence, you can 
 * connect them using setConnection() and the friendly operators
 * operator<<(QAShrAlgorithm descendant, QAShrAlgorithm ancestor) and
 * operator>>(QAShrAlgorithm ancestor, QAShrAlgorithm descendant).
 * Connections no more needed can be closed using closeConnection().
 * 
 * Generally you should use the methods serialExecution() (using the same thread)
 * or parallelExecution() (using as many thread as possible) to run the whole
 * algorithm tree. The connections you established between algorithms
 * take care of passing outputs and parameters from parents to children;
 * this is controlled by the so called \e PropagationRules. This is basically
 * a map of strings to strings, where the name of parent's property is mapped
 * to its children's destination property. The rules are as follows:
 * - if no entry corresponds to parent's output property, then it is passed to a child
 * only if it has an input property with the same name;
 * - a parent's output property is passed to a child's input property with the name
 * given by the PropagationRules;
 * - a parent's parameter is passed to a child's input property or parameter only if
 * there is an entry in the PropagationRules, otherwise parameters are not shared.
 * The PropagationRules can be created using the static method makePropagationRules().
 * 
 * Every algorithm will have a boolean parameter called \e KeepInput. If this property 
 * is set to true the input is not freed until this instance is destroyed.
 * Otherwise the input values are set to QVariant() as soon as the
 * computation ends, and the connection with children is closed as soon as
 * properties have been passed to them.
 * 
 * Every algorithm has also a \e ParallelExecution property (not to be confused with
 * the method with the same name). This is a boolean value stating whether its
 * children will be run in a different thread or in the same one. Forcing serial
 * execution only for some connections can be useful if an algorithm's output
 * is huge and can be processed immediately by its children.
 * 
 * An algorithm may also have multiple parents; in this case it is good
 * for children algorithms to have a container to store all parents' outputs.
 * This can be achieved declaring the children's inputs with the macros
 * QA_INPUT_LIST() (for faster connections) or QA_INPUT_VEC() (for contiguous memory)
 * instead of QA_INPUT().
 */
class QAlgorithm : public QObject, public QRunnable 
{
	
	Q_OBJECT
	
	QA_PARAMETER(bool, KeepInput, false)
	QA_PARAMETER(QAPropagationRules, PropagationRules, QAPropagationRules())
	QA_PARAMETER(bool, ParallelExecution, true)
	
	Q_PROPERTY(bool finished READ isFinished NOTIFY justStarted)
	Q_PROPERTY(bool started READ isStarted NOTIFY justFinished)
	Q_PROPERTY(QACompletionMap ancestors READ getAncestors)
	Q_PROPERTY(QACompletionMap descendants READ getDescendants)
	
	/** 
	 * \brief Map with ancestors and their completion.
	 *
	 * Map that contains information about the ancestors of the algorithm;
	 * each key is a shared pointer to an ancestor of this algorithm,
	 * the values are booleans stating whether the algorithm is finished or not.
	 *
	 * \note This is a read-only property. You can have access to this property
	 *		value through the const getter getAncestors().
	 *
	 * You can set ancestors through the functions mentioned in the see-also section.
	 *
	 * \sa setConnection, closeConnection, operator<<, operator>>
	 */
	QACompletionMap ancestors;
	
	/** 
	 * \brief Map with descendants and their completion.
	 *
	 * Map that contains information about the descendants of the algorithm;
	 * see \link ancestors\endlink for a description of the mapped values.
	 *
	 * \note This is a read-only property. You can have access to this property
	 *		value through the const getter getDescendants().
	 *
	 * You can set descendants through the functions mentioned in the see-also section.
	 *
	 * \sa setConnection, closeConnection, operator<<, operator>>
	 */
	QACompletionMap descendants;
	
	/** 
	 * \brief Whether the algorithm finished to run and outputs are ready.
	 *
	 * The signal justFinished() is emitted whenever this property changes its
	 * value and the algorithm reach completion.
	 *
	 * \note This is a read-only property. You can have access to this property
	 *		value through the const getter isFinished().
	 *
	 * \sa setFinished, isFinished, started
	 */
	bool finished = false;
	
	/** 
	 * \brief Set the algorithm as comleted and signals it.
	 *
	 * This is actually the setter method for \link finished\endlink and
	 * emits justFinished().
	 *
	 * It is not meant to be directly used in the code; it is called
	 * by serialExecution() and parallelExecution(), use
	 * these functions instead.
	 *
	 * \sa finished, isFinished, started
	 */
	void setFinished();
	
	/** 
	 * \brief Whether the algorithm started to run.
	 *
	 * The signal justStarted() is emitted whenever this property changes its
	 * value and the algorithm reach completion.
	 *
	 * \note This is a read-only property. You can have access to this property
	 *		value through the const getter isStarted().
	 *
	 * \sa setStarted, isStarted, finished
	 */
	bool started = false;
	
	/** 
	 * \brief Set the algorithm as started and signals it.
	 *
	 * This is actually the setter method for \link started\endlink and
	 * emits justStarted().
	 *
	 * It is not meant to be directly used in the code; it is called
	 * by serialExecution() and parallelExecution(), use
	 * these functions instead.
	 *
	 * \sa started, isStarted, finished
	 */
	void setStarted();
	
	static quint32 print_counter;
	
	QFuture<void> result;
	QFutureWatcher<void> watcher;
	
protected:
	
	/** 
	 * \brief Set of instructions to set up the algorithm.
	 * 
	 * Any subclass that needs to preallocate variables or configure
	 * anything should do that reimplementing this function.
	 *
	 * \note This is automatically called by the \e create() function,
	 * before setting any parameter; if you want to manually allocate
	 * a subclass instance, you should call this function by yourself.
	 *
	 * \note If you need to set things up \b after the algorithm parameters
	 * has been given, you can reimplement the function init().
	 *
	 * \sa init, setParameters, QA_IMPL_CREATE
	 */
	virtual void setup();
	
	/** 
	 * \brief Set of instructions to initialize the algorithm.
	 * 
	 * Any subclass that needs to preallocate variables or configure
	 * anything should do that reimplementing this function.
	 *
	 * \note This is automatically called by the \e create() function,
	 * after setting any parameter; if you want to manually allocate
	 * a subclass instance, you should call this function by yourself.
	 *
	 * \note If you need to set things up \b before the algorithm parameters
	 * has been given, you can reimplement the function setup().
	 *
	 * \sa setup, setParameters, QA_IMPL_CREATE
	 */
	virtual void init(){};
	
	/** 
	 * \brief Find an ancestor.
	 *
	 * Scans the ancestors of this algorithm looking for the given ancestor.
	 * If nothing is found returns a null shared pointer.
	 * 
	 * \param[in] ancestor Pointer to the ancestor to be found.
	 *
	 * \return Shared pointer to the requested ancestor, if any, or a null shared
	 * pointer otherwise.
	 *
	 * \sa findAncestor(QAShrAlgorithm), findDescendant, findSharedThis
	 */
	QAShrAlgorithm findAncestor(const QAlgorithm* ancestor) const;
	
	/** 
	 * \brief Find an ancestor.
	 *
	 * This is an overloaded method of findAncestor(QAlgorithm*).
	 *
	 * \param[in] ancestor Shared pointer to the ancestor to be found.
	 * \return Shared pointer to the requested ancestor, if any, or a null shared
	 * pointer otherwise.
	 *
	 * \sa findAncestor(QAlgorithm*), findDescendant, findSharedThis
	 */
	QAShrAlgorithm findAncestor(const QAShrAlgorithm ancestor) const;
	
	/** 
	 * \brief Find an descendant.
	 *
	 * Scans the descendants of this algorithm looking for the given descendant.
	 * If nothing is found returns a null shared pointer.
	 * 
	 * \param[in] descendant Pointer to the descendant to be found.
	 *
	 * \return Shared pointer to the requested descendant, if any, or a null shared
	 * pointer otherwise.
	 *
	 * \sa findAncestor, findDescendant(QAShrAlgorithm), findSharedThis
	 */
	QAShrAlgorithm findDescendant(const QAlgorithm* descendant) const;
	
	/** 
	 * \brief Find an descendant.
	 *
	 * This is an overloaded method of findAncestor(QAlgorithm*).
	 *
	 * \param[in] descendant Shared pointer to the descendant to be found.
	 * \return Shared pointer to the requested descendant, if any, or a null shared
	 * pointer otherwise.
	 *
	 * \sa findAncestor, findDescendant(QAlgorithm*), findSharedThis
	 */
	QAShrAlgorithm findDescendant(const QAShrAlgorithm descendant) const;
	
	/** 
	 * \brief Find a shared pointer to this instance.
	 *
	 * Scans the descendants and ancestors of this algorithm looking
	 * for a shared pointer to this instance. The first pointer found
	 * is returned, if any, otherwise a null shared pointer is returned.
	 *
	 * No shared pointer is created by this function.
	 *
	 * \return Shared pointer to this instance, if any, or a null shared
	 * pointer otherwise.
	 *
	 * \sa findAncestor, findDescendant
	 */
	QAShrAlgorithm findSharedThis() const;
	
public:
	/**
	 * \brief Constructor.
	 * 
	 * Simply calls inherited classes' constructors and registers meta types
	 * used in the QAlgorithm class.
	 */
	QAlgorithm(QObject* parent = Q_NULLPTR);
	
	/**
	 * \brief Core part of the algorithm, to be reimplemented in subclasses.
	 * 
	 * For more information see class QAlgorithm description.
	 */
	virtual void run() = 0;
	
	/** 
	 * \brief Get the value of ancestors.
	 *
	 * \return The value of this algorithm's \link ancestors\endlink.
	 *
	 * \sa ancestors, descendants
	 */
	QACompletionMap getAncestors() const;
	
	/** 
	 * \brief Get the value of descendants.
	 *
	 * \return The value of this algorithm's \link descendants\endlink.
	 *
	 * \sa ancestors, descendants
	 */
	QACompletionMap getDescendants() const;
	
	/**
	 * \brief Checks if the algorithm is ready to run.
	 * 
	 * This function scan the \link ancestors\endlink and returns 
	 * whether they all have finished their computations.
	 * 
	 * \return Whether every ancestor finished running.
	 */
	bool allInputsReady() const;
	
	/**
	 * \brief Get the value of \link started\endlink.
	 *
	 * \return Whether the algorithm started running.
	 *
	 * \sa started, finished, isFinished
	 */
	bool isStarted() const;

	/**
	 * \brief Get the value of \link finished\endlink.
	 *
	 * \return Whether the algorithm finished running.
	 *
	 * \sa started, finished, isStarted
	 */
	bool isFinished() const;
	
	/**
	 * \brief Load inputs from parent's outputs.
	 *
	 * This function assigns any <em>parent</em>'s output or parameter 
	 * properties to this instance's input or parameter properties sharing
	 * the same name. In case you want to connect a <em>parent</em>'s
	 * property to a property with different name, you can use the
	 * PropagationRules. Taking parameters from parent is allowed
	 * only if the <em>parent</em>'s parameter name is explicitly mentioned
	 * among the PropagationRules.
	 *
	 * \return Whether inputs have been loaded successfully.
	 *
	 * \sa makePropagationRules
	 */
	virtual bool getInput(QAShrAlgorithm parent);
	
	/**
	 * \brief Set parameters for the algorithm.
	 *
	 * The given name-value parameter pairs are assigned to parameters
	 * or input properties.
	 *
	 * \note This is automatically called by the \e create() function,
	 * after calling \e setup() but before \e init(); if you want to manually allocate
	 * a subclass instance, you should call this function by yourself.
	 *
	 * \param[in] parameters Name-value parameter/input pairs.
	 *
	 * \sa setup, init, QA_IMPL_CREATE
	 */
	virtual void setParameters(const QAPropertyMap& parameters);
	
	/**
	 * \brief Create a GraphViz diagram of the algorithm tree.
	 * 
	 * Mostly useful for debugging purposes, this function the graphviz executable
	 * as an external process to generate the visual representation of
	 * the algorithm tree.
	 * 
	 * \note This feature is still experimental. It has not been tested and
	 * lacks generality.
	 * 
	 * \param[in] path Path to the output file.
	 */
	void printGraph(const QString& path = QString()) const;
	
	/**
	 * \brief Returns name, memory address and class name of the algorithm.
	 * 
	 * This function is mostly useful for debugging purposes.
	 * 
	 * \return A string composed by memory address, class name and object name
	 * of the algotithm.
	 */
	QString printName() const;
	
	/**
	 * \brief Creates a flat representation of the algorithm tree.
	 * 
	 * A flat representation of the algorithm is a map with:
	 * - \b key, an algorithm shared pointer (father)
	 * - \b values, the set of shared pointer algorithms that are direct children of \b key
	 * 
	 * flattenTree() method can be called with no arguments on any node of an algorithm
	 * tree to generate such representation.
	 * 
	 * When flattenTree() is called with a flat representation \e tree as argument, then
	 * it appends the new representation to the given one. This behaviour is internally used
	 * by the flattenTree() method and is probably of no use for the common user.
	 * 
	 * \param[in] tree Optional representation to include in the output.
	 * \return The flat representation of the algorithm tree structure.
	 * 
	 * \sa printTree, printGraph, improveTree
	 */
	QAFlatRepresentation flattenTree(QAFlatRepresentation tree = QAFlatRepresentation()) const;
	
	/**
	 * \brief Outputs a text representation of the algorithm tree.
	 * 
	 * Actually sends to the standard output the flat representation of the given
	 * tree, printed as such in a key-value structure.
	 * 
	 * \param[in] tree The tree to be visualized; if empty the function will use the output
	 * of flattenTree() called on this instance.
	 *
	 * \sa flattenTree
	 */
	void printTree(const QAFlatRepresentation& tree = QAFlatRepresentation()) const;
	
	/**
	 * \brief Connect two algorithms.
	 * 
	 * Modifies the completion maps of both algorithms and reciprocally connect them
	 * through the raise() signal, for error propagation throughout the whole algorithm
	 * tree.
	 * 
	 * \param[in] ancestor 	Shared pointer to an algorithm, that you want to 
	 						connect to its child \e descendant.
	 * \param[in] descendant 	Shared pointer to an algorithm, that you want to 
	 							connect to its parent \e ancestor.
	 * \sa closeConnection, checkConnection
	 */
	static void setConnection(QAShrAlgorithm ancestor, QAShrAlgorithm descendant);
	
	/**
	 * \brief Disconnect two algorithms.
	 * 
	 * This function does the opposite of setConnection().
	 * 
	 * \param[in] ancestor 	Shared pointer to an algorithm, that you want to 
	 						disconnect from its child \e descendant.
	 * \param[in] descendant 	Shared pointer to an algorithm, that you want to 
	 							disconnect from its parent \e ancestor.
	 * \sa setConnection, checkConnection
	 */
	static void closeConnection(QAShrAlgorithm ancestor, QAShrAlgorithm descendant);
	
	/**
	\brief Check if two algorithms are connected.

	This function simply checks if the given algorithms have previously
	been connected, for example using setConnection(), operator<<() or operator>>().

	\param[in] ancestor Shared pointer to an algorithm, that you want to check whether it is
						parent to \e descendant.
	\param[in] descendant 	Shared pointer to an algorithm, that you want to check whether it is
							child to \e ancestor.
	\return Whether \e ancestor is the parent of \e descendant.
	\sa setConnection, closeConnection
	*/
	static bool checkConnection(const QAShrAlgorithm ancestor, const QAShrAlgorithm descendant);
	
	/**
	 * \brief Check if two algorithms are connected and the connection is removable.
	 * 
	 * This function checks if two algorithms are connected using checkConnection(); 
	 * if so, it checks also if it is a <em>removable connection</em>, that is:
	 * a connection where the parent has only one child, and the child has only
	 * one parent. It is called removable because parent and child can be put
	 * together in a single algorithm, without changing the overall behavior.
	 * 
	 * \param[in] ancestor Shared pointer to an algorithm, that you want to check whether it is
	 * 					parent to \e descendant.
	 * \param[in] descendant 	Shared pointer to an algorithm, that you want to check whether it is
	 * 						child to \e ancestor.
	 * \return Whether there is a removable connection between \e ancestor and \e descendant.
	 * \sa improveTree
	 */
	static bool isRemovableConnection(const QAShrAlgorithm p1, const QAShrAlgorithm p2);
	
	/**
	 * \brief Replace all removable connections, thus improving the tree performance.
	 * 
	 * It actually force any pair of removably-connected algorithms to run in the same thread.
	 * If KeepInput is set to false for the parent, and properties are \e moved insted of \e copied
	 * to the child instance, the improvement is high, since there is almost no waste in memory
	 * and time. Indeed the parent instance will be deleted as soon as it becomes useless, and its
	 * properties are directly exploited by the child instance without deep copying them;
	 * furthermore time consumption is limited by running on the same thread.
	 * 
	 * \note This function is still experimental, since it's not been much tested.
	 * 
	 * \param[in] leaf Pointer to an algorithm belonging to the tree to improve.
	 * \sa isRemovableConnection
	 */
	static void improveTree(QAlgorithm* leaf);
	
	/**
	 * \brief Convenience method for writing \e PropagationRules.
	 *
	 * \param[in] lst Something like {{"Out1","In1"},{"Out2","In2"}}
	 *
	 * \return A pair that can be used to construct a QAPropertyMap from
	 * an initializer list.
	 */
	static std::pair<QString, QVariant> makePropagationRules(std::initializer_list<std::pair<QString,QString>> lst);
	
	public Q_SLOTS:
	
	/**
	 * \brief Start computing the algorithm tree on different threads.
	 * 
	 * This function makes the entire algorithm tree run on threads
	 * other than the one who calls it. No matter which level
	 * the function is called on, the whole algorithm tree will be
	 * computed.
	 * 
	 * \note The calling function will \b NOT freeze waiting for completion.
	 * 
	 * \sa serialExecution()
	 */
	Q_SLOT void parallelExecution();

	/**
	 * \brief Start computing the algorithm tree on the calling thread.
	 * 
	 * This function makes the entire algorithm tree run on the same
	 * thread that calls it. No matter which level
	 * the function is called on, the whole algorithm tree will be
	 * computed.
	 * 
	 * \note The calling function will freeze waiting for completion.
	 * 
	 * \sa parallelExecution()
	 */
	Q_SLOT void serialExecution();
	
	/** 
	 * \brief Emit the given error signal.
	 * 
	 * Typical use case for this function is to propagate an error
	 * from connected algorithms. Indeed it is, by default, connected
	 * to their raise() signal.
	 * 
	 * \sa raise, setConnection
	 */
	Q_SLOT void abort(QString message = "Unknown Error") const;
	
	/**
	 * \brief Execute descendants.
	 * 
	 * This functions changes values to \link ancestors\endlink and
	 * \link descendants\endlink, making connected algorithms know that
	 * this instance finished running. Then passes outputs and parameters
	 * to every descendant.
	 * 
	 * If \e KeepInput parameter is set to false, the inputs are invalidated
	 * and the connection with descendants is closed. This will deallocate
	 * this object as soon as it ends its computation and sends the results.
	 * 
	 * Finally, serialExecution() or parallelExecution() is called on each
	 * descendant according to the value of \e ParallelExecution.
	 * 
	 * \note Generally there is no need for the users to directly call this
	 * function. It is by default connected to the justFinished() signal.
	 * 
	 * \sa serialExecution, parallelExecution
	 */
	Q_SLOT void propagateExecution();
	
Q_SIGNALS:
	/**
	 * \brief Signal emitted on algorithm's end.
	 * 
	 * \note By default this signal is connected to the slot propagateExecution(),
	 * to make descendants run as this algorithm ends.
	 * 
	 * \sa propagateExecution, finished
	 */
	Q_SIGNAL void justFinished();

	/**
	 * \brief Signal emitted on algorithm's start.
	 * 
	 * \sa started
	 */
	Q_SIGNAL void justStarted();
	
	/** 
	 * \brief Signal emitted whenever an error occurs.
	 * 
	 * This class emits the raise signal whenever an internal error
	 * occurs. This behaviour replaces the traditional exception throwing,
	 * allowing the user to easily handle errors in Qt event loop.
	 * Indeed each subclass should use the same convention, and every caller
	 * should handle and possibly retransmit the raise signal.
	 * 
	 * \param[in] message The error description.
	 */
	Q_SIGNAL void raise(QString message) const;
};

/** \relates QAlgorithm
 * \brief Creates connections like \link setConnection()\endlink. 
 */
QAShrAlgorithm operator>>(QAShrAlgorithm ancestor, QAShrAlgorithm descendant);

/** \relates QAlgorithm
 * \brief Creates connections like \link setConnection()\endlink. 
 */
QAShrAlgorithm operator<<(QAShrAlgorithm descendant, QAShrAlgorithm ancestor);

/** \relates QAlgorithm
 * \brief Send debugging information to a QDebug instance.
 */
QDebug operator<<(QDebug debug, const QAlgorithm& c);

/** \relates QAlgorithm
 * \brief Store a QAlgorithm into a QDataStream.
 * 
 * This function is useful to quickly save an algorithm instance to file.
 * It internally saves all the input and output properties, and all the
 * parameters. The saved information can be retrieved by the converse
 * operator>>(QDataStream&, QAlgorithm&).
 * 
 * The behaviour can be customized by subclasses.
 *
 * \sa operator>>(QDataStream&, QAlgorithm&)
 */
QDataStream& operator<<(QDataStream& stream, const QAlgorithm& c);

/** \relates QAlgorithm
 * \brief Loads a QAlgorithm from a QDataStream.
 * 
 * This function is useful to quickly load an algorithm instance from file.
 * 
 * The behaviour can be customized by subclasses.
 *
 * \sa operator<<(QDataStream&, const QAlgorithm&)
 */
QDataStream& operator>>(QDataStream& stream, QAlgorithm& c);

#endif /* QAlgorithm_h */
