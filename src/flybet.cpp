#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/transaction.hpp>
#include <vector>

using namespace eosio;

class [[eosio::contract("flybet")]] flybet : public contract {
  public:
      using contract::contract;
    
        flybet( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds), global_state(receiver, code.value)
    {}

struct [[eosio::class]] room
{
     uint8_t               room_id;

     uint8_t               max_players;
     std::vector<eosio::name> players;
     uint64_t              bet;
};

struct [[eosio::table]] globalstate
    {
        uint64_t id;

        uint64_t last_room_id;
        std::vector<room> rooms;

        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE( globalstate, (id)(rooms)(last_room_id))
    };

typedef eosio::multi_index<"globalstate"_n, globalstate> statetable;
statetable global_state;

[[eosio::action]]
void createroom(uint8_t max_players, uint64_t bet, eosio::name creator)
{
     require_auth(creator);

     auto _state = global_state.find(0);
     global_state.modify(_state, get_self(), [&](auto& p) {
          room new_room;
          new_room.bet = bet;
          new_room.max_players = max_players;
          new_room.room_id = _state->last_room_id;
          new_room.players.push_back(creator);

          p.rooms.push_back(new_room);

          p.last_room_id++;
     });

     // TODO: write test for this.
}

[[eosio::action]]
void setup()
{
     global_state.emplace(get_self(), [&](auto& p) {
          p.id = 0;
          p.last_room_id = 0;
      });
}

[[eosio::action]]
void clear()
{
     auto e = global_state.find(0);
     if(e != global_state.end())
     {
          global_state.erase(e);
     }
}

/*

[[eosio::action]]
void call(uint64_t ix, uint64_t id)
{
     // ID is the number of iterations
     // i  is a dummy value to prevent tx duplications

     eosio::print("DEBUG PRINT");

     uint64_t b = 0;

     // 200K iterations will consume 22 MS of CPU per call.

     for (uint64_t i=0; i < id; i++)
     {
          uint64_t j = i * i;
          uint64_t k = j * i;
          b = j * k;
     }

     eosio::print("B: ", b);
}

[[eosio::action]]
void setstat(uint64_t s)
{
     auto e = counter.find(0);
     if(counter.begin() == counter.end())
     {
          counter.emplace(get_self(), [&](auto& p) {
               p.id = 0;
               p.value = 0;
               p.allow_cyclic = s;
          });

          eosio::print("AAAL   ", counter.find(0)->allow_cyclic);
     }
     else
     {
          counter.modify(e, get_self(), [&](auto& p) {
               p.allow_cyclic = s;
          });
     }
}

[[eosio::action]]
void set(eosio::name from, uint64_t delay, uint64_t defer, uint64_t repeat, uint64_t id)
{
     auto e = counter.find(0);

     if(counter.begin() == counter.end())
     {
          counter.emplace(get_self(), [&](auto& p) {
               p.id = 0;
               p.value = 0;
               p.allow_cyclic = 1;
          });
          e = counter.find(0);
     }

     eosio::print("E->cycles:    ", e->allow_cyclic);
     eosio_assert(e->allow_cyclic > 0, "Cycles disabled by owner");

     for (uint64_t i = 0; i<repeat; i++)
     {
          counter.modify(e, get_self(), [&](auto& s) {
               s.value++;
          });

          eosio::transaction c{};
          c.actions.emplace_back(
               eosio::permission_level(_self, "active"_n),
               _self,
               "call"_n,
               std::make_tuple(i, id));
               c.delay_sec = defer;
          c.send(e->value, _self, true);
     }

     eosio::transaction t{};
     t.actions.emplace_back(
          eosio::permission_level(_self, "active"_n),
          _self,
          "set"_n,
          std::make_tuple(from, delay, defer, repeat, id));
     t.delay_sec = delay;
     t.send(now(), _self, true);
}
*/

};


EOSIO_DISPATCH( flybet, (setup)(createroom)(clear))