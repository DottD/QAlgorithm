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

#include "QAlgorithm.h"

quint32 QAlgorithm::print_counter = 1;

bool QAlgorithm::isFinished() const
{
	return finished;
}

bool QAlgorithm::isStarted() const
{
	return started;
}

void QAlgorithm::setStarted()
{
	started = true;
	Q_EMIT justStarted();
}

void QAlgorithm::setFinished()
{
	finished = true;
	Q_EMIT justFinished();
}

bool QAlgorithm::allInputsReady() const
{
	return !getAncestors().values().contains(false);
}

QACompletionMap QAlgorithm::getAncestors() const
{
	return ancestors;
}

QACompletionMap QAlgorithm::getDescendants() const
{
	return descendants;
}

QAShrAlgorithm QAlgorithm::findAncestor(const QAlgorithm* ancestor) const
{
	foreach(auto& shr_ancestor, getAncestors().keys())
	{
		if (shr_ancestor == ancestor)
		{
			return shr_ancestor;
		}
	}
	return QAShrAlgorithm();
}

QAShrAlgorithm QAlgorithm::findAncestor(const QAShrAlgorithm ancestor) const
{
	return findAncestor(ancestor.data());
}

QAShrAlgorithm QAlgorithm::findDescendant(const QAlgorithm* descendant) const
{
	foreach(auto& shr_descendant, getDescendants().keys())
	{
		if (shr_descendant == descendant)
		{
			return shr_descendant;
		}
	}
	return QAShrAlgorithm();
}

QAShrAlgorithm QAlgorithm::findDescendant(const QAShrAlgorithm descendant) const
{
	return findDescendant(descendant.data());
}

QAShrAlgorithm QAlgorithm::findSharedThis() const
{
	// Check among the descendants
	foreach(auto descendant, getDescendants().keys())
	{
		auto shr_this = descendant->findAncestor(this);
		if(!shr_this.isNull()) return shr_this;
	}
	// Otherwise check among the ancestors
	foreach(auto ancestor, getAncestors().keys())
	{
		auto shr_this = ancestor->findDescendant(this);
		if(!shr_this.isNull()) return shr_this;
	}
	// If nothing was found return null shared pointer
	return QAShrAlgorithm();
}

QAlgorithm::QAlgorithm(QObject* parent) : QObject(parent), QRunnable()
{
	qRegisterMetaType<QAPropertyMap>();
	qRegisterMetaType<QAPropagationRules>();
	qRegisterMetaType<QAShrAlgorithm>();
}

void QAlgorithm::setParameters(const QAPropertyMap& parameters)
{
	// Scan the object properties
	for(const QString& PropName: parameters.keys())
	{
		bool set = false;
		for(int k = 0; k < metaObject()->propertyCount(); ++k)
		{
			if(metaObject()->property(k).name() == QA_PAR + PropName ||
			   metaObject()->property(k).name() == QA_IN + PropName)
			{
				// This property is a parameter or an input
				// Write the desired value in the property
				if (!setProperty(metaObject()->property(k).name(), parameters.value(PropName)))
				{
					qWarning() << "Cannot set parameter/input" << PropName;
				}
				set = true;
			}
		}
		if(!set) qWarning() << "Trying to set" << PropName << "but it is not among object's properties";
	}
}

void QAlgorithm::setup()
{
	// Prevent the QThreadPool to delete a parent instance
	setAutoDelete(false);
	// Make internal connections
	connect(this, &QAlgorithm::justFinished, this, &QAlgorithm::propagateExecution, Qt::AutoConnection);
	connect(&watcher, &QFutureWatcher<void>::finished, this, &QAlgorithm::setFinished, Qt::AutoConnection);
}

void QAlgorithm::propagateExecution()
{
	auto shr_this = findSharedThis();
	if(!shr_this.isNull())
	{
		// Notify ancestors
		foreach(auto ancestor, getAncestors().keys()) ancestor->descendants[shr_this] = true;
		// Notify descendants, transfer output to and execute them
		foreach(auto descendant, getDescendants().keys())
		{
			descendant->ancestors[shr_this] = true;
			descendant->getInput(shr_this);
			if(!descendant->getKeepInput())
			{
				QAlgorithm::closeConnection(shr_this, descendant);
				// Set each input property to null
				// useful if input has been received with implicit sharing
				for(int k = 0; k < metaObject()->propertyCount(); ++k)
				{
					if(QString(metaObject()->property(k).name()).startsWith(QA_IN))
					{
						setProperty(metaObject()->property(k).name(), QVariant());
					}
				}
			}
			if(!descendant->isStarted())
			{
				if (getParallelExecution()) descendant->parallelExecution();
				else descendant->serialExecution();
			}
		}
	}
}

