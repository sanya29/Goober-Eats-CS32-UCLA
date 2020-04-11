#include "provided.h"
#include <list>
#include <queue>
#include <unordered_map>
#include "ExpandableHashMap.h"
#include <map>
using namespace std;

class PointToPointRouterImpl
{
public:
    PointToPointRouterImpl(const StreetMap* sm);
    ~PointToPointRouterImpl();
    DeliveryResult generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const;
private: 
	const StreetMap* StreetMapPtr;
};

PointToPointRouterImpl::PointToPointRouterImpl(const StreetMap* sm)
{
	StreetMapPtr = sm;
}

PointToPointRouterImpl::~PointToPointRouterImpl()
{
}

DeliveryResult PointToPointRouterImpl::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
	route.clear();
	vector<StreetSegment> segs; 
	if (StreetMapPtr->getSegmentsThatStartWith(start, segs) == false || StreetMapPtr->getSegmentsThatStartWith(end, segs) == false)
	{
		cerr << "BAD_COORD returned" << endl;
		return BAD_COORD;
	}

	if (start == end)
		return DELIVERY_SUCCESS; //eg. if all deliveries are at the depot itself

	queue<GeoCoord> GQ;
	ExpandableHashMap<GeoCoord, GeoCoord> locationOfPreviousWayPoint;
	GQ.push(start);
	bool pathFound = false;

	while (!GQ.empty())
	{
		GeoCoord current = GQ.front();
		GQ.pop();

		if (current == end)
		{
			pathFound = true;
			break;
		}
		//find all segments that start with the current location and
		//(1) push them onto the queue of GeoCoords for later evaluation,
		//(2) make associations in the hash map
		if (StreetMapPtr->getSegmentsThatStartWith(current, segs))
		{
			for (int i = 0; i < segs.size(); i++)
			{
				if (locationOfPreviousWayPoint.find(segs[i].end) == nullptr)
				{
					GQ.push(segs[i].end);
					locationOfPreviousWayPoint.associate(segs[i].end, current);
				}
			}
		}
		while (!segs.empty())
			segs.pop_back();
	}

	if (!pathFound)
	{
		cerr << "NO_ROUTE returned" << endl;
		return NO_ROUTE; 
	}
	else
	{
		//reconstruct the whole route backwards using the associations from the hash map
		GeoCoord myEnd = end;
		while (myEnd!=start)
		{
			GeoCoord startCoord = myEnd;
			GeoCoord endCoord = *(locationOfPreviousWayPoint.find(startCoord));
			vector<StreetSegment> vec;
			StreetMapPtr->getSegmentsThatStartWith(startCoord, vec);
			for (int i = 0; i < vec.size(); i++)
			{
				if (vec[i].end == endCoord)
				{
					StreetSegment temp(endCoord, startCoord, vec[i].name);
					route.push_front(temp);
					break;
				}
			}
			totalDistanceTravelled += distanceEarthMiles(endCoord, startCoord); 
			myEnd = endCoord;
		}
	}
	return DELIVERY_SUCCESS;
	
}

//******************** PointToPointRouter functions ***************************

// These functions simply delegate to PointToPointRouterImpl's functions.
// You probably don't want to change any of this code.

PointToPointRouter::PointToPointRouter(const StreetMap* sm)
{
    m_impl = new PointToPointRouterImpl(sm);
}

PointToPointRouter::~PointToPointRouter()
{
    delete m_impl;
}

DeliveryResult PointToPointRouter::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
    return m_impl->generatePointToPointRoute(start, end, route, totalDistanceTravelled);
}
