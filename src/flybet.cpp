#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/system.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <vector>
#include <cctype>

using namespace eosio;

class [[eosio::contract("flybet")]] flybet : public contract {
  public:
      using contract::contract;
    
        flybet( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds ): eosio::contract(receiver, code, ds), global_state(receiver, code.value), rooms_table(receiver, code.value), balances_table(receiver, code.value)
    {}

struct [[eosio::class]] player
{
     eosio::name  account;
     checksum256  bet_hash;
};

struct [[eosio::table]] rooms
    {
          uint64_t id;

          uint64_t last_interaction_timestamp;

          uint8_t               max_players;
          uint8_t               disclosure_refund_ratio;  // Percentage of the refund paid back to the player
                                                     // if he discloses his hashed numbers.
          std::vector<player>   players;
          uint64_t              bet;

          uint64_t primary_key() const { return id; }
          EOSLIB_SERIALIZE( rooms, (id)(max_players)(disclosure_refund_ratio)(players)(bet))
    };

struct [[eosio::table]] globalstate
    {
        uint64_t id;

        uint64_t last_room_id;
        //std::vector<room> rooms;

        uint64_t primary_key() const { return id; }
        EOSLIB_SERIALIZE( globalstate, (id)(last_room_id))
    };

struct [[eosio::table]] balances
    {
        eosio::name    account;
        asset          balance;

        uint64_t primary_key() const { return account.value; }
        EOSLIB_SERIALIZE( balances, (account)(balance))
    };

typedef eosio::multi_index<"globalstate"_n, globalstate> statetable;
typedef eosio::multi_index<"balances"_n, balances> balancetable;
typedef eosio::multi_index<"rooms"_n, rooms> roomstable;
statetable   global_state;
balancetable balances_table;
roomstable   rooms_table;

[[eosio::action]]
void createroom(uint8_t max_players, uint64_t bet, eosio::name creator)
{
     require_auth(creator);

     auto _state = global_state.find(0);

     rooms_table.emplace(get_self(), [&](auto& row) {
          player room_creator;
          room_creator.account = creator;

          row.id = _state->last_room_id;
          row.last_interaction_timestamp = now();
          row.max_players = max_players;
          row.players.push_back(room_creator);
      });

     global_state.modify(_state, get_self(), [&](auto& p) {
          p.last_room_id++;
     });
     // TODO: write test for this.
}

[[eosio::action]]
void roll(uint64_t room_id, checksum256 hash)
{
     
}

[[eosio::action]]
void reveal()
{

}

[[eosio::on_notify("eosio.token::transfer")]]
void deposit(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo)
{
  if (to != get_self() || from == get_self())
  {
    print("These are not the droids you are looking for.");
    return;
  }
  check(quantity.amount > 0, "When pigs fly");

  auto depositer_itr = balances_table.find(from.value);

  if (depositer_itr != balances_table.end())
    balances_table.modify(depositer_itr, get_self(), [&](auto &row) {
      row.balance += quantity;
    });
  else
    balances_table.emplace(get_self(), [&](auto &row) {
      row.balance = quantity;
    });
}

//// DEBUG FUNCTIONS

[[eosio::action]]
void setup()
{
     global_state.emplace(get_self(), [&](auto& row) {
          row.id = 0;
          row.last_room_id = 0;
      });
}

[[eosio::action]]
void clearglobal()
{
     auto e = global_state.find(0);
     if(e != global_state.end())
     {
          global_state.erase(e);
     }
}

[[eosio::action]]
void clearbalance()
{
     auto e = balances_table.find(0);
     if(e != balances_table.end())
     {
          balances_table.erase(e);
     }
}

};


EOSIO_DISPATCH( flybet, (setup)(createroom)(roll)(reveal)(clearbalance)(clearglobal))

/* Deprecated in favor of eosio::on_notify(_action) usage
extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
   if( code == token_contract.value && action == "transfer"_n.value ) {
      eosio::execute_action(eosio::name(receiver), token_contract, &transfer_filter::ontransfer);
   }
}
*/
