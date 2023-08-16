#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include "includes/checkformat.hpp"
#include "includes/atomicdata.hpp"
#include <map>
#include <algorithm>
#include <ctime>

using namespace std;
using namespace eosio;

CONTRACT aphsharehold : public contract {
    public:
        using contract::contract;
        
        [[eosio::on_notify("atomicassets::logtransfer")]] void handlenfttransfer (name collection_name, name from, name to, vector<uint64_t> asset_ids, string memo);
        [[eosio::on_notify("eosio.token::transfer")]] void topup (name from, name to, asset quantity, string memo);
        
        [[eosio::action]] void stake(name owner, vector<uint64_t> asset_ids);
        [[eosio::action]] void unstake(name to, vector<uint64_t> asset_ids);
        [[eosio::action]] void shareprofits(name collection_name);
        [[eosio::action]] void claimshare(name collection_name, name staker, string yearMonth);
        
        void checkschema(name schema_name);
        uint64_t yearMonthToInt(string yearMonth);
        bool isstakevalid(time_point_sec staked_at);
    
    private:
        string sh_schema_name = "sharehold";
        string quantity_attribute = "units";
        string percentage_attribute = "percentage_per_unit";
        uint32_t staking_valid_period = 30 * 24 * 60 * 60; // 30 days
        
        struct [[eosio::table]] stakers_s {
            name staker;
            
            uint64_t primary_key() const { return staker.value; }
        };
        typedef multi_index<name("stakers"), stakers_s> stakers_t;
        stakers_t stakers = stakers_t(get_self(), get_self().value);
        
        struct [[eosio::table]] stakedassets_s {
            uint64_t id;
            uint16_t quantity;
            bool staked;
            double percentage_per_unit;
            time_point_sec staked_at;
            
            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("stakedassets"), stakedassets_s> stakedassets_t;
        stakedassets_t get_staked_assets(name acc);
        
        struct [[eosio::table]] balance_s {
            name collection_name;
            asset amount;
            
            uint64_t primary_key() const { return collection_name.value; }
        };
        typedef multi_index<name("balance"), balance_s> balance_t;
        balance_t balance = balance_t(get_self(), get_self().value);
        
        struct [[eosio::table]] claimable_s {
            name collection_name;
            asset amount;
            
            uint64_t primary_key() const { return collection_name.value; }
        };
        typedef multi_index<name("claimables"), claimable_s> claimable_t;
        claimable_t claimables = claimable_t(get_self(), get_self().value);
        
        struct [[eosio::table]] shares_s {
            string month;
            vector<uint64_t> staked_assets;
            double percentage;
            asset claimable;
            bool claimed;
            
            uint64_t primary_key() const {
                string yearMonth = month;
                replace(yearMonth.begin(), yearMonth.end(), '-', '0');
                return stoull(yearMonth); 
            }
        };
        typedef multi_index<name("shares"), shares_s> shares_t;
        shares_t get_shares(name acc);
        
        /* ATOMICASSETS */
        struct assets_s {
            uint64_t         asset_id;
            name             collection_name;
            name             schema_name;
            int32_t          template_id;
            name             ram_payer;
            vector <asset>   backed_tokens;
            vector <uint8_t> immutable_serialized_data;
            vector <uint8_t> mutable_serialized_data;
            
            uint64_t primary_key() const { return asset_id; };
        };
        typedef multi_index<name("assets"), assets_s> assets_t;
        assets_t get_assets(name acc);
        
        struct schemas_s {
            name            schema_name;
            vector <FORMAT> format;

            uint64_t primary_key() const { return schema_name.value; }
        };
        typedef multi_index<name("schemas"), schemas_s> schemas_t;
        schemas_t get_schemas(name collection_name);
        
        struct templates_s {
            int32_t          template_id;
            name             schema_name;
            bool             transferable;
            bool             burnable;
            uint32_t         max_supply;
            uint32_t         issued_supply;
            vector <uint8_t> immutable_serialized_data;
    
            uint64_t primary_key() const { return (uint64_t) template_id; }
        };
        typedef multi_index <name("templates"), templates_s> templates_t;
        templates_t get_templates(name collection_name);
};