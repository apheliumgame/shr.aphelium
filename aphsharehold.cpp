#include "aphsharehold.hpp"

void aphsharehold::checkschema(name schema_name)
{
    check(schema_name == name(sh_schema_name), "Please stake only NFTs from the " + sh_schema_name + " schema");
}

void aphsharehold::stake(name owner, vector<uint64_t> asset_ids)
{
    require_auth(owner);
    
    // Check if the sender owns the NFT
    assets_t owned_assets = get_assets(owner);
    stakedassets_t staked_assets = get_staked_assets(owner);
    
    for (uint64_t asset_id : asset_ids) {
        auto asset_itr = owned_assets.find(asset_id);
        check(asset_itr != owned_assets.end(), "Asset is not owned by you");
        
        checkschema(asset_itr->schema_name);
        
        schemas_t collection_schemas = get_schemas(asset_itr->collection_name);
        auto schema_itr = collection_schemas.find(asset_itr->schema_name.value);
        
        templates_t collection_templates = get_templates(asset_itr->collection_name);
        auto template_itr = collection_templates.find(asset_itr->template_id);
        
        ATTRIBUTE_MAP deserialized_immutable_data = deserialize(
            template_itr->immutable_serialized_data,
            schema_itr->format
        );
        
        uint16_t quantity = get<uint16_t>(deserialized_immutable_data[quantity_attribute]);
        double percentage_per_unit = get<double>(deserialized_immutable_data[percentage_attribute]);
        
        // Create the row
        staked_assets.emplace(owner, [&](auto &_asset) {
           _asset.id = asset_id;
           _asset.quantity = quantity;
           _asset.percentage_per_unit = percentage_per_unit;
           _asset.staked = false;
           _asset.staked_at = eosio::current_time_point();
        });
    }
}

void aphsharehold::handlenfttransfer(name collection_name, name from, name to, vector<uint64_t> asset_ids, string memo)
{
    if (to != get_self() || from == get_self()) {
        return;
    }
    
    assets_t owned_assets = get_assets(to);
    stakedassets_t staked_assets = get_staked_assets(from);
    // Get all the assets
    for (uint64_t asset_id : asset_ids) {
        auto asset_itr = owned_assets.find(asset_id);
        check(asset_itr != owned_assets.end(), "Asset id not found");
        
        // Check the schema
        checkschema(asset_itr->schema_name);
        
        // Check if the RAM for the asset is payed
        auto staked_asset_itr = staked_assets.find(asset_id);
        check(staked_asset_itr != staked_assets.end(), "RAM not payed");
        
        // Save the asset
        staked_assets.modify(staked_asset_itr, from, [&](auto &_asset) {
           _asset.staked = true;
           _asset.staked_at = eosio::current_time_point();
        });
    }
}

void aphsharehold::unstake(name to, vector<uint64_t> asset_ids)
{
    require_auth(to);
    require_recipient(to);
    
    stakedassets_t staked_assets = get_staked_assets(to);
    for (uint64_t asset_id : asset_ids) {
        auto asset_itr = staked_assets.find(asset_id);
        
        // Check if the assets exists
        check(asset_itr != staked_assets.end(), "The asset does not exists or it's not owned by you");
    }
    
    // Send back the assets
    action(
        permission_level{get_self(), name("active")},
        name("atomicassets"),
        name("transfer"),
        make_tuple(
            get_self(),
            to,
            asset_ids,
            std::string("Unstake")
        )
    ).send();
    
    // Erase the stake
    for (uint64_t asset_id : asset_ids) {
        auto asset_itr = staked_assets.find(asset_id);
        
        staked_assets.erase(asset_itr);
    }
}

void aphsharehold::topup (name from, name to, asset quantity, string memo)
{
    if (to != get_self() || from == get_self()) {
        print("These are not the droids you are looking for");
        return;
    }
    
    check(quantity.amount > 0, "Amount has to be > 0");
    
    if (memo.substr(0, 6) == "topup:") {
        // Define an array of supported symbols
        const string supported_symbols[] = {"WAX"};
        // Check if the quantity symbol is in the supported symbols array
        bool is_supported = false;
        for (const auto& sym : supported_symbols) {
            if (quantity.symbol.code() == symbol_code(sym)) {
                is_supported = true;
                break;
            }
        }
        check(is_supported, "Unsupported symbol");
  
        // Find the position of the ":" character
        size_t pos = memo.find(":");

        // Extract the substring after the ":" character
        string collection_name = memo.substr(pos + 1);
        
        auto itr = balance.find(name(collection_name).value);
        if (itr == balance.end()) {
            balance.emplace(get_self(), [&](auto &_balance) {
                _balance.collection_name = name(collection_name);
                _balance.amount = quantity;
            });
        } else {
            balance.modify(itr, get_self(), [&](auto &_balance) {
                _balance.amount += quantity;
            });
        }

    }
}


aphsharehold::assets_t aphsharehold::get_assets(name acc) {
    return assets_t(name("atomicassets"), acc.value);
}
aphsharehold::stakedassets_t aphsharehold::get_staked_assets(name acc) {
    return stakedassets_t(get_self(), acc.value);
}
aphsharehold::schemas_t aphsharehold::get_schemas(name collection_name) {
    return schemas_t(name("atomicassets"), collection_name.value);
}
aphsharehold::templates_t aphsharehold::get_templates(name collection_name) {
    return templates_t(name("atomicassets"), collection_name.value);
}