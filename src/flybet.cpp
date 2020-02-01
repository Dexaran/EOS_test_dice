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

//enum room_state { roll, reveal };
enum player_state { init, rolled, revealed };

struct [[eosio::class]] player
{
     eosio::name  account;
     checksum256  bet_hash;
     player_state status;
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
          uint64_t              entropy;

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
     eosio_assert(max_players < 3, "Temporarily restricting the max number of players to 2");

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
void closeroom(uint64_t room_id)
{
    // TODO: write the code.
}


/** roll() - players submit entropy and invoke payments by calling this function
/* hash - the hash of SEED and SECRET variables calculated offchain
*/
[[eosio::action]]
void roll(eosio::name player, uint64_t room_id, checksum256 hash)
{
     require_auth(player);

     auto room_itr = rooms_table.find(room_id);

     for (uint8_t i = 0; i < room_itr->max_players; i++)
     {
          if(player == room_itr->players[i].account)
          {
               rooms_table.modify(room_itr, get_self(), [&](auto& row) {
                    row.players[i].bet_hash = hash;
                    row.players[i].status   = player_state::rolled;
               });

               auto balances_itr = balances_table.find(player.value);

               balances_table.modify(balances_itr, get_self(), [&](auto& row) {
                    eosio_assert(row.balance.amount >= room_itr->bet, "Overdrawn balance");
                    row.balance.amount -= room_itr->bet;
               });
          }
     }
}

/** reveal() - after all the SECRETs were submitted players reveal their hashed entropy
/*
*/
[[eosio::action]]
void reveal(eosio::name player, uint64_t room_id, uint64_t secret, uint64_t seed)
{
     require_auth(player);

     bool reveal_available = true; // Check if all the players have already submitted their SECRET hashes.
     auto room_itr = rooms_table.find(room_id);
     for (uint8_t i = 0; i < room_itr->max_players; i++)
     {
          if(! (room_itr->players[i].status == player_state::rolled || room_itr->players[i].status == player_state::revealed) )
          {
               reveal_available = false;
          }
     }
     eosio_assert(reveal_available, "Wait for all players to roll and submit their entropy hashes");

     std::string _data = "" + secret + seed;

          //@print
          eosio::print(_data);

     checksum256 hash = sha256(&_data[0], _data.size());

          //@print
          eosio::print(hash);


     for (uint8_t i = 0; i < room_itr->max_players; i++)
     {
          if(player == room_itr->players[i].account)
          {
               eosio_assert(hash == room_itr->players[i].bet_hash, "Provided secret and seed do not match the provided sha256 hash");
               rooms_table.modify(room_itr, get_self(), [&](auto& row) {
                    row.players[i].status = player_state::revealed;
                    row.entropy += secret;
               });
          }
     }

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

[[eosio::action]]
void clearroom()
{
     auto e = rooms_table.find(0);
     if(e != rooms_table.end())
     {
          rooms_table.erase(e);
     }
}

     private:

     void pay_and_refresh_room(uint64_t room_id)
     {

     }

     uint64_t rng(uint64_t room_id)
     {
          auto room_itr = rooms_table.find(room_id);

          return room_itr->entropy%100;
     }

};


EOSIO_DISPATCH( flybet, (setup)(createroom)(roll)(reveal)(closeroom)(clearbalance)(clearglobal)(clearroom))

/* Deprecated in favor of eosio::on_notify(_action) usage
extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
   if( code == token_contract.value && action == "transfer"_n.value ) {
      eosio::execute_action(eosio::name(receiver), token_contract, &transfer_filter::ontransfer);
   }
}
*/