bool QAlgorithm::getInput(QAShrAlgorithm parent)
{
	// Create an alias to this for better readability
	auto child = this;
	// Scan parent's properties and grab all the possible outputs and parameters
	for(int k = 0; k < parent->metaObject()->propertyCount(); ++k)
	{
		QString parentPropName = parent->metaObject()->property(k).name();
		// Get the parent's property base name
		// Both output and parameter properties are checked
		QString parentPropBaseName;
		if(parentPropName.startsWith(QA_OUT))
		{
			// Output property
			parentPropBaseName = parentPropName.mid(strlen(QA_OUT));
		}
		else if(parentPropName.startsWith(QA_PAR))
		{
			// Parameter
			parentPropBaseName = parentPropName.mid(strlen(QA_PAR));
		}
		else continue;
		// Get the child's property base name, based on PropagationRules values
		QString childPropBaseName;
		if(!getPropagationRules().contains(parentPropBaseName))
		{
			childPropBaseName = parentPropBaseName;
		}
		else
		{
			// The values associated with the given key
			auto values = getPropagationRules().values(parentPropBaseName);
			if(values.size() > 1)
			{
				// If there are more than one value, then use the first one that contain the parent object name
				childPropBaseName = values.filter(parent->objectName()).first();
			}
			else
			{
				childPropBaseName = values.first();
			}
		}
		// Parameters are sent only if they are explicitly mentioned in the PropagationRules
		if(parentPropName.startsWith(QA_PAR) && !getPropagationRules().contains(parentPropBaseName))
		{
			continue;
		}
		// Check if the property is in child's input or parameter properties
		for(int i = 0; i < child->metaObject()->propertyCount(); ++i)
		{
			QString childPropName = child->metaObject()->property(i).name();
			// If the current property corresponds to an input with the same base name as before,
			// then assign it; the same will be done also for parameters.
			if(childPropName == QA_IN+childPropBaseName || childPropName == QA_PAR+childPropBaseName)
			{
				QVariant parentProp = parent->property(parentPropName.toStdString().c_str());
				if(parentProp.isValid())
				{
					if(!child->setProperty(childPropName.toStdString().c_str(), parentProp))
					{
						qWarning() << "getInput():" << childPropName << "failed to set for" << child->printName();
						return false;
					}
				}
				else
				{
					qWarning() << "getInput():" << parentPropName << "failed to read for" << parent->printName();
					return false;
				}
			}
		}
	}
	return true;
}

void QAlgorithm::parallelExecution()
{
	// Check if every ancestor has finished
	if(allInputsReady())
	{
		// Perform the core part of the algorithm is a separate thread
		setStarted();
		result = QtConcurrent::run(this, &QAlgorithm::run);
		watcher.setFuture(result);
	}
	else
	{
		// Run those not started
		for(const auto& ancestor: getAncestors().keys(false))
		{
			// Only start processes not already started
			if(!ancestor->isStarted()) ancestor->parallelExecution();
		}
	}
}

void QAlgorithm::serialExecution()
{
	// Check if every ancestor has finished
	if(!allInputsReady())
	{
		// Run those not started
		for(const auto& ancestor: getAncestors().keys(false))
		{
			// Only start processes not already started
			if(!ancestor->isStarted()) ancestor->serialExecution();
		}
	}
	// Set the ParallelExecution policy to false
	setParallelExecution(false);
	// Perform the core part of the algorithm in the same thread
	setStarted();
	run();
	setFinished();
}

void QAlgorithm::abort(QString message) const
{
	Q_EMIT raise(message);
}

void QAlgorithm::printGraph(const QString &path) const
{
	QFile dotFile;
	QString outFileName;
	if (path.isEmpty())
	{
		dotFile.setFileName(QDir::home().absoluteFilePath("QAlgorithmTree.gv"));
		outFileName = QDir::home().absoluteFilePath("QAlgorithmTree.svg");
	}
	else
	{
		dotFile.setFileName(path);
		QStringList svgpath = path.split(".");
		svgpath.removeLast();
		outFileName = svgpath.join(".")+".svg";
	}
	if (!dotFile.open(QFile::WriteOnly)) Q_EMIT raise("Cannot write graph to given file");
	else
	{
		QTextStream dot(&dotFile);
		QLocale nospace;
		nospace.setNumberOptions(QLocale::NumberOption::OmitGroupSeparator);
		dot << "digraph g{\n";
		auto flatMap = flattenTree();
		// Save node labels
		foreach(auto alg, flatMap.keys())
		{
			QString id = nospace.toString(quint64(alg.data()));
			QString idspace = QLocale().toString(quint64(alg.data()));
			QString name = alg->metaObject()->className();
			QString nickname = alg->objectName();
			QString dotstring = "var"+id.replace(" ", "")+"[label=\""+name+"\\nID "+idspace;
			if(!nickname.isEmpty()) dotstring += "\\nNick: "+nickname;
			dotstring += "\"];\n";
			dot << dotstring;
		}
		// Save connections between nodes
		foreach(auto parent, flatMap.keys())
		{
			QString parentName = "var" + nospace.toString(quint64(parent.data()));
			foreach(auto child, flatMap[parent])
			{
				QString childName = "var" + nospace.toString(quint64(child.data()));
				dot << parentName << " -> " << childName << "\n";
			}
		}
		dot << "}\n";
		dotFile.close();
		// Convert dot file to svg
		QStringList cmdArgs =
		{dotFile.fileName(), "-Tsvg", "-o", outFileName}
		;
		int r = QProcess::execute("/usr/local/bin/circo", cmdArgs);
		if (r == -1)
		{
			qWarning("%s", "The dot process crashed");
		}
		else if (r == -2)
		{
			qWarning("%s", "Cannot start the dot process");
		}
		else
		{
			r = QProcess::execute("/bin/rm",
								  {dotFile.fileName()}
								  );
			if (r == -1)
			{
				qWarning("%s", "The rm process crashed");
			}
			else if (r == -2)
			{
				qWarning("%s", "Cannot start the rm process");
			}
		}
	}
}

