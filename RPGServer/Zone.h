#pragma once
#include"NetWork.h"

class Zone;
class Zone_PTR {
	unsigned long long next;
public:
	Zone_PTR() : next(0) {}
	Zone_PTR(bool removed, Zone* ptr)
	{
		next = reinterpret_cast<unsigned long long>(ptr);
		if (true == removed) next = next | 1;
	}
	Zone* get_ptr()
	{
		return reinterpret_cast<Zone*>(next & 0xFFFFFFFFFFFFFFFE);
	}
	bool get_removed()
	{
		return (next & 1) == 1;
	}
	Zone* get_ptr_marked(bool* removed)
	{
		unsigned long long cur_next = next;
		*removed = (cur_next & 1) == 1;
		return reinterpret_cast<Zone*>(cur_next & 0xFFFFFFFFFFFFFFFE);
	}
	bool CAS(unsigned long long o_next, unsigned long long n_next) {
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_uint64_t*>(&next), &o_next, n_next);

	}
	bool CAS(Zone* o_ptr, Zone* n_ptr, bool o_mark, bool n_mark)
	{
		unsigned long long o_next = reinterpret_cast<unsigned long long>(o_ptr);
		if (true == o_mark) o_next++;
		unsigned long long n_next = reinterpret_cast<unsigned long long>(n_ptr);
		if (true == n_mark) n_next++;
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_uint64_t*>(&next), &o_next, n_next);
	}
	Zone* get_ptr(bool* removed) {
		unsigned long long  temp = next;
		if (0 == (temp & 1)) *removed = false;
		else *removed = true;
		return reinterpret_cast<Zone*>(temp & 0xFFFFFFFFFFFFFFFE);
	}
	bool attedmpMark(Zone* o_ptr, bool mark)
	{
		unsigned long long o_next = reinterpret_cast<unsigned long long>(o_ptr);
		unsigned long long n_next = o_next;

		if (mark) n_next = n_next | 1;
		else n_next = n_next & 0xFFFFFFFFFFFFFFFE;

		return CAS(o_next, n_next);
	}
};

class Zone {

public:
	int _id;
	Zone_PTR _next;
	Zone() : _next(false, nullptr) {_id = -1;}
	Zone(int id) :_id(id),_next(false, nullptr) {}
	Zone(int id, Zone* next) :_id(id), _next(false, next) {}
};

class ZoneManager {
public:
	Zone head;
	Zone tail;
	ZoneManager();
	
	vector<Zone*> deletedObjects;
	mutex deletionMutex;

	void ADD(int);
	void REMOVE(int);
	void Find(Zone*& prev, Zone*& curr, int id);
	void Zonelist(set<int>& a);

};