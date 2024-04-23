#pragma once
#include"NetWork.h"



class Zone {

	int _id;
	Zone* _next;
	
public:
	mutex n_lock;
	volatile bool removed;


	Zone() {
		_id = -1;
		_next = nullptr;
		removed = false;
	}
	Zone(int id) :_id(id) {}

	Zone(int id, Zone* next) :_id(id), _next(next) { removed = false; }
	void SetNext(Zone* next)
	{
		_next = next;
	}
	Zone* GetNext()
	{
		return _next;
	}

	int GetObjID()
	{
		return _id;
	}
	void SetID(int id)
	{
		_id = id;
	}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};


class ZoneManager {
public:
	Zone* head;
	Zone* tail;
	shared_mutex r_lock;
	ZoneManager();
	
	vector<Zone*> deletedObjects;
	mutex deletionMutex;

	void ADD(int);
	void REMOVE(int);
	bool validate(Zone* prev, Zone* curr);
	void removeObj(Zone* zone);
	void Zonelist(set<int>& a);
	void backgroundCleanup();
};