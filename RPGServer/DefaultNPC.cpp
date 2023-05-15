#include "DefaultNPC.h"

void NPC::add_objects(int o_id)
{
	{
		std::unique_lock<std::shared_mutex> lock(_vl);
		if (_view_list.count(o_id) != 0) {
			lock.unlock();
			return;
		}
		_view_list.insert(o_id);
	}
}
