myid = 99999;
countmove = 4;

function set_uid(x)
   myid = x;
end

function set_countmove(cnt)
	countmove = cnt;
end

function event_object_Defence(atk_id)
   atk_x = API_get_x(atk_id);
   atk_y = API_get_y(atk_id);
   my_x = API_get_x(myid);
   my_y = API_get_y(myid);
   if (math.abs(atk_x - my_x) <= 1 ) then
      if (math.abs(atk_y - my_y) <= 1) then
         API_Defence(atk_id, myid);
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
         API_Attack(myid, def_id);
      end
   end
end



function event_player_chat(player, msg)
   player_x = API_get_x(player);
   player_y = API_get_y(player);
   my_x = API_get_x(myid);
   my_y = API_get_y(myid);
   if (player_x == my_x) then
      if (player_y == my_y) then
         API_SendMessage(myid, player, msg);
      end
   end
end

