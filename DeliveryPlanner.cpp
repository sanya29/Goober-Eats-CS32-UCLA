#include "provided.h"
#include <vector>
using namespace std;

class DeliveryPlannerImpl
{
public:
    DeliveryPlannerImpl(const StreetMap* sm);
    ~DeliveryPlannerImpl();
    DeliveryResult generateDeliveryPlan(
        const GeoCoord& depot,
        const vector<DeliveryRequest>& deliveries,
        vector<DeliveryCommand>& commands,
        double& totalDistanceTravelled) const;
private:
	const StreetMap* StreetMapPtr;
	inline
	string calculateDirection(double angle) const
	{
		if (angle >= 0 && angle < 22.5)
			return "east";
		else if (angle >= 22.5 && angle < 67.5)
			return "northeast";
		else if (angle >= 67.5 && angle < 112.5)
			return "north";
		else if (angle >= 112.5 && angle < 157.5)
			return "northwest";
		else if (angle >= 157.5 && angle < 202.5)
			return "west";
		else if (angle >= 202.5 && angle < 247.5)
			return "southwest";
		else if (angle >= 247.5 && angle < 292.5)
			return "south";
		else if (angle >= 292.5 && angle < 337.5)
			return "southeast";
		else if (angle >= 337.5)
			return "east";
		return "";
	}
};

DeliveryPlannerImpl::DeliveryPlannerImpl(const StreetMap* sm)
{
	StreetMapPtr = sm;
}

DeliveryPlannerImpl::~DeliveryPlannerImpl()
{
}

