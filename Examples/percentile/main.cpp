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

#include <QtCore>
#include <QDebug>
#include <functional>
#include <QAlgorithm.h>

class RandomGenerator: public QAlgorithm
{
	Q_OBJECT
	
	QA_PARAMETER(int, Amount, 10)
	QA_OUTPUT(QVector<double>, Numbers)
	
	QA_IMPL_CREATE(RandomGenerator)
	QA_CTOR_INHERIT
	
public:
	void run();
};

class MovingAverage: public QAlgorithm
{
	Q_OBJECT
	
	QA_INPUT(QVector<double>, Array)
	QA_PARAMETER(int, Size, 3)
	QA_OUTPUT(QVector<double>, Array)
	
	QA_IMPL_CREATE(MovingAverage)
	QA_CTOR_INHERIT
	
public:
	void run();
};

class Percentile: public QAlgorithm
{
	Q_OBJECT
	
	QA_INPUT(QVector<double>, Array)
	QA_PARAMETER(int, Order, 50)
	QA_OUTPUT(double, Percentile)
	
	QA_IMPL_CREATE(Percentile)
	QA_CTOR_INHERIT
	
public:
	void run();
};

class ElementPicker: public QAlgorithm
{
	Q_OBJECT
	
	QA_INPUT(QVector<double>, Array)
	QA_PARAMETER(int, Position, 0)
	QA_OUTPUT(double, PickedElement)
	
	QA_IMPL_CREATE(ElementPicker)
	QA_CTOR_INHERIT
	
public:
	void run();
};

class Mean: public QAlgorithm
{
	Q_OBJECT
	
	QA_INPUT_LIST(double, Array)
	QA_OUTPUT(double, Mean)
	
	QA_IMPL_CREATE(Mean)
	QA_CTOR_INHERIT
	
public:
	void run();
};

class ApplicationCloser: public QAlgorithm
{
	Q_OBJECT
	
	QA_IMPL_CREATE(ApplicationCloser)
	QA_CTOR_INHERIT
	
public:
	void run();
};

int main(int argc, char* argv[]){
	QCoreApplication app(argc, argv);
	QTimer::singleShot(0, []()
	{
		// Some parameters
		int repetitions = 10;
		int lengthRandomArray = 100;
		int movAverageSize = 5;
		int pickerPosition = 30;
		int percentileOrder = 70;
		// Allocate an algorithm that will perform the means
		auto meanPosition = Mean::create({
			QAlgorithm::makePropagationRules({{"PickedElement","Array"}})
		});
		auto meanPercentile = Mean::create({
			QAlgorithm::makePropagationRules({{"Percentile","Array"}})
		});
		
		for(int k = 0; k < repetitions; ++k)
		{
			// Allocate a RandomGenerator that will create some random numbers
			auto generator = RandomGenerator::create({{"Amount",lengthRandomArray}});
			// Allocate the moving average algorithm of these numbers
			auto movAverage = MovingAverage::create(
			{
				QAlgorithm::makePropagationRules({{"Numbers","Array"}}),
				{"Size",movAverageSize}
			});
			// Allocate the algorithm that picks an element
			auto picker = ElementPicker::create({{"Position",pickerPosition}});
			// Allocate the algorithm that computes the percentile
			auto percentile = Percentile::create(
			{
				QAlgorithm::makePropagationRules({{"Numbers","Array"}}),
				{"Order",percentileOrder}
			});
			// Partial output
			QObject::connect(picker.data(), &QAlgorithm::justFinished, [picker,k]()
							 {
								 qDebug() << "At" << k << "-th iteration, the picker was" << picker->getOutPickedElement();
							 });
			QObject::connect(percentile.data(), &QAlgorithm::justFinished, [percentile,k]()
							 {
								 qDebug() << "At" << k << "-th iteration, the percentile was" << percentile->getOutPercentile();
							 });
			// Make connections
			generator >> movAverage >> picker >> meanPosition;
			meanPercentile << percentile << generator;
		}
		// Print results from the mean algorithms
		QObject::connect(meanPosition.data(), &QAlgorithm::justFinished, [meanPosition,pickerPosition]()
						 {
							 qDebug() << "After computing the moving average, the" << pickerPosition <<
							 "-th position has the mean value of" << meanPosition->getOutMean();
						 });
		QObject::connect(meanPercentile.data(), &QAlgorithm::justFinished, [meanPercentile,percentileOrder]()
						 {
							 qDebug() << "The" << percentileOrder << "-percentile of the random arrays" <<
							 "has the mean value of" << meanPercentile->getOutMean();
						 });
		// Connect the two means with an algorithm that closes the application
		// as soon as it gets the value of two algorithms
		auto closer = ApplicationCloser::create();
		closer << meanPercentile;
		closer << meanPosition;
		// Connect the raise signal of a node in the algorithm tree to handle exceptions
		QObject::connect(closer.data(), &QAlgorithm::raise, []()
		{
			qDebug() << "Exception caught, closing application";
			QCoreApplication::instance()->exit();
		});
		// Make the whole algorithm structure run
		//	closer->serialExecution(); // not working properly
		closer->parallelExecution();
	});
	return app.exec();
}

void RandomGenerator::run()
{
	if(getAmount() <= 0)
	{
		QString msg = "amount must be positive";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	QVector<double> numbers;
	numbers.reserve(getAmount());
	std::generate_n(std::back_inserter(numbers), getAmount(), []()
					{
						return QRandomGenerator::global()->generateDouble();
					});
	setOutNumbers(numbers);
}

void MovingAverage::run()
{
	if(getInArray().isEmpty())
	{
		QString msg = "input is empty";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	if(getInArray().size() < getSize())
	{
		QString msg = "moving average size is too low";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	// Move the input array, no need to copy it
	auto array = getInMoveArray();
	QVector<double> output;
	auto outputSize = array.size() - getSize();
	auto left = array.begin();
	auto right = array.begin()+getSize();
	std::generate_n(std::back_inserter(output), outputSize, [&left, &right]()
					{
						auto average = std::reduce(left, right) / std::distance(left, right);
						++left;
						++right;
						return average;
					});
	setOutArray(output);
}

void Percentile::run()
{
	if(getInArray().isEmpty())
	{
		QString msg = "input is empty";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	// Move the input array in order to modify it during sorting
	auto array = getInMoveArray();
	qSort(array);
	// Take the desired element
	auto picker = ElementPicker::create(
	{
		{"Position",int(getOrder()*array.size()/100.0)},
		{"Array",QVariant::fromValue(array)}
	});
	picker->run();
	setOutPercentile(picker->getOutMovePickedElement());
}

void ElementPicker::run()
{
	if(getInRefArray().isEmpty())
	{
		QString msg = "input is empty";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	if(getPosition() < 0 || getPosition() >= getInRefArray().size())
	{
		QString msg = "input is empty";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	setOutPickedElement(getInRefArray().at(getPosition()));
}

void Mean::run()
{
	if(getInRefArray().isEmpty())
	{
		QString msg = "input is empty";
		qDebug() << printName() << msg;
		abort(msg);
		return;
	}
	const auto& array = getInRefArray();
	auto mean = std::reduce(array.begin(), array.end()) / double(array.size());
	setOutMean(mean);
}

void ApplicationCloser::run()
{
	qDebug() << "All algorithms finished, closing the application";
	QCoreApplication::instance()->exit();
}

#include "main.moc"
