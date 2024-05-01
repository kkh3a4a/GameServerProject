#include"Zone.h"

ZoneManager::ZoneManager()
{
	head._id = 0x80000000;
	tail._id = 0x7FFFFFFF;
	head._next = Zone_PTR{ false, &tail };
}

void ZoneManager::ADD(int id)
{
	while (1) {
		Zone* prev;
		Zone* curr;
		Find(prev, curr, id);
		if (curr->_id != id) {
			Zone* node = new Zone{ id , curr };
			if (true == prev->_next.CAS(curr, node, false, false))
				return;
			delete node;
		}
		else
		{
			return;
		}
	}
}

void ZoneManager::REMOVE(int id)
{
	while (1) {
		Zone* prev;
		Zone* curr;
		Find(prev, curr, id);
		if (curr->_id == id) {
			Zone* succ = curr->_next.get_ptr();
			/*if (!curr->next.attedmpMark(succ, true))
				continue;*/

			if (!curr->_next.CAS(succ, succ, false, true))
				continue;
			prev->_next.CAS(curr, succ, false, false);
			return;

		}
		else
		{
			return;
		}
	}
}

void ZoneManager::Find(Zone*& prev, Zone*& curr,int id)
{
	while(true)
	{
	retry:
		prev = &head;
		curr = prev->_next.get_ptr();
		while (true) {
			bool removed;
			Zone* succ = curr->_next.get_ptr_marked(&removed);
			while (true == removed) {
				if (false == prev->_next.CAS(curr, succ, false, false))
				{
					goto retry;
				}
				curr = succ;
				succ = curr->_next.get_ptr_marked(&removed);
			}
			if (curr->_id >= id)
			{
				return;
			}
			prev = curr;
			curr = succ;
		}
	}
}


void ZoneManager::Zonelist(set<int>& a)
{
	Zone* zone;
	zone = head._next.get_ptr();
	while (zone->_id != tail._id)
	{
		a.insert(zone->_id);
		zone = zone->_next.get_ptr();
	}
	
}