DeliveryResult DeliveryPlannerImpl::generateDeliveryPlan(
    const GeoCoord& depot,
    const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands,
    double& totalDistanceTravelled) const
{
	//first optimize the delivery requests
	vector<DeliveryRequest> optimizedDeliveries;

	for (int i = 0; i < deliveries.size(); i++)
		optimizedDeliveries.push_back(deliveries[i]);

	DeliveryOptimizer DO(StreetMapPtr);
	double oldCrowDistance, newCrowDistance;
	DO.optimizeDeliveryOrder(depot, optimizedDeliveries, oldCrowDistance, newCrowDistance);

	//now generate a route from the depot to all the delivery locations and back to the depot.
	//add all the individual routes between the delivery points to one long list of street segments
	//called route2
	PointToPointRouter p2p(StreetMapPtr);
	list<StreetSegment> route;
	list<StreetSegment> route2;
	const GeoCoord home = depot;
	totalDistanceTravelled = 0;

	//first generate the route from the depot to the first delivery point, and add it to route2
	p2p.generatePointToPointRoute(home, optimizedDeliveries[0].location, route, totalDistanceTravelled); cerr << "cp1\n";
	for (auto x = route.begin(); x != route.end(); x++)
		route2.push_back(*x);

	//now generate the route between successive delivery locations and back to the depot
	for (int i = 0; i < optimizedDeliveries.size(); i++)
	{
		if (i == optimizedDeliveries.size() - 1)
		{
			p2p.generatePointToPointRoute(optimizedDeliveries[i].location, home, route, totalDistanceTravelled);
			for (auto x = route.begin(); x != route.end(); x++)
				route2.push_back(*x);
		}
		else
		{
			p2p.generatePointToPointRoute(optimizedDeliveries[i].location, optimizedDeliveries[i + 1].location, route, totalDistanceTravelled);
			for (auto x = route.begin(); x != route.end(); x++)
				route2.push_back(*x);
		}
	}

	int deliveryNo = 0;

	if (route2.empty()) //eg. if all deliveries are at the depot
	{
		while (deliveryNo < optimizedDeliveries.size() && optimizedDeliveries[deliveryNo].location == depot)
		{
			DeliveryCommand DC;
			DC.initAsDeliverCommand(optimizedDeliveries[deliveryNo].item);
			commands.push_back(DC);
			deliveryNo++;
		}
		return DELIVERY_SUCCESS;
	}

	list<StreetSegment>::iterator it = route2.begin();
	list<StreetSegment>::iterator temp = ++it;
	it--; //temp is one street segment ahead of it

	string streetName;
	string dir;
	double dist = 0;
	bool firstSegment = true; //tells us if the present street segment is the first of the street
							  //helps us determine the direction for the proceed command

	while (temp != route2.end()) 
	{
		DeliveryCommand DC;
		
		//if a delivery is to be made at the start of this street segment
		if (optimizedDeliveries[deliveryNo].location == it->start)
		{
			//if some distance was travelled on the street to get to the delivery point
			//as opposed to a delivery being just after turning onto a new street
			//give the proceed command followed by the delivery command
			if (dist != 0) 
			{
				DC.initAsProceedCommand(dir, (*it).name, dist);
				commands.push_back(DC);
				cerr << "\nI AM DELIVERING SOMETHING////////////////////\n";
				dist = 0;
			}

			firstSegment = true; 

			DC.initAsDeliverCommand(optimizedDeliveries[deliveryNo].item);
			commands.push_back(DC);

			if (deliveryNo < optimizedDeliveries.size())
				deliveryNo++;

			//if there are multiple deliveries in the same location
			while (deliveryNo < optimizedDeliveries.size())
			{
				if (optimizedDeliveries[deliveryNo].location == it->start)
				{
					DC.initAsDeliverCommand(optimizedDeliveries[deliveryNo].item);
					commands.push_back(DC); 
					cerr << "\nI AM DELIVERING SOMETHING////////////////////\n";
					deliveryNo++;
				}
				else
					break;
			}
		}
		if (it->name == temp->name)
		{
			//if the street segments pointed to by both iterators are the same i.e. we are on the same street
			//only calculate the direction based on the direction of the first street segment.
			if (firstSegment)
			{
				firstSegment = false;
				double angle = angleOfLine(*it);
				dir = calculateDirection(angle);
			}
			dist += distanceEarthMiles(it->start, it->end); 
		}
		else if (it->name != temp->name)
		{
			if (firstSegment)
			{
				double angle3 = angleOfLine(*it);
				dir = calculateDirection(angle3);
			}
			
			//since the two street segments don't belong to the same street, give the proceed command
			//based on the cumulative distance travelled until now
			DC.initAsProceedCommand(dir, it->name, dist + distanceEarthMiles(it->start, it->end));
			commands.push_back(DC);
			firstSegment = true; //this is true because the next street segment that we move onto 
								 //will be the first of that street

			//if there is a delivery to be made on the next street segment, skip the rest of the steps
			//i.e. don't turn
			if (temp->start == optimizedDeliveries[deliveryNo].location)
			{
				dist = 0;
				it++;
				temp++;
				continue;
			}

			//decide which direction to turn in or proceed
			double angle = angleBetween2Lines(*it, *temp);
			if (angle < 1 || angle > 359)
			{
				double angle2 = angleOfLine(*temp);
				dir = calculateDirection(angle2);

				firstSegment = false;
			}
			else if (angle >= 1 && angle < 180)
			{
				DeliveryCommand DC2;
				DC2.initAsTurnCommand("left", temp->name); 
				commands.push_back(DC2);
				dir = calculateDirection(angleOfLine(*temp));
			}
			else if (angle >= 180 && angle <= 359)
			{
				DeliveryCommand DC2; 
				DC2.initAsTurnCommand("right", temp->name);
				commands.push_back(DC2);
				dir = calculateDirection(angleOfLine(*temp));
			}
			dist = 0;
		}
		it++;
		temp++;
	}

	//since the while loop runs until temp is just past the end, and because iterator
	//temp is one street segment ahead of iterator it, the last segment of the route remains unevaluated
	DeliveryCommand lastleg;
	bool firstProceed = true;

	//if there are any deliveries left, they are either at the start of the last segment or the end
	if (deliveryNo < optimizedDeliveries.size())
	{
		while (deliveryNo < optimizedDeliveries.size())
		{
			if (optimizedDeliveries[deliveryNo].location == it->start) 
			{
				lastleg.initAsDeliverCommand(optimizedDeliveries[deliveryNo].item);
				commands.push_back(lastleg);
				deliveryNo++;
				cerr << "\nI AM DELIVERING SOMETHING//////////////////// LAST LEG start\n";
				if (deliveryNo == optimizedDeliveries.size())
				{
					double angle = angleOfLine(*it);
					string newdir = calculateDirection(angle);
					lastleg.initAsProceedCommand(newdir, it->name, distanceEarthMiles(it->start, it->end));
					commands.push_back(lastleg);
				}
			}
			else if (optimizedDeliveries[deliveryNo].location == it->end) 
			{
				if (firstProceed)
				{
					lastleg.initAsProceedCommand(dir, it->name, dist + distanceEarthMiles(it->start, it->end));
					commands.push_back(lastleg);
					firstProceed = false;
				}
				lastleg.initAsDeliverCommand(optimizedDeliveries[deliveryNo].item);
				commands.push_back(lastleg);
				cerr << "\nI AM DELIVERING SOMETHING//////////////////// LAST LEG end\n";
				deliveryNo++;
			}
		}
	}
	else
	{
		lastleg.initAsProceedCommand(dir, it->name, dist + distanceEarthMiles(it->start, it->end));
		commands.push_back(lastleg);
	}
	cerr<<"Ignore everything until here.....\n///////////\n////////////////////////////////////////////////\n";
	return DELIVERY_SUCCESS;

	//if it's it->start, you need to give proceed commands for the last leg. if it's it->end, you need to just not do anything
	//check if your turns are taken care of in the last leg
}

//******************** DeliveryPlanner functions ******************************

// These functions simply delegate to DeliveryPlannerImpl's functions.
// You probably don't want to change any of this code.

DeliveryPlanner::DeliveryPlanner(const StreetMap* sm)
{
    m_impl = new DeliveryPlannerImpl(sm);
}

DeliveryPlanner::~DeliveryPlanner()
{
    delete m_impl;
}

DeliveryResult DeliveryPlanner::generateDeliveryPlan(
    const GeoCoord& depot,
    const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands,
    double& totalDistanceTravelled) const
{
    return m_impl->generateDeliveryPlan(depot, deliveries, commands, totalDistanceTravelled);
}