std::pair<QString, QVariant> QAlgorithm::makePropagationRules(std::initializer_list<std::pair<QString,QString>> pairs)
{
	return std::pair<QString, QVariant>({"PropagationRules", QVariant::fromValue(QAPropagationRules(pairs))});
}

QString QAlgorithm::printName() const
{
	QString msg;
	msg += QString(metaObject()->className()) + " ";
	msg += QLocale().toString(qlonglong(this)) + " ";
	if (!objectName().isEmpty()) msg += objectName();
	return msg;
}

QAFlatRepresentation QAlgorithm::flattenTree(QAFlatRepresentation tree) const
{
	// Search for a shared pointer attached to this instance
	auto keysList = tree.keys();
	// Check if the function has already been applied to this
	auto shr_this_it = std::find_if(keysList.begin(), keysList.end(), [this](auto& ptr)
									{return ptr == this;}
									);
	if (shr_this_it == keysList.end())
	{
		// Otherwise link each descendant to this in the map
		auto shr_this = findSharedThis();
		if(shr_this.isNull()) qWarning() << "This instance has no properly set connection, flattenTree will not work";
		else
		{
			// Insert shr_this as new key in the map
			tree[shr_this] = QSet<QAShrAlgorithm>();
			// Append each descendant to it
			for (auto descendant: getDescendants().keys()) tree[shr_this] << descendant;
			// Recursive step: apply the function to each relative not yet scanned
			for (auto relative: getDescendants().keys()+getAncestors().keys())
			{
				if (!tree.contains(relative)) tree = relative->flattenTree(tree);
			}
		}
	}
	else qWarning() << "flattenTree: possible loop";
	return tree;
}

void QAlgorithm::setConnection(QAShrAlgorithm ancestor, QAShrAlgorithm descendant)
{
	ancestor->descendants[descendant] = descendant->isFinished();
	descendant->ancestors[ancestor] = ancestor->isFinished();
	connect(ancestor.data(), &QAlgorithm::raise, descendant.data(), &QAlgorithm::abort, Qt::QueuedConnection);
	connect(descendant.data(), &QAlgorithm::raise, ancestor.data(), &QAlgorithm::abort, Qt::QueuedConnection);
}

void QAlgorithm::closeConnection(QAShrAlgorithm ancestor, QAShrAlgorithm descendant)
{
	ancestor->descendants.remove(descendant);
	descendant->ancestors.remove(ancestor);
	disconnect(ancestor.data(), &QAlgorithm::raise, descendant.data(), &QAlgorithm::abort);
	disconnect(descendant.data(), &QAlgorithm::raise, ancestor.data(), &QAlgorithm::abort);
}

bool QAlgorithm::checkConnection(QAShrAlgorithm ancestor, QAShrAlgorithm descendant)
{
	return ancestor->getDescendants().contains(descendant) && descendant->getAncestors().contains(ancestor);
}

bool QAlgorithm::isRemovableConnection(QAShrAlgorithm p1, QAShrAlgorithm p2)
{
	if (QAlgorithm::checkConnection(p2, p1))
	{
		return (p2->getDescendants().count() == 1) && (p1->getAncestors().count() == 1);
	}
	else if (QAlgorithm::checkConnection(p1, p2))
	{
		return (p1->getDescendants().count() == 1) && (p2->getAncestors().count() == 1);
	}
	else return false;
}

QAShrAlgorithm operator>>(QAShrAlgorithm ancestor, QAShrAlgorithm descendant)
{
	QAlgorithm::setConnection(ancestor, descendant);
	return descendant;
}

QAShrAlgorithm operator<<(QAShrAlgorithm descendant, QAShrAlgorithm ancestor)
{
	ancestor >> descendant;
	return ancestor;
}

