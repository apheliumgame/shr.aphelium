#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include "includes/checkformat.hpp"
#include "includes/atomicdata.hpp"
#include <map>

using namespace std;
using namespace eosio;

CONTRACT aphsharehold : public contract {
    public:
        using contract::contract;
        
        [[eosio::on_notify("atomicassets::logtransfer")]] void handlenfttransfer (name collection_name, name from, name to, vector<uint64_t> asset_ids, string memo);
        [[eosio::on_notify("eosio.token::transfer")]] void topup (name from, name to, asset quantity, string memo);
        
        [[eosio::action]] void stake(name owner, vector<uint64_t> asset_ids);
        [[eosio::action]] void unstake(name to, vector<uint64_t> asset_ids);
        
        void checkschema(name schema_name);
    
    private:
        string sh_schema_name = "sharehold";
        string quantity_attribute = "quantity";
        string percentage_attribute = "percentage_per_unit2";
        
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
            
            uint16_t primary_key() const { return collection_name.value; }
        };
        typedef multi_index<name("balance"), balance_s> balance_t;
        balance_t balance = balance_t(get_self(), get_self().value);
        
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