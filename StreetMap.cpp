#include "provided.h"
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <sstream>
#include "ExpandableHashMap.h" 
using namespace std;


unsigned int hasher(const GeoCoord& g)
{
	return std::hash<string>()(g.latitudeText + g.longitudeText);
}

class StreetMapImpl
{
public:
	StreetMapImpl();
	~StreetMapImpl();
	bool load(string mapFile);
	bool getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const;
private:
	ExpandableHashMap<GeoCoord, vector<StreetSegment>> coord2seg;
};

StreetMapImpl::StreetMapImpl()
{
}

StreetMapImpl::~StreetMapImpl()
{
}

bool StreetMapImpl::load(string mapFile)
{
	//return false;  // Delete this line and implement this function correctly
	ifstream i1(mapFile);    // infile is a name of our choosing
	if (!i1)		        // Did opening the file fail?
	{
		cerr << "Error: Cannot open mapdata.txt!" << endl;
		return false;
	}

	string line;
	int count = 0; //to keep track of which line/type of data we are extracting
	string nameOfStreet;
	int k = 0, i = 0; //k = the # of street segments per street
					  //i = iterator to go through all k street segments

	while (getline(i1, line))
	{
		istringstream iss(line);
		if (count == 0)
		{
			nameOfStreet = line;
			count++;
		}
		else if (count == 1)
		{
			iss >> k;
			count++;
		}
		else if (count == 2)
		{
			string lat1, lat2, lon1, lon2;

			iss >> lat1;
			iss >> lon1;
			iss >> lat2;
			iss >> lon2;

			GeoCoord start(lat1, lon1), end(lat2, lon2);

			StreetSegment S1(start, end, nameOfStreet);
			StreetSegment S2(end, start, nameOfStreet);

			i++;
			
			//map the coordinates to a vector of segments
			if (coord2seg.find(start) != nullptr)
			{
				coord2seg.find(start)->push_back(S1);
			}
			else
			{
				vector<StreetSegment> segV1;
				segV1.push_back(S1);
				coord2seg.associate(start, segV1);
			}
			if (coord2seg.find(end) != nullptr)
			{
				coord2seg.find(end)->push_back(S2);
			}
			else
			{
				vector <StreetSegment> segV2;
				segV2.push_back(S2);
				coord2seg.associate(end, segV2);
			}

			//if all the segments have already been extracted, set count to 0 so that the next
			//thing to be extracted is the name of the next street
			if (i == k)
			{
				count = 0;
				i = 0;
			}
			
		}
	}
	return true;
}

bool StreetMapImpl::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
	if (coord2seg.find(gc) != nullptr)
	{
		segs = *coord2seg.find(gc);
		return true;
	}
	return false;
}

//******************** StreetMap functions ************************************

// These functions simply delegate to StreetMapImpl's functions.
// You probably don't want to change any of this code.

StreetMap::StreetMap()
{
	m_impl = new StreetMapImpl;
}

StreetMap::~StreetMap()
{
	delete m_impl;
}

bool StreetMap::load(string mapFile)
{
	return m_impl->load(mapFile);
}

bool StreetMap::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
	return m_impl->getSegmentsThatStartWith(gc, segs);
}
