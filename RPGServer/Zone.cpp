#include"Zone.h"

ZoneManager::ZoneManager()
{
	head = new Zone;
	tail = new Zone;
	head->SetNext(tail);
	tail->SetNext(nullptr);
	head->SetID(0x80000000);
	tail->SetID(0x7FFFFFFF);
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

				//r_lock.unlock();
				curr->unlock();
				prev->unlock();
				
				return;
			}
			else
			{
				//r_lock.unlock();
				curr->unlock();
				prev->unlock();
				return;
			}
		}
		//r_lock.unlock();
		curr->unlock();
		prev->unlock();
	}
}

void ZoneManager::REMOVE(int id)
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
				//r_lock.unlock();
				curr->unlock();
				prev->unlock();
				return;
			}
			else {
				curr->removed = true;
				prev->SetNext(curr->GetNext());
				//r_lock.unlock();
				curr->unlock();
				prev->unlock();
				//delete curr;
				//removeObj(curr);
				return;
			}
		}
		//r_lock.unlock();
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
	std::unique_lock<std::shared_mutex> rlock(r_lock);
	delete obj;
	r_lock.unlock();
}

bool ZoneManager::validate(Zone* prev, Zone* curr)
{
	return (!prev->removed) &&
		(!curr->removed) &&
		(prev->GetNext() == curr);
}