QDebug operator<<(QDebug debug, const QAlgorithm& c)
{
	QDebugStateSaver saver(debug);
	const QMetaObject* obj = c.metaObject();
	
	// Write the algorithm class name
	debug << endl << "------------------------------" << c.printName() << "subclass of QAlgorithm" << endl;
	
	// Write the input properties of the algorithm
	debug << "Algorithm with input:" << endl;
	for(int k = 0; k < obj->propertyCount(); k++)
	{
		QMetaProperty prop = obj->property(k);
		// Check whether the current property's name starts with "algin_"
		QString propName = prop.name();
		if(propName.startsWith(QA_IN))
		{
			propName.remove(QA_IN);
			debug << propName.rightJustified(30,' ',true) << "\t" << prop.read(&c) << endl;
		}
	}
	
	// Write the parameters of the algorithm
	debug << "Algorithm with parameters:" << endl;
	for(int k = 0; k < obj->propertyCount(); k++)
	{
		QMetaProperty prop = obj->property(k);
		// Check whether the current property's name starts with "algin_"
		QString propName = prop.name();
		if(propName.startsWith(QA_PAR))
		{
			propName.remove(QA_PAR);
			debug << propName.rightJustified(30,' ',true) << "\t" << prop.read(&c) << endl;
		}
	}
	
	// Write the output properties of the algorithm
	debug << "Algorithm with output:" << endl;
	for(int k = 0; k < obj->propertyCount(); k++)
	{
		QMetaProperty prop = obj->property(k);
		// Check whether the current property's name starts with "algout_"
		QString propName = prop.name();
		if(propName.startsWith(QA_OUT))
		{
			propName = propName.remove(QA_OUT);
			debug << propName.rightJustified(30,' ',true) << "\t" << prop.read(&c) << endl;
		}
	}
	
	debug << "------------------------------" << endl;
	
	return debug;
}

void QAlgorithm::printTree(const QAFlatRepresentation& tree) const
{
	auto _printTree = [](decltype(tree) map)
	{
		foreach(auto key, map.keys())
		{
			qInfo() << "key" << key->printName();
			foreach(auto value, map[key])
			{
				qInfo() << "\tvalue" << value->printName();
			}
		}
	}
	;
	if (tree.isEmpty()) _printTree(flattenTree());
	else _printTree(tree);
}

void QAlgorithm::improveTree(QAlgorithm* leaf)
{
	auto flatMap = leaf->flattenTree();
	QMap<QAShrAlgorithm, QList<QAShrAlgorithm>> replacements;
	foreach (auto ptr, flatMap.uniqueKeys())
	{
		foreach(auto child, flatMap[ptr])
		{
			if (QAlgorithm::isRemovableConnection(ptr, child))
			{
				// Insert a new replacement (assuming uniqueness of flat map keys)
				replacements[ptr] << child;
			}
		}
	}
	// From the set of removable connection, link the pairs that form a removable connection all together
	bool some_changes = true;
	while(some_changes)
	{
		some_changes = false;
		foreach(auto p1, replacements.keys())
		{
			// Take the last element in the list of removable connections
			auto p2 = replacements[p1].last();
			// If it has its own removable connections, add them to p1's
			if (replacements.contains(p2))
			{
				replacements[p1] += replacements.take(p2);
				some_changes = true;
				break; // recompute keys after any change
			}
		}
	}
	// Create the envelopes for each replacement
	foreach(QList<QAShrAlgorithm> nodes, replacements.values())
	{
		// Tell each node, except the last, to serially execute its children
		nodes.removeLast();
		foreach(auto node, nodes)
		{
			node->setParallelExecution(false);
		}
	}
}

QDataStream& operator<<(QDataStream& stream, const QAlgorithm& c)
{
	QAPropertyMap properties;
	for(int k = 0; k < c.metaObject()->propertyCount(); k++)
	{
		QMetaProperty prop = c.metaObject()->property(k);
		QString propName = prop.name();
		QVariant propValue = prop.read(&c);
		if(propValue.isValid() &&
		   (propName.startsWith(QA_IN) || propName.startsWith(QA_OUT) || propName.startsWith(QA_PAR)))
			properties.insert(propName, propValue);
	}
	return (stream << properties);
}

QDataStream& operator>>(QDataStream& stream, QAlgorithm& c)
{
	QAPropertyMap properties;
	stream >> properties;
	for(int k = 0; k < c.metaObject()->propertyCount(); k++)
	{
		QMetaProperty prop = c.metaObject()->property(k);
		QString propName = prop.name();
		if(properties.contains(propName))
		{
			if (!prop.write(&c, properties.values(propName)))
			{
				qWarning() << c.printName() << "Unable to write property value, report to the QAlgorithm developer";
			}
		}
	}
	return stream;
}
