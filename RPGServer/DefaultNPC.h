#pragma once
#include "NetWork.h"

class NPC : public Object
{
public:
	volatile bool _n_wake;	// �ֺ��� �÷��̾ �ִٸ� �ѹ��� wake�� true �ٲ��־� wake�۵��ϰ� �Ѵ�.
	
	void add_objects(int o_id);
};
