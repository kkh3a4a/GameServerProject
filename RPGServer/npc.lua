myid = 99999;
countmove = 4;
agro = 1;
type = 1;
function set_uid(x)
   myid = x;
end

function set_agro(y)
   agro = y;
end

function set_type(t)
   type = t;
end


function set_countmove(cnt)
	countmove = cnt;
end

function event_object_Defence(atk_id)
    atk_x = API_get_x(atk_id);
    atk_y = API_get_y(atk_id);
    my_x = API_get_x(myid);
    my_y = API_get_y(myid);
    r_msg = math.random(0,2);

    if (math.abs(atk_x - my_x) <= 1 ) then
        if (math.abs(atk_y - my_y) <= 1) then
            API_Defence(atk_id, myid);
			if (agro == 0) then
				if (r_msg == 0) then
					API_SendMessage(myid, "No!")
				elseif (r_msg == 1) then
					API_SendMessage(myid, "Hey!")
				elseif (r_msg == 2) then
					API_SendMessage(myid, "Stop!")
				end
			elseif (agro == 1) then
				if (r_msg == 0) then
					API_SendMessage(myid, "kket!")
				elseif (r_msg == 1) then
					API_SendMessage(myid, "Ow!")
				elseif (r_msg == 2) then
					API_SendMessage(myid, "Yow!")
				end
			end
		end
    end
end

function event_object_Attack(def_id)
   def_x = API_get_x(def_id);
   def_y = API_get_y(def_id);
   my_x = API_get_x(myid);
   my_y = API_get_y(myid);
   if (math.abs(def_x - my_x) <= 1 ) then
      if (math.abs(def_y - my_y) <= 1) then
			API_Default_Attack(myid, def_id);
		end
	end
end

function event_range_Attack()
	API_Range_Attack(myid, def_id, 1000);
end

function event_NPC_Attack_msg()
	r_msg = math.random(0,2);
	if (agro == 0) then
		if (r_msg == 0) then
			API_SendMessage(myid, "Kill!")
		elseif (r_msg == 1) then
			API_SendMessage(myid, "Die!")
		elseif (r_msg == 2) then
			API_SendMessage(myid, "Perish!")
		end
	elseif (agro == 1) then
		if (r_msg == 0) then
			API_SendMessage(myid, "Krrrr!")
		elseif (r_msg == 1) then
			API_SendMessage(myid, "Uh-oh!")
		elseif (r_msg == 2) then
			API_SendMessage(myid, "Argh!")
		end
	end
end

function event_NPC_chat(msg)
    API_SendMessage(myid, msg);
end

