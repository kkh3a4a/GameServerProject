#include"Zone.h"

ZoneManager::ZoneManager()
{
	head = new Zone;
	tail = new Zone;
	head->SetNext(tail);
	tail->SetNext(nullptr);
	head->SetID(0x80000000);
	tail->SetID(0x7FFFFFFF);
	std::thread cleanupThread(&ZoneManager::backgroundCleanup, this);
	cleanupThread.detach();
}

void ZoneManager::ADD(int id)
{
	while (1) {
		//std::shared_lock<std::shared_mutex> rlock(r_lock);
		Zone* prev = head;
		Zone* curr = prev->GetNext();
		while (curr->GetObjID() < id) {
			prev = curr;
			curr = curr->GetNext();
		}
		prev->lock();
		curr->lock();
		if (validate(prev, curr))
		{
			if (curr->GetObjID() != id) {
				Zone* node = new Zone{ id };
				node->SetNext(curr);
				prev->SetNext(node);
				curr->unlock();
				prev->unlock();
				
				return;
			}
			else
			{
				curr->unlock();
				prev->unlock();
				return;
			}
		}
		curr->unlock();
		prev->unlock();
	}
}

void ZoneManager::REMOVE(int id)
{
	while (1) {
		Zone* prev = head;
		Zone* curr = prev->GetNext();
		while (curr->GetObjID() < id) {
			prev = curr;
			curr = curr->GetNext();
		}
		prev->lock();
		curr->lock();
		if (validate(prev, curr))
		{
			if (curr->GetObjID() != id) {
				curr->unlock();
				prev->unlock();
				return;
			}
			else {
				curr->removed = true;
				prev->SetNext(curr->GetNext());
				curr->unlock();
				prev->unlock();
				removeObj(curr);
				return;
			}
		}
		curr->unlock();
		prev->unlock();
	}
}


void ZoneManager::Zonelist(set<int>& a)
{
	Zone* prev = head;
	Zone* curr = prev->GetNext();
	while (curr->GetNext() != nullptr) {
		a.insert(curr->GetObjID());
		curr = curr->GetNext();
	}
	
}
void ZoneManager::removeObj(Zone* obj)
{
	std::lock_guard<std::mutex> lock(deletionMutex);
	deletedObjects.push_back(obj);
}

void ZoneManager::backgroundCleanup()
{
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(600));
		{
			std::lock_guard<std::mutex> lock(deletionMutex);
			for (auto obj : deletedObjects) {
				delete obj;
			}
			deletedObjects.clear();
		}
	}
}



bool ZoneManager::validate(Zone* prev, Zone* curr)
{
	return (!prev->removed) &&
		(!curr->removed) &&
		(prev->GetNext() == curr);
}