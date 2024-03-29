#pragma once

#include "item.h"

#include <unordered_set>
#include <vector>

/*
 * Items is responsible for holding which items are allowed/banned and 
 * providing random items when needed
 *
 * Item containers will be loaded and filled at startup
 */
struct Items {
    // TODO: Not sure what was going on when this was made, but this needs cleaned up

    Items(int number_of_items) { this->number_of_items = number_of_items; }

    Item get_item(int id) { return items[id]; }

    std::vector<Item> get_items() { return items; }

    Item random_allowed_item();

    void set_items(std::vector<Item> items) { this->items = items; }

    void set_allowed_items(std::vector<Item> items) { this->allowed_items = items; }

    void populate_allowed_items(const std::vector<Item> &banned_items);

    size_t size() {
        return items.size();
    }

    size_t allowed_items_size() {
        return allowed_items.size();
    }

    int number_of_items;
    std::vector<Item> items;
    std::vector<Item> allowed_items;        //Expected to not have any gaps
};
