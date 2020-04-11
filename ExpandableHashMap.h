#ifndef EXPANDABLEHASHMAP_H
#define EXPANDABLEHASHMAP_H

#include <vector>

// ExpandableHashMap.h
// Skeleton for the ExpandableHashMap class template.  You must implement the first six
// member functions.

template<typename KeyType, typename ValueType>
class ExpandableHashMap
{
public:
	ExpandableHashMap(double maximumLoadFactor = 0.5);
	~ExpandableHashMap();
	void reset();
	int size() const;
	void associate(const KeyType& key, const ValueType& value);
	// for a map that can't be modified, return a pointer to const ValueType
	const ValueType* find(const KeyType& key) const;
	// for a modifiable map, return a pointer to modifiable ValueType
	ValueType* find(const KeyType& key)
	{
		return const_cast<ValueType*>(const_cast<const ExpandableHashMap*>(this)->find(key));
	}
	// C++11 syntax for preventing copying and assignment
	ExpandableHashMap(const ExpandableHashMap&) = delete;
	ExpandableHashMap& operator=(const ExpandableHashMap&) = delete;
private:
	int m_buckets;
	double m_load;
	int m_size; //the # of associations in the hash map
	struct Association
	{
		KeyType m_key;
		ValueType m_value;
	};

	std::vector<Association>** m_hashMap;

};
template <typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::ExpandableHashMap(double maximumLoadFactor)
{
	m_buckets = 8;
	if (maximumLoadFactor <= 0)
		m_load = 0.5;
	else
		m_load = maximumLoadFactor;
	m_size = 0;
	m_hashMap = new std::vector<Association> * [m_buckets];
	for (int i = 0; i < m_buckets; i++)
	{
		m_hashMap[i] = nullptr;
	}
}
template <typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::~ExpandableHashMap()
{
	for (int i = 0; i < m_buckets; i++) 
	{
		delete m_hashMap[i];
	}
	delete[] m_hashMap;
}
template <typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::reset()
{
	for (int i = 0; i < m_buckets; i++)
	{
		delete m_hashMap[i];
	}
	delete m_hashMap;
	m_buckets = 8;
	m_hashMap = new std::vector<Association> * [m_buckets];
}
template <typename KeyType, typename ValueType>
int ExpandableHashMap<KeyType, ValueType>::size() const
{
	return m_size;
}
template <typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::associate(const KeyType& key, const ValueType& value)
{
	ValueType* ptr = find(key);
	//if the key already exists, update its value
	if (ptr != nullptr)
	{
		*ptr = value;
	}
	else
	{
		unsigned int hasher(const KeyType& g);
		unsigned int h = hasher(key);
		unsigned int index = (h % m_buckets);

		std::vector<Association>* vectorOfInterest = m_hashMap[index];

		//if there is no pointer to vector in the desired index slot of the hash map, create one.
		//Either way, create a new Association and push back onto the vector
		if (vectorOfInterest == nullptr)
		{
			vectorOfInterest = new std::vector <Association>;
			m_hashMap[index] = vectorOfInterest;
		}
		Association newAssociation;
		newAssociation.m_key = key;
		newAssociation.m_value = value;
		vectorOfInterest->push_back(newAssociation);
		m_size++; 

		//if the load of the hashmap exceeds the max load limit, rehash it 
		double tempLoad = (double)size() / m_buckets;
		if (tempLoad >= m_load)
		{
			m_buckets *= 2;
			m_size = 0; 
			std::vector<Association>** newHashMap = new std::vector<Association> * [m_buckets]; 
			for (int i = 0; i < m_buckets; i++)
			{
				newHashMap[i] = nullptr;
			}
			for (int i = 0; i < m_buckets / 2; i++)
			{
				//create a new vector in the new hash map only if one existed in the previous one
				if (m_hashMap[i] != nullptr)
				{
					for (int j = 0; j < (*m_hashMap[i]).size(); j++)
					{
						//rehash into a new slot in the new hash map
						//create a pointer to a vector there if one doesn't already exist
						unsigned int hasher(const KeyType& g);
						unsigned int newIndex = hasher((*m_hashMap[i])[j].m_key) % m_buckets;

						if (newHashMap[newIndex] == nullptr)
						{
							newHashMap[newIndex] = new std::vector<Association>;
							//m_size++;
						}
						Association copyAssociation;
						copyAssociation.m_key = (*m_hashMap[i])[j].m_key;
						copyAssociation.m_value = (*m_hashMap[i])[j].m_value;
						(*newHashMap[newIndex]).push_back(copyAssociation);
						m_size++; 
					}
					delete m_hashMap[i];
				}
			}
			//delete the old hash map and set the pointer equal to the new one
			delete[] m_hashMap;
			m_hashMap = newHashMap;
		}
	}
}
template <typename KeyType, typename ValueType>
const ValueType* ExpandableHashMap<KeyType, ValueType>::find(const KeyType& key) const
{
	//get the index in which the value type corresponding to the key would have been stored
	unsigned int hasher(const KeyType& g);
	unsigned int index = hasher(key) % m_buckets;
	std::vector<Association>* vectorOfInterest = m_hashMap[index];
	if (vectorOfInterest != nullptr)
	{
		for (int j = 0; j < vectorOfInterest->size(); j++)
		{
			if ((*vectorOfInterest)[j].m_key == key)
			{
				const ValueType* ptr = &((*vectorOfInterest)[j].m_value);
				return ptr;
			}
		}
	}
	return nullptr;
}
#endif