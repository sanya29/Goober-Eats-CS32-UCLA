#include "provided.h"
#include <vector>
#include <cmath>
#include <utility>
#include <random>
using namespace std;

class DeliveryOptimizerImpl
{
public:
	DeliveryOptimizerImpl(const StreetMap* sm);
	~DeliveryOptimizerImpl();
	void optimizeDeliveryOrder(
		const GeoCoord& depot,
		vector<DeliveryRequest>& deliveries,
		double& oldCrowDistance,
		double& newCrowDistance) const;
private:
	void calculateCrowDistance(double& crowDistance, vector<DeliveryRequest> deliveries, const GeoCoord depot) const
	{
		crowDistance = distanceEarthMiles(depot, deliveries[0].location);
		for (int i = 0; i < deliveries.size() - 1; i++)
		{
			crowDistance += distanceEarthMiles(deliveries[i].location, deliveries[i + 1].location);
		}
		crowDistance += distanceEarthMiles(deliveries[deliveries.size() - 1].location, depot);
	}

	vector<DeliveryRequest> reorderDeliveries(const vector<DeliveryRequest> deliveries) const
	{
		vector<DeliveryRequest> randomlyGeneratedDeliveries;
		for (int i = 0; i < deliveries.size(); i++)
		{
			randomlyGeneratedDeliveries.push_back(deliveries[i]);
		}

		int randomIndexOne = randInt(0, deliveries.size() - 1);
		int randomIndexTwo = randInt(0, deliveries.size() - 1);

		string tempItem = randomlyGeneratedDeliveries[randomIndexOne].item;
		GeoCoord tempLocation = randomlyGeneratedDeliveries[randomIndexOne].location;
		randomlyGeneratedDeliveries[randomIndexOne].item = randomlyGeneratedDeliveries[randomIndexTwo].item;
		randomlyGeneratedDeliveries[randomIndexOne].location = randomlyGeneratedDeliveries[randomIndexTwo].location;
		randomlyGeneratedDeliveries[randomIndexTwo].item = tempItem;
		randomlyGeneratedDeliveries[randomIndexTwo].location = tempLocation;

		return randomlyGeneratedDeliveries;
	}

	inline
		int randInt(int min, int max) const
	{
		if (max < min)
			std::swap(max, min);
		static std::random_device rd;
		static std::default_random_engine generator(rd());
		std::uniform_int_distribution<> distro(min, max);
		return distro(generator);
	}
	const StreetMap* StreetMapPtr;
};

DeliveryOptimizerImpl::DeliveryOptimizerImpl(const StreetMap* sm)
{
	StreetMapPtr = sm;
}

DeliveryOptimizerImpl::~DeliveryOptimizerImpl()
{
}

void DeliveryOptimizerImpl::optimizeDeliveryOrder(
	const GeoCoord& depot,
	vector<DeliveryRequest>& deliveries,
	double& oldCrowDistance,
	double& newCrowDistance) const
{
	////Simulated Annealing: 

	calculateCrowDistance(oldCrowDistance, deliveries, depot);
	cerr << "Old Deliveries: ///////////////////////////\n";
	for (int i = 0; i < deliveries.size(); i++)
	{
		cerr <<i<< " j "<< deliveries[i].item << " k ";
	}
	cerr << endl;

	double temperature = 10000.0;
	double deltaDistance = 0;
	double coolingRate = 0.9999;
	double absoluteTemperature = 0.00001;

	double distance = 0;
	calculateCrowDistance(distance, deliveries, depot);

	vector<DeliveryRequest> newDeliveries; 
	for (int i = 0; i < deliveries.size(); i++)
	{
		newDeliveries.push_back(deliveries[i]);
	}

	while (temperature > absoluteTemperature)
	{
		//to randomly swap any two items in the vector
		newDeliveries = reorderDeliveries(deliveries);

		double tempNewDistance = 0;
		calculateCrowDistance(tempNewDistance, newDeliveries, depot);
		deltaDistance =  tempNewDistance - distance;
		
		//to calculate a random number between 0 and 1
		int toBeRandom = randInt(0, 1000);
		double nowRandom = (double)toBeRandom / 1000; 
		//doing this, turns any toBeRandom into a decimal number between 0 and 1.

		//accept the new solution if it has a smaller distance or satisfies the Boltzman condition
		if ((deltaDistance < 0) || (distance > 0 && (double)exp(-deltaDistance / temperature) > nowRandom))
		{
			for (int i = 0; i < newDeliveries.size(); i++)
				deliveries[i] = newDeliveries[i];

			distance = deltaDistance + distance;
		}

		//cool down the temperature
		temperature *= coolingRate;
	}

	calculateCrowDistance(newCrowDistance, deliveries, depot);

	cerr << "Old Crow Distance: " << oldCrowDistance << " New Crow Distance: " << newCrowDistance << endl;
	cerr << "New deliveries: //////////////////////////////////////////////\n";
	for (int i = 0; i < deliveries.size(); i++)
	{
		cerr << deliveries[i].item << " ";
	}
}


//******************** DeliveryOptimizer functions ****************************

// These functions simply delegate to DeliveryOptimizerImpl's functions.
// You probably don't want to change any of this code.

DeliveryOptimizer::DeliveryOptimizer(const StreetMap* sm)
{
	m_impl = new DeliveryOptimizerImpl(sm);
}

DeliveryOptimizer::~DeliveryOptimizer()
{
	delete m_impl;
}

void DeliveryOptimizer::optimizeDeliveryOrder(
	const GeoCoord& depot,
	vector<DeliveryRequest>& deliveries,
	double& oldCrowDistance,
	double& newCrowDistance) const
{
	return m_impl->optimizeDeliveryOrder(depot, deliveries, oldCrowDistance, newCrowDistance);
}